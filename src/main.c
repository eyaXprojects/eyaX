#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <limine.h>
#include "keyboard.h"
#include "shell.h"

// PIT frequency.
#define eX_PIT_FREQUENCY 100

// Early IDT
#define eX_IDT_ENTRIES 256

// Early Memory
#define eX_PAGE_SIZE 4096

struct eX_IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct eX_IDTPointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct eX_IDTEntry eX_IDT[eX_IDT_ENTRIES];
static struct eX_IDTPointer eX_IDTPointer;

#define eX_GDT_ENTRIES 3

struct eX_GDTEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct eX_GDTPointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct eX_GDTEntry eX_GDT[eX_GDT_ENTRIES];
static struct eX_GDTPointer eX_GDTPointer;

// Set the base revision to 6, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;

// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *pdest = dest;
    const uint8_t *psrc = src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = dest;
    const uint8_t *psrc = src;

    if ((uintptr_t)src > (uintptr_t)dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if ((uintptr_t)src < (uintptr_t)dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = s1;
    const uint8_t *p2 = s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

// eyaX framebuffer state.
static struct limine_framebuffer *eX_Framebuffer;

static size_t eX_CursorX = 0;
static size_t eX_CursorY = 0;

static const size_t eX_CharWidth = 8;
static const size_t eX_CharHeight = 8;

static uint32_t eX_Foreground = 0xFFFFFFFF;
static uint32_t eX_Background = 0x00000000;

// Put one pixel on the framebuffer.
static void eX_PutPixel(size_t x, size_t y, uint32_t color) {
    volatile uint32_t *pixel = eX_Framebuffer->address;

    pixel[y * (eX_Framebuffer->pitch / 4) + x] = color;
}

// this is the reason why this file is 1k lines
static const uint8_t eX_Font[128][8] = {

    ['A'] = {
        0b00011000,
        0b00100100,
        0b01000010,
        0b01000010,
        0b01111110,
        0b01000010,
        0b01000010,
        0b00000000
    },

    ['B'] = {
        0b01111100,
        0b01000010,
        0b01000010,
        0b01111100,
        0b01000010,
        0b01000010,
        0b01111100,
        0b00000000
    },

    ['C'] = {
        0b00111110,
        0b01000000,
        0b01000000,
        0b01000000,
        0b01000000,
        0b01000000,
        0b00111110,
        0b00000000
    },

    ['D'] = {
        0b01111100,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01111100,
        0b00000000
    },

    ['E'] = {
        0b01111110,
        0b01000000,
        0b01000000,
        0b01111100,
        0b01000000,
        0b01000000,
        0b01111110,
        0b00000000
    },

    ['F'] = {
        0b01111110,
        0b01000000,
        0b01000000,
        0b01111100,
        0b01000000,
        0b01000000,
        0b01000000,
        0b00000000
    },

    ['G'] = {
        0b00111110,
        0b01000000,
        0b01000000,
        0b01001110,
        0b01000010,
        0b01000010,
        0b00111110,
        0b00000000
    },

    ['H'] = {
        0b01000010,
        0b01000010,
        0b01000010,
        0b01111110,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00000000
    },

    ['I'] = {
        0b00111110,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00111110,
        0b00000000
    },

    ['J'] = {
        0b00011110,
        0b00000100,
        0b00000100,
        0b00000100,
        0b01000100,
        0b01000100,
        0b00111000,
        0b00000000
    },

    ['K'] = {
        0b01000010,
        0b01000100,
        0b01001000,
        0b01110000,
        0b01001000,
        0b01000100,
        0b01000010,
        0b00000000
    },

    ['L'] = {
        0b01000000,
        0b01000000,
        0b01000000,
        0b01000000,
        0b01000000,
        0b01000000,
        0b01111110,
        0b00000000
    },

    ['M'] = {
        0b01000010,
        0b01100110,
        0b01011010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00000000
    },

    ['N'] = {
        0b01000010,
        0b01100010,
        0b01010010,
        0b01001010,
        0b01000110,
        0b01000010,
        0b01000010,
        0b00000000
    },

    ['O'] = {
        0b00111100,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00111100,
        0b00000000
    },

    ['P'] = {
        0b01111100,
        0b01000010,
        0b01000010,
        0b01111100,
        0b01000000,
        0b01000000,
        0b01000000,
        0b00000000
    },

    ['Q'] = {
        0b00111100,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01001010,
        0b01000100,
        0b00111010,
        0b00000000
    },

    ['R'] = {
        0b01111100,
        0b01000010,
        0b01000010,
        0b01111100,
        0b01001000,
        0b01000100,
        0b01000010,
        0b00000000
    },

    ['S'] = {
        0b00111110,
        0b01000000,
        0b01000000,
        0b00111100,
        0b00000010,
        0b00000010,
        0b01111100,
        0b00000000
    },

    ['T'] = {
        0b01111110,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00000000
    },

    ['U'] = {
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00111100,
        0b00000000
    },

    ['V'] = {
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00100100,
        0b00100100,
        0b00011000,
        0b00000000
    },

    ['W'] = {
        0b01000010,
        0b01000010,
        0b01000010,
        0b01011010,
        0b01011010,
        0b01100110,
        0b01000010,
        0b00000000
    },

    ['X'] = {
        0b10000001,
        0b01000010,
        0b00100100,
        0b00011000,
        0b00011000,
        0b00100100,
        0b01000010,
        0b10000001
    },

    ['Y'] = {
        0b01000010,
        0b01000010,
        0b00100100,
        0b00011000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00000000
    },

    ['Z'] = {
        0b01111110,
        0b00000010,
        0b00000100,
        0b00001000,
        0b00010000,
        0b00100000,
        0b01111110,
        0b00000000
    },

    ['a'] = {
        0b00000000,
        0b00000000,
        0b00111000,
        0b00000100,
        0b00111100,
        0b01000100,
        0b00111100,
        0b00000000
    },

    ['b'] = {
        0b01000000,
        0b01000000,
        0b01011100,
        0b01100010,
        0b01000010,
        0b01100010,
        0b01011100,
        0b00000000
    },

    ['c'] = {
        0b00000000,
        0b00000000,
        0b00111100,
        0b01000000,
        0b01000000,
        0b01000000,
        0b00111100,
        0b00000000
    },

    ['d'] = {
        0b00000010,
        0b00000010,
        0b00111010,
        0b01000110,
        0b01000010,
        0b01000110,
        0b00111010,
        0b00000000
    },

    ['e'] = {
        0b00000000,
        0b00000000,
        0b00111000,
        0b01000100,
        0b01111100,
        0b01000000,
        0b00111100,
        0b00000000
    },

    ['f'] = {
        0b00011100,
        0b00100000,
        0b00100000,
        0b01111000,
        0b00100000,
        0b00100000,
        0b00100000,
        0b00000000
    },

    ['g'] = {
        0b00000000,
        0b00111010,
        0b01000110,
        0b01000010,
        0b01000110,
        0b00111010,
        0b00000010,
        0b00111100
    },

    ['h'] = {
        0b01000000,
        0b01000000,
        0b01011100,
        0b01100010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00000000
    },

    ['i'] = {
        0b00001000,
        0b00000000,
        0b00111000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00111110,
        0b00000000
    },

    ['j'] = {
        0b00000100,
        0b00000000,
        0b00011100,
        0b00000100,
        0b00000100,
        0b00000100,
        0b01000100,
        0b00111000
    },

    ['k'] = {
        0b01000000,
        0b01000000,
        0b01000100,
        0b01001000,
        0b01110000,
        0b01001000,
        0b01000100,
        0b00000000
    },

    ['l'] = {
        0b00110000,
        0b00010000,
        0b00010000,
        0b00010000,
        0b00010000,
        0b00010000,
        0b00111000,
        0b00000000
    },

    ['m'] = {
        0b00000000,
        0b00000000,
        0b01101100,
        0b01010010,
        0b01010010,
        0b01010010,
        0b01010010,
        0b00000000
    },

    ['n'] = {
        0b00000000,
        0b00000000,
        0b01011100,
        0b01100010,
        0b01000010,
        0b01000010,
        0b01000010,
        0b00000000
    },

    ['o'] = {
        0b00000000,
        0b00000000,
        0b00111000,
        0b01000100,
        0b01000100,
        0b01000100,
        0b00111000,
        0b00000000
    },

    ['p'] = {
        0b00000000,
        0b01011100,
        0b01100010,
        0b01000010,
        0b01100010,
        0b01011100,
        0b01000000,
        0b01000000
    },

    ['q'] = {
        0b00000000,
        0b00111010,
        0b01000110,
        0b01000010,
        0b01000110,
        0b00111010,
        0b00000010,
        0b00000010
    },

    ['r'] = {
        0b00000000,
        0b00000000,
        0b01011100,
        0b01100010,
        0b01000000,
        0b01000000,
        0b01000000,
        0b00000000
    },

    ['s'] = {
        0b00000000,
        0b00000000,
        0b00111100,
        0b01000000,
        0b00111000,
        0b00000100,
        0b01111000,
        0b00000000
    },

    ['t'] = {
        0b00100000,
        0b00100000,
        0b01111000,
        0b00100000,
        0b00100000,
        0b00100000,
        0b00011100,
        0b00000000
    },

    ['u'] = {
        0b00000000,
        0b00000000,
        0b01000010,
        0b01000010,
        0b01000010,
        0b01000110,
        0b00111010,
        0b00000000
    },

    ['v'] = {
        0b00000000,
        0b00000000,
        0b01000010,
        0b01000010,
        0b00100100,
        0b00100100,
        0b00011000,
        0b00000000
    },

    ['w'] = {
        0b00000000,
        0b00000000,
        0b01000010,
        0b01011010,
        0b01011010,
        0b01100110,
        0b01000010,
        0b00000000
    },

    ['x'] = {
        0b00000000,
        0b00000000,
        0b01000010,
        0b00100100,
        0b00011000,
        0b00100100,
        0b01000010,
        0b00000000
    },

    ['y'] = {
        0b00000000,
        0b01000010,
        0b01000010,
        0b00100100,
        0b00011000,
        0b00001000,
        0b00010000,
        0b01100000
    },

    ['z'] = {
        0b00000000,
        0b00000000,
        0b01111100,
        0b00000100,
        0b00011000,
        0b00100000,
        0b01111100,
        0b00000000
    },

    ['0'] = {
        0b00111100,
        0b01000010,
        0b01000110,
        0b01001010,
        0b01010010,
        0b01100010,
        0b00111100,
        0b00000000
    },

    ['1'] = {
        0b00001000,
        0b00011000,
        0b00101000,
        0b00001000,
        0b00001000,
        0b00001000,
        0b00111110,
        0b00000000
    },

    ['2'] = {
        0b00111100,
        0b01000010,
        0b00000010,
        0b00001100,
        0b00110000,
        0b01000000,
        0b01111110,
        0b00000000
    },

    ['3'] = {
        0b00111100,
        0b01000010,
        0b00000010,
        0b00011100,
        0b00000010,
        0b01000010,
        0b00111100,
        0b00000000
    },

    ['4'] = {
        0b00000100,
        0b00001100,
        0b00010100,
        0b00100100,
        0b01111110,
        0b00000100,
        0b00000100,
        0b00000000
    },

    ['5'] = {
        0b01111110,
        0b01000000,
        0b01000000,
        0b01111100,
        0b00000010,
        0b01000010,
        0b00111100,
        0b00000000
    },

    ['6'] = {
        0b00111100,
        0b01000000,
        0b01000000,
        0b01111100,
        0b01000010,
        0b01000010,
        0b00111100,
        0b00000000
    },

    ['7'] = {
        0b01111110,
        0b00000010,
        0b00000100,
        0b00001000,
        0b00010000,
        0b00010000,
        0b00010000,
        0b00000000
    },

    ['8'] = {
        0b00111100,
        0b01000010,
        0b01000010,
        0b00111100,
        0b01000010,
        0b01000010,
        0b00111100,
        0b00000000
    },

    ['9'] = {
        0b00111100,
        0b01000010,
        0b01000010,
        0b00111110,
        0b00000010,
        0b00000010,
        0b00111100,
        0b00000000
    },

    ['.'] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00011000,
        0b00011000,
        0b00000000
    },

    [','] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00011000,
        0b00011000,
        0b00110000
    },

    [':'] = {
        0b00000000,
        0b00011000,
        0b00011000,
        0b00000000,
        0b00000000,
        0b00011000,
        0b00011000,
        0b00000000
    },

    ['!'] = {
        0b00011000,
        0b00011000,
        0b00011000,
        0b00011000,
        0b00011000,
        0b00000000,
        0b00011000,
        0b00000000
    },

    ['?'] = {
        0b00111100,
        0b01000010,
        0b00000010,
        0b00001100,
        0b00011000,
        0b00000000,
        0b00011000,
        0b00000000
    },

    ['-'] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b01111110,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    },

    ['_'] = {
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b01111110
    },

    ['('] = {
        0b00001100,
        0b00010000,
        0b00100000,
        0b00100000,
        0b00100000,
        0b00010000,
        0b00001100,
        0b00000000
    },

    [')'] = {
        0b00110000,
        0b00001000,
        0b00000100,
        0b00000100,
        0b00000100,
        0b00001000,
        0b00110000,
        0b00000000
    },

    ['/'] = {
        0b00000010,
        0b00000100,
        0b00001000,
        0b00010000,
        0b00100000,
        0b01000000,
        0b00000000,
        0b00000000
    },

    ['\\'] = {
        0b01000000,
        0b00100000,
        0b00010000,
        0b00001000,
        0b00000100,
        0b00000010,
        0b00000000,
        0b00000000
    },

    ['='] = {
        0b00000000,
        0b00000000,
        0b01111110,
        0b00000000,
        0b01111110,
        0b00000000,
        0b00000000,
        0b00000000
    },

    ['+'] = {
        0b00000000,
        0b00001000,
        0b00001000,
        0b00111110,
        0b00001000,
        0b00001000,
        0b00000000,
        0b00000000
    },

    ['%'] = {
        0b01100010,
        0b01100100,
        0b00001000,
        0b00010000,
        0b00100000,
        0b01001100,
        0b10001100,
        0b00000000
    },

    ['#'] = {
        0b00010100,
        0b00010100,
        0b01111110,
        0b00010100,
        0b01111110,
        0b00010100,
        0b00010100,
        0b00000000
    },

    ['@'] = {
        0b00111100,
        0b01000010,
        0b01001110,
        0b01010010,
        0b01001110,
        0b01000000,
        0b00111100,
        0b00000000
    },

    ['*'] = {
        0b00000000,
        0b00100100,
        0b00011000,
        0b01111110,
        0b00011000,
        0b00100100,
        0b00000000,
        0b00000000
    },

    ['"'] = {
        0b00100100,
        0b00100100,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    },

    ['\''] = {
        0b00001000,
        0b00001000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000,
        0b00000000
    },

    ['>'] = {
        0b00000000,
        0b00000000,
        0b01000000,
        0b00100000,
        0b00010000,
        0b00100000,
        0b01000000,
        0b00000000
    },

    ['<'] = {
        0b00000000,
        0b00000000,
        0b00000100,
        0b00001000,
        0b00010000,
        0b00001000,
        0b00000100,
        0b00000000
    }
};

// Draw one character at a pixel position.
static void eX_DrawChar(char c, size_t x, size_t y) {
    if ((unsigned char)c >= 128) {
        return;
    }

    const uint8_t *glyph = eX_Font[(unsigned char)c];

    for (size_t row = 0; row < eX_CharHeight; row++) {
        for (size_t col = 0; col < eX_CharWidth; col++) {
            if (glyph[row] & (1 << (7 - col))) {
                eX_PutPixel(
                    x + col,
                    y + row,
                    eX_Foreground
                );
            } else {
                eX_PutPixel(
                    x + col,
                    y + row,
                    eX_Background
                );
            }
        }
    }
}

// Print one character.
void eX_PrintChar(char c) {
    if (c == '\n') {
        eX_CursorX = 0;
        eX_CursorY++;
        return;
    }

    eX_DrawChar(
        c,
        eX_CursorX * eX_CharWidth,
        eX_CursorY * eX_CharHeight
    );

    eX_CursorX++;
}

static void eX_PrintString(const char *string) {
    while (*string) {
        eX_PrintChar(*string);
        string++;
    }
}

static void eX_PrintNumberUnsigned(uint64_t value, uint32_t base, bool uppercase) {
    char buffer[32];
    size_t i = 0;

    const char *digits;

    if (uppercase) {
        digits = "0123456789ABCDEF";
    } else {
        digits = "0123456789abcdef";
    }

    if (value == 0) {
        eX_PrintChar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = digits[value % base];
        value /= base;
    }

    while (i > 0) {
        eX_PrintChar(buffer[--i]);
    }
}

static void eX_PrintNumberSigned(int64_t value) {
    if (value < 0) {
        eX_PrintChar('-');

        /*
         * Avoid overflowing when value is INT64_MIN.
         */
        uint64_t magnitude = (uint64_t)(-(value + 1)) + 1;
        eX_PrintNumberUnsigned(magnitude, 10, false);
    } else {
        eX_PrintNumberUnsigned((uint64_t)value, 10, false);
    }
}

void eX_Print(const char *format, ...) {
    va_list args;

    va_start(args, format);

    while (*format) {
        if (*format != '%') {
            eX_PrintChar(*format);
            format++;
            continue;
        }

        format++;

        switch (*format) {
            case '%':
                eX_PrintChar('%');
                break;

            case 'c':
                eX_PrintChar((char)va_arg(args, int));
                break;

            case 's': {
                const char *string = va_arg(args, const char *);

                if (string == NULL) {
                    eX_PrintString("(null)");
                } else {
                    eX_PrintString(string);
                }

                break;
            }

            case 'd':
                eX_PrintNumberSigned(va_arg(args, int));
                break;

            case 'u':
                eX_PrintNumberUnsigned(
                    va_arg(args, unsigned int),
                                       10,
                                       false
                );
                break;

            case 'x':
                eX_PrintNumberUnsigned(
                    va_arg(args, unsigned int),
                                       16,
                                       false
                );
                break;

            case 'X':
                eX_PrintNumberUnsigned(
                    va_arg(args, unsigned int),
                                       16,
                                       true
                );
                break;

            default:
                eX_PrintChar('%');
                eX_PrintChar(*format);
                break;
        }

        format++;
    }

    va_end(args);
}

void eX_Clear(void) {
    volatile uint32_t *pixel = eX_Framebuffer->address;

    size_t pitch_pixels = eX_Framebuffer->pitch / 4;

    for (size_t y = 0; y < eX_Framebuffer->height; y++) {
        for (size_t x = 0; x < eX_Framebuffer->width; x++) {
            pixel[y * pitch_pixels + x] = eX_Background;
        }
    }

    eX_CursorX = 0;
    eX_CursorY = 0;
}

__attribute__((interrupt))
static void eX_TimerHandler(void *frame);

// outb
static inline void eX_Outb(uint16_t port, uint8_t value) {
    asm volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static void eX_PIC_Init(void) {
    eX_Outb(0x20, 0x11);
    eX_Outb(0xA0, 0x11);

    eX_Outb(0x21, 0x20);
    eX_Outb(0xA1, 0x28);

    eX_Outb(0x21, 0x04);
    eX_Outb(0xA1, 0x02);

    eX_Outb(0x21, 0x01);
    eX_Outb(0xA1, 0x01);

    // Enable IRQ0: timer
    // Enable IRQ1: keyboard
    eX_Outb(0x21, 0xFC);

    // Mask all slave PIC IRQs.
    eX_Outb(0xA1, 0xFF);
}

static void eX_PIT_Init(void) {
    uint16_t divisor = 1193182 / eX_PIT_FREQUENCY;

    eX_Outb(0x43, 0x36);

    eX_Outb(0x40, divisor & 0xFF);
    eX_Outb(0x40, divisor >> 8);
}

// PIT (aka, a pain in the bottom)
static volatile uint64_t eX_TimerTicks = 0;

static void eX_Sleep(uint64_t seconds) {
    uint64_t target = eX_TimerTicks + (seconds * eX_PIT_FREQUENCY);

    while (eX_TimerTicks < target) {
        asm volatile ("hlt");
    }
}

static void eX_InvalidOpcodeHandler(void *frame);

static void eX_BugCheck(
    const char *exception,
    uint64_t vector,
    uint64_t address
);

// Page Fault
__attribute__((interrupt))
static void eX_PageFaultHandler(void *frame) {
    (void)frame;

    uint64_t cr2;

    asm volatile (
        "mov %%cr2, %0"
        : "=r"(cr2)
    );

    eX_BugCheck(
        "Page Fault",
        0x0E,
        cr2
    );
}

static void eX_IDT_Load(void) {
    asm volatile (
        "lidt %0"
        :
        : "m"(eX_IDTPointer)
    );
}

static uint16_t eX_ReadCS(void) {
    uint16_t cs;

    asm volatile (
        "mov %%cs, %0"
        : "=r"(cs)
    );

    return cs;
}

void eX_IDT_SetGate(
    uint8_t vector,
    uint64_t handler
) {
    eX_IDT[vector].offset_low  = handler & 0xFFFF;
    eX_IDT[vector].selector = 0x08;
    eX_IDT[vector].ist         = 0;
    eX_IDT[vector].type_attr   = 0x8E;
    eX_IDT[vector].offset_mid  = (handler >> 16) & 0xFFFF;
    eX_IDT[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    eX_IDT[vector].zero        = 0;
}

__attribute__((interrupt))
static void eX_TimerHandler(void *frame) {
    (void)frame;

    eX_TimerTicks++;

    if (eX_TimerTicks == 1) {
        eX_Print("IRQ0 FIRED!\n");
    }

    eX_Outb(0x20, 0x20);
}

static void eX_InvalidOpcodeHandler(void *frame);

static void eX_IDT_Init(void) {
    for (size_t i = 0; i < eX_IDT_ENTRIES; i++) {
        eX_IDT[i] = (struct eX_IDTEntry){0};
    }

    eX_IDTPointer.limit =
    sizeof(eX_IDT) - 1;

    eX_IDTPointer.base =
    (uint64_t)&eX_IDT;

    eX_IDT_SetGate(
        0x06,
        (uint64_t)eX_InvalidOpcodeHandler
    );

    eX_IDT_SetGate(
        0x20,
        (uint64_t)eX_TimerHandler
    );

    eX_IDT_SetGate(
        0x0E,
        (uint64_t)eX_PageFaultHandler
    );

    eX_IDT_Load();
}

static inline uint8_t eX_Inb(uint16_t port) {
    uint8_t value;

    asm volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static uint8_t eX_PIC_ReadIRR(void) {
    eX_Outb(0x20, 0x0A);
    return eX_Inb(0x20);
}

static void eX_DrawScaledChar(
    uint32_t x,
    uint32_t y,
    char c,
    uint32_t scale
) {
    if ((unsigned char)c >= 128) {
        return;
    }

    const uint8_t *glyph = eX_Font[(unsigned char)c];

    for (uint32_t row = 0; row < 8; row++) {
        for (uint32_t col = 0; col < 8; col++) {
            if (glyph[row] & (1 << col)) {
                for (uint32_t dy = 0; dy < scale; dy++) {
                    for (uint32_t dx = 0; dx < scale; dx++) {
                        eX_PutPixel(
                            x + (col * scale) + dx,
                                    y + (row * scale) + dy,
                                    eX_Foreground
                        );
                    }
                }
            }
        }
    }
}

static void eX_DrawLogo(void) {
    const char *logo = "Xaye";

    const uint32_t scale = 4;
    const uint32_t margin = 16;

    uint32_t width = 4 * eX_CharWidth * scale;

    uint32_t x =
    eX_Framebuffer->width - width - margin;

    uint32_t y = margin;

    for (size_t i = 0; i < 4; i++) {
        const uint8_t *glyph =
        eX_Font[(unsigned char)logo[3 - i]];

        uint32_t char_x =
        x + (i * eX_CharWidth * scale);

        for (size_t row = 0; row < eX_CharHeight; row++) {
            for (size_t col = 0; col < eX_CharWidth; col++) {
                if (glyph[row] & (1 << (7 - col))) {
                    for (uint32_t dy = 0; dy < scale; dy++) {
                        for (uint32_t dx = 0; dx < scale; dx++) {
                            eX_PutPixel(
                                char_x + (col * scale) + dx,
                                        y + (row * scale) + dy,
                                        eX_Foreground
                            );
                        }
                    }
                }
            }
        }
    }
}

static void eX_GDT_Load(void) {
    asm volatile (
        "lgdt %0\n"
        :
        : "m"(eX_GDTPointer)
        : "memory"
    );

    asm volatile (
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        :
        :
        : "rax", "memory"
    );

    asm volatile (
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"
        :
        :
        : "rax", "memory"
    );
}

static void eX_GDT_Init(void) {
    eX_GDT[0] = (struct eX_GDTEntry){0};

    eX_GDT[1] = (struct eX_GDTEntry){
        .limit_low = 0xFFFF,
        .base_low = 0,
        .base_mid = 0,
        .access = 0x9A,
        .granularity = 0xAF,
        .base_high = 0
    };

    eX_GDT[2] = (struct eX_GDTEntry){
        .limit_low = 0xFFFF,
        .base_low = 0,
        .base_mid = 0,
        .access = 0x92,
        .granularity = 0xCF,
        .base_high = 0
    };

    eX_GDTPointer.limit =
    sizeof(eX_GDT) - 1;

    eX_GDTPointer.base =
    (uint64_t)&eX_GDT;

    eX_GDT_Load();
}

static void eX_BugCheck(const char *exception, uint64_t vector, uint64_t address) {
    eX_Clear();

    const uint32_t scale = 8;
    const uint32_t margin = 16;

    const char *symbol = "X";

    const uint8_t *glyph =
    eX_Font[(unsigned char)symbol[0]];

    for (size_t row = 0; row < eX_CharHeight; row++) {
        for (size_t col = 0; col < eX_CharWidth; col++) {
            if (glyph[row] & (1 << (7 - col))) {
                for (uint32_t dy = 0; dy < scale; dy++) {
                    for (uint32_t dx = 0; dx < scale; dx++) {
                        eX_PutPixel(
                            margin + (col * scale) + dx,
                                    margin + (row * scale) + dy,
                                    eX_Foreground
                        );
                    }
                }
            }
        }
    }

    eX_Print("\n");
    eX_Print(
        "A problem has occurred and the eyaX kernel halted "
        "to protect the computer and the CPU, Logs are below.\n\n"
    );

    eX_Print("Exception: %s\n", exception);
    eX_Print("Vector: 0x%X\n", vector);
    eX_Print("Address: 0x%X\n", address);

    eX_Print("\nSystem halted. to reinstate the system please reboot the computer.\n");

    hcf();
}

__attribute__((interrupt))
static void eX_InvalidOpcodeHandler(void *frame) {
    (void)frame;

    eX_BugCheck("Invalid Opcode", 6, 0);
}

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request eX_MemoryMapRequest = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

// Memory
static uint8_t *eX_PageBitmap;
static uint64_t eX_PageCount;
static uint64_t eX_FreePageCount;

static void eX_PageSet(uint64_t page) {
    eX_PageBitmap[page / 8] |=
    (uint8_t)(1 << (page % 8));
}

static void eX_PageClear(uint64_t page) {
    eX_PageBitmap[page / 8] &=
    (uint8_t)~(1 << (page % 8));
}

static bool eX_PageIsUsed(uint64_t page) {
    return
    (eX_PageBitmap[page / 8] &
    (uint8_t)(1 << (page % 8))) != 0;
}

static uint64_t eX_PageBitmapSize(void) {
    return (eX_PageCount + 7) / 8;
}

static void eX_PPA_Init(void) {
    uint64_t highest_address = 0;

    for (uint64_t i = 0;
         i < eX_MemoryMapRequest.response->entry_count;
    i++) {

        struct limine_memmap_entry *entry =
        eX_MemoryMapRequest.response->entries[i];

        uint64_t end =
        entry->base + entry->length;

        if (end > highest_address) {
            highest_address = end;
        }
    }

    eX_PageCount =
    (highest_address + eX_PAGE_SIZE - 1) /
    eX_PAGE_SIZE;

    uint64_t bitmap_size =
    (eX_PageCount + 7) / 8;

    eX_PageBitmap =
    NULL;

    for (uint64_t i = 0;
         i < eX_MemoryMapRequest.response->entry_count;
    i++) {

        struct limine_memmap_entry *entry =
        eX_MemoryMapRequest.response->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        if (entry->length >= bitmap_size) {
            eX_PageBitmap =
            (uint8_t *)entry->base;

            break;
        }
    }

    if (eX_PageBitmap == NULL) {
        eX_Print("PPA: Cannot place bitmap!\n");
        hcf();
    }

    for (uint64_t i = 0; i < bitmap_size; i++) {
        eX_PageBitmap[i] = 0xFF;
    }

    eX_FreePageCount = 0;

    for (uint64_t i = 0;
         i < eX_MemoryMapRequest.response->entry_count;
    i++) {

        struct limine_memmap_entry *entry =
        eX_MemoryMapRequest.response->entries[i];

        if (entry->type != LIMINE_MEMMAP_USABLE) {
            continue;
        }

        uint64_t first_page =
        (entry->base + eX_PAGE_SIZE - 1) /
        eX_PAGE_SIZE;

        uint64_t last_page =
        (entry->base + entry->length) /
        eX_PAGE_SIZE;

        for (uint64_t page = first_page;
             page < last_page;
        page++) {

            eX_PageClear(page);
            eX_FreePageCount++;
        }
    }

    uint64_t bitmap_first_page =
    ((uint64_t)eX_PageBitmap) /
    eX_PAGE_SIZE;

    uint64_t bitmap_last_page =
    ((uint64_t)eX_PageBitmap + bitmap_size +
    eX_PAGE_SIZE - 1) /
    eX_PAGE_SIZE;

    for (uint64_t page = bitmap_first_page;
         page < bitmap_last_page;
    page++) {

        if (!eX_PageIsUsed(page)) {
            eX_PageSet(page);
            eX_FreePageCount--;
        }
    }
}

void eX_PIC_UnmaskIRQ(uint8_t irq) {
    if (irq < 8) {
        uint8_t mask = eX_Inb(0x21);
        mask &= (uint8_t)~(1 << irq);
        eX_Outb(0x21, mask);
        return;
    }

    if (irq < 16) {
        uint8_t mask = eX_Inb(0xA1);
        mask &= (uint8_t)~(1 << (irq - 8));
        eX_Outb(0xA1, mask);
    }
}

void eX_Backspace(void) {
    if (eX_CursorX == 0) {
        return;
    }

    eX_CursorX--;

    eX_DrawChar(
        ' ',
        eX_CursorX * eX_CharWidth,
        eX_CursorY * eX_CharHeight
    );
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    if (framebuffer_request.response == NULL ||
        framebuffer_request.response->framebuffer_count < 1) {
        hcf();
        }

        if (eX_MemoryMapRequest.response == NULL) {
            hcf();
        }

        eX_Framebuffer =
        framebuffer_request.response->framebuffers[0];

    if (eX_Framebuffer->bpp != 32) {
        hcf();
    }

    eX_Clear();
    eX_DrawLogo();

    eX_Print("Before GDT: CS = 0x%X\n", eX_ReadCS());

    eX_GDT_Init();

    eX_Print("After GDT: CS = 0x%X\n", eX_ReadCS());

    eX_Print("\nMemory map:\n");

    uint64_t total_usable = 0;

    for (uint64_t i = 0;
         i < eX_MemoryMapRequest.response->entry_count;
    i++) {

        struct limine_memmap_entry *entry =
        eX_MemoryMapRequest.response->entries[i];

        eX_Print(
            "Type: %u  Base: 0x%X  Length: 0x%X\n",
            (unsigned int)entry->type,
                 (unsigned int)entry->base,
                 (unsigned int)entry->length
        );

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            total_usable += entry->length;
        }
    }

    eX_Print(
        "Total usable memory: %u bytes\n",
        (unsigned int)total_usable
    );

    eX_IDT_Init();

    eX_Print("Testing IDT...\n");

    asm volatile ("int $0x20");

    eX_Print("IDT test returned.\n");

    eX_PIC_Init();
    eX_PIT_Init();

    eX_Outb(0x43, 0x00);

    uint8_t low = eX_Inb(0x40);
    uint8_t high = eX_Inb(0x40);

    uint16_t count1 =
    ((uint16_t)high << 8) | low;

    for (volatile uint64_t i = 0; i < 1000000; i++) {
        asm volatile ("pause");
    }

    eX_Print(
        "PPA initialized. Free pages: %u\n",
        (unsigned int)eX_FreePageCount
    );

    eX_Outb(0x43, 0x00);

    low = eX_Inb(0x40);
    high = eX_Inb(0x40);

    uint16_t count2 =
    ((uint16_t)high << 8) | low;

    eX_Print("PIT: %u -> %u\n", count1, count2);

    eX_Outb(0x43, 0x00);

    uint8_t pit_status = eX_Inb(0x40);

    uint8_t pic_mask = eX_Inb(0x21);

    eX_Print("PIC mask: 0x%X\n", pic_mask);
    eX_Print("PIT read: 0x%X\n", pit_status);

    for (volatile uint64_t i = 0; i < 1000000; i++) {
        asm volatile ("pause");
    }

    uint8_t irr = eX_PIC_ReadIRR();

    eX_Print("PIC IRR: 0x%X\n", irr);

    eX_Print("KBC STATUS: 0x%x\n", eX_Keyboard_GetControllerStatus());
    eX_Print("PIC MASK: 0x%x\n", eX_Keyboard_GetPICMask());

    eX_Keyboard_Init();

    asm volatile ("sti");

    eX_Print("Tests completed\n");
    eX_Print("eyaX kernel, beta 0.1\n\n");

    eX_Shell_Init();


    uint64_t last_keyboard_irq = 0;

    for (;;) {
        eX_Keyboard_Poll();
        eX_Shell_Update();
    }
}

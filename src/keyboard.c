#include "keyboard.h"

#define eX_KEYBOARD_DATA_PORT        0x60
#define eX_KEYBOARD_STATUS_PORT      0x64

#define eX_KBC_STATUS_OUTPUT_FULL   0x01

#define eX_KEYBOARD_BUFFER_SIZE      128

static volatile char eX_KeyboardBuffer[eX_KEYBOARD_BUFFER_SIZE];
static volatile uint8_t eX_KeyboardReadIndex;
static volatile uint8_t eX_KeyboardWriteIndex;

static bool eX_ShiftPressed;
static bool eX_CapsLock;
static bool eX_ExtendedScancode;

static inline uint8_t eX_Keyboard_Inb(uint16_t port) {
    uint8_t value;

    asm volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void eX_Keyboard_Outb(uint16_t port, uint8_t value) {
    asm volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static void eX_Keyboard_PushChar(char c) {
    uint8_t next =
    (uint8_t)(
        (eX_KeyboardWriteIndex + 1) %
        eX_KEYBOARD_BUFFER_SIZE
    );

    if (next == eX_KeyboardReadIndex) {
        return;
    }

    eX_KeyboardBuffer[eX_KeyboardWriteIndex] = c;
    eX_KeyboardWriteIndex = next;
}

int eX_Keyboard_GetChar(void) {
    char c;

    if (eX_KeyboardReadIndex == eX_KeyboardWriteIndex) {
        return -1;
    }

    c = eX_KeyboardBuffer[eX_KeyboardReadIndex];

    eX_KeyboardReadIndex =
    (uint8_t)(
        (eX_KeyboardReadIndex + 1) %
        eX_KEYBOARD_BUFFER_SIZE
    );

    return (unsigned char)c;
}

uint8_t eX_Keyboard_GetControllerStatus(void) {
    return eX_Keyboard_Inb(eX_KEYBOARD_STATUS_PORT);
}

uint8_t eX_Keyboard_GetPICMask(void) {
    return eX_Keyboard_Inb(0x21);
}

uint8_t eX_Keyboard_GetPICIRR(void) {
    eX_Keyboard_Outb(0x20, 0x0A);
    return eX_Keyboard_Inb(0x20);
}

static const char eX_KeyboardNormal[128] = {
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',
    [0x0C] = '-',
    [0x0D] = '=',
    [0x0E] = '\b',
    [0x0F] = '\t',

    [0x10] = 'q',
    [0x11] = 'w',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',

    [0x1A] = '[',
    [0x1B] = ']',
    [0x1C] = '\n',

    [0x1E] = 'a',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = ';',
    [0x28] = '\'',
    [0x29] = '`',

    [0x2B] = '\\',

    [0x2C] = 'z',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = 'm',
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',
    [0x39] = ' '
};

static const char eX_KeyboardShift[128] = {
    [0x02] = '!',
    [0x03] = '@',
    [0x04] = '#',
    [0x05] = '$',
    [0x06] = '%',
    [0x07] = '^',
    [0x08] = '&',
    [0x09] = '*',
    [0x0A] = '(',
    [0x0B] = ')',
    [0x0C] = '_',
    [0x0D] = '+',

    [0x10] = 'Q',
    [0x11] = 'W',
    [0x12] = 'E',
    [0x13] = 'R',
    [0x14] = 'T',
    [0x15] = 'Y',
    [0x16] = 'U',
    [0x17] = 'I',
    [0x18] = 'O',
    [0x19] = 'P',

    [0x1A] = '{',
    [0x1B] = '}',

    [0x1E] = 'A',
    [0x1F] = 'S',
    [0x20] = 'D',
    [0x21] = 'F',
    [0x22] = 'G',
    [0x23] = 'H',
    [0x24] = 'J',
    [0x25] = 'K',
    [0x26] = 'L',
    [0x27] = ':',
    [0x28] = '"',
    [0x29] = '~',

    [0x2B] = '|',

    [0x2C] = 'Z',
    [0x2D] = 'X',
    [0x2E] = 'C',
    [0x2F] = 'V',
    [0x30] = 'B',
    [0x31] = 'N',
    [0x32] = 'M',
    [0x33] = '<',
    [0x34] = '>',
    [0x35] = '?',
    [0x39] = ' '
};

static void eX_Keyboard_HandleScancode(uint8_t scancode) {
    bool released = (scancode & 0x80) != 0;
    uint8_t code = scancode & 0x7F;

    if (scancode == 0xE0) {
        eX_ExtendedScancode = true;
        return;
    }

    if (eX_ExtendedScancode) {
        eX_ExtendedScancode = false;
        return;
    }

    if (code == 0x2A || code == 0x36) {
        eX_ShiftPressed = !released;
        return;
    }

    if (code == 0x3A) {
        if (!released) {
            eX_CapsLock = !eX_CapsLock;
        }

        return;
    }

    if (released) {
        return;
    }

    char c;

    if (eX_ShiftPressed) {
        c = eX_KeyboardShift[code];
    } else {
        c = eX_KeyboardNormal[code];
    }

    if (eX_CapsLock && c >= 'a' && c <= 'z') {
        c = (char)(c - 'a' + 'A');
    } else if (eX_CapsLock && c >= 'A' && c <= 'Z') {
        c = (char)(c - 'A' + 'a');
    }

    if (c != '\0') {
        eX_Keyboard_PushChar(c);
    }
}

void eX_Keyboard_Init(void) {
    eX_KeyboardReadIndex = 0;
    eX_KeyboardWriteIndex = 0;

    eX_ShiftPressed = false;
    eX_CapsLock = false;
    eX_ExtendedScancode = false;
}

void eX_Keyboard_Poll(void) {
    while (
        eX_Keyboard_Inb(eX_KEYBOARD_STATUS_PORT) &
        eX_KBC_STATUS_OUTPUT_FULL
    ) {
        uint8_t scancode =
        eX_Keyboard_Inb(eX_KEYBOARD_DATA_PORT);

        eX_Keyboard_HandleScancode(scancode);
    }
}

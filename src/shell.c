#include "shell.h"
#include "keyboard.h"

#include <stddef.h>
#include <stdbool.h>

#define eX_SHELL_BUFFER_SIZE 128
extern void eX_Clear(void);

static char eX_ShellBuffer[eX_SHELL_BUFFER_SIZE];
static size_t eX_ShellLength;

extern void eX_Print(const char *format, ...);
extern void eX_PrintChar(char c);
extern void eX_Backspace(void);

static void eX_Shell_Prompt(void) {
    eX_Print("eyaX> ");
}

void eX_Shell_Init(void) {
    eX_ShellLength = 0;
    eX_Shell_Prompt();
}

static void eX_Shell_Backspace(void) {
    if (eX_ShellLength == 0) {
        return;
    }

    eX_ShellLength--;

    eX_Backspace();
}

static bool eX_Shell_StringEquals(
    const char *a,
    const char *b
) {
    while (*a && *b) {
        if (*a != *b) {
            return false;
        }

        a++;
        b++;
    }

    return *a == *b;
}

static void eX_Shell_Enter(void) {
    eX_ShellBuffer[eX_ShellLength] = '\0';

    eX_Print("\n");

    if (eX_Shell_StringEquals(eX_ShellBuffer, "help")) {
        eX_Print("Available commands:\n");
        eX_Print("  help\n");
        eX_Print("  clear\n");
        eX_Print("  echo\n");
        eX_Print("  version\n");
    } else if (eX_Shell_StringEquals(eX_ShellBuffer, "clear")) {
        eX_Clear();
    } else if (eX_Shell_StringEquals(eX_ShellBuffer, "version")) {
        eX_Print("eyaX kernel beta 0.1\n");
    } else if (eX_ShellLength > 0) {
        eX_Print("Unknown command: %s\n", eX_ShellBuffer);
    }

    eX_ShellLength = 0;

    eX_Shell_Prompt();
}

static void eX_Shell_Character(char c) {
    if (eX_ShellLength >= eX_SHELL_BUFFER_SIZE - 1) {
        return;
    }

    eX_ShellBuffer[eX_ShellLength++] = c;

    eX_PrintChar(c);
}

void eX_Shell_Update(void) {
    int c;

    while ((c = eX_Keyboard_GetChar()) != -1) {
        if (c == '\n') {
            eX_Shell_Enter();
        } else if (c == '\b') {
            eX_Shell_Backspace();
        } else if (c >= 32 && c <= 126) {
            eX_Shell_Character((char)c);
        }
    }
}

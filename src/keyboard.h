#ifndef EYAX_KEYBOARD_H
#define EYAX_KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

void eX_Keyboard_Init(void);
int eX_Keyboard_GetChar(void);
uint64_t eX_Keyboard_GetIRQCount(void);
uint8_t eX_Keyboard_GetControllerStatus(void);
uint8_t eX_Keyboard_GetPICMask(void);
uint8_t eX_Keyboard_GetPICIRR(void);
uint64_t eX_Keyboard_GetIDTHandler(void);
void eX_Keyboard_Poll(void);

#endif

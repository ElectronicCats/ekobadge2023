#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "HAL.h"
#include "app_mesh_config.h"
#include "display.h"

#define BUTTON_BACK 0x01
#define BUTTON_UP 0x02
#define BUTTON_DOWN 0x04
#define BUTTON_SELECT 0x08

void Keyboard_Print_Button(uint8_t keys);
void Keyboard_Print_Layer(uint8_t layer);
void Keyboard_Scan_Callback(uint8_t keys);

#endif
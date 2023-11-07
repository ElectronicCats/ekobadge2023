#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "HAL.h"
#include "app_mesh_config.h"
#include "display.h"

#define BTN1 0x01
#define BTN2 0x02
#define BTN3 0x04
#define BTN4 0x08

#define BUTTON_BACK BTN3
#define BUTTON_UP BTN1
#define BUTTON_DOWN BTN2
#define BUTTON_SELECT BTN4

void Keyboard_Print_Button(uint8_t keys);
void Keyboard_Print_Layer(uint8_t layer);
void Keyboard_Scan_Callback(uint8_t keys);
void Update_Previous_Layer();
void Main_Menu();
void Neopixels_Menu();
void Neopixel_Options();

#endif
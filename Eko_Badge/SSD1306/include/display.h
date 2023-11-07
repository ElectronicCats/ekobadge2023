#ifndef DISPLAY_H
#define DISPLAY_H

#include "app_mesh_config.h"
// #include "ch32v003fun.h"
#include <stdio.h>
#include "ssd1306_i2c.h"
#include "ssd1306.h"
#include "bomb.h"
#include "HAL.h"
#include "levels.h"

#define I2C_TIMEOUT 1000
#define TxAdderss 0x02

// Display events
#define DISPLAY_TEST_EVENT 0x0001
#define DISPLAY_CLEAR_EVENT 0x0002
#define DISPLAY_SHOW_VMENU_EVENT 0x0004
#define DISPLAY_SHOW_HMENU_EVENT 0x0008
#define DISPLAY_SHOW_LOGO_EVENT 0x0010
#define DISPLAY_SEND_SECRET_EVENT 0x0020
#define DISPLAY_RECEIVE_SECRET_EVENT 0x0040

#define NO_DELAY 0
#define SHOW_LOGO_DELAY 1000
#define SHOW_MENU_DELAY 1200
#define SEND_SECRET_DELAY 2000

#define WHITE 1
#define BLACK 0
#define VERTICAL_MENU 0
#define HORIZONTAL_MENU 1

enum DisplayLayer
{
    LAYER_MAIN = 0,
    LAYER_NEOPIXELS_MENU,
    LAYER_NEOPIXEL_1,
    LAYER_NEOPIXEL_2,
    LAYER_NEOPIXEL_3,
    LAYER_NEOPIXELS_RAINBOW,
    LAYER_I2C_SCANNER,
    LAYER_FRIENDS_MENU,
    LAYER_FRIENDS_SEARCH,
    LAYER_FRIENDS_HELP,
    LAYER_FRIEND_FOUND,
    LAYER_FRIENDS_MENU_UNLOCKED,
    LAYER_PROPERTIES,
    LAYER_CREDITS,
    LAYER_SENSOR_MENU,
    LAYER_SENSOR_QUESTION,
    LAYER_SENSOR_HELP,
    LAYER_WRONG_YEAR,
    LAYER_CORRECT_YEAR,
    LAYER_SENSOR_MENU_UNLOCKED,
    LAYER_FINISH_LEVEL_1,
    LAYER_GET_50_FRIENDS,
    LAYER_FINISH_LEVEL_2,
    LAYER_BANNER_LEVEL_2,
    LAYER_BANNER_LEVEL_3,
    LAYER_WELCOME_LEVEL_2,
    LAYER_WELCOME_LEVEL_3,
    LAYER_SECRET_BANNER,
    LAYER_LEVELS_MENU,
    LAYER_LEVEL_1_OPTONS,
    LAYER_LEVEL_2_OPTONS,
    LAYER_LEVEL_3_OPTONS,
    LAYER_LEVEL_1_HELP,
    LAYER_LEVEL_2_HELP,
    LAYER_LEVEL_3_HELP
};

// Horizontal menu options
#define OK 0
#define CANCEL 1

// Main menu options
#define MAIN_NEOPIXELS_MENU 0
#define MAIN_LEVELS_MENU 1
#define MAIN_FRIENDS_MENU 2
#define MAIN_PROPERTIES_MENU 3
#define MAIN_SENSOR_MENU 4
#define MAIN_SECRET_BANNER 5

// Levels menu options
#define LEVEL_1 0
#define LEVEL_2 1
#define LEVEL_3 2

// Levels options
#define LEVELS_HELP 0

// Neopixels menu options
#define NEOPIXEL_1 0
#define NEOPIXEL_2 1
#define NEOPIXEL_3 2
#define NEOPIXELS_RAINBOW 3

// Neopixel options
#define NEOPIXEL_SET_RED 0
#define NEOPIXEL_SET_GREEN 1
#define NEOPIXEL_SET_BLUE 2
#define NEOPIXEL_ON 3
#define NEOPIXEL_OFF 4

// Friends menu options
#define FRIENDS_COUNTER 0
#define FRIENDS_SEARCH 1
#define FRIENDS_HELP 2

// Properties menu options
#define PROPERTIES_CREDITS 0

// Sensor menu options
#define SENSOR_QUESTION 0
#define SENSOR_HELP 1

/* Global variables */
extern tmosTaskID displayTaskID;
extern uint8_t selectedOption;
extern uint8_t previousLayer;
extern uint8_t currentLayer;
extern uint8_t optionsSize;
extern uint8_t bannerSize;
extern uint8_t menuOrientation;
extern uint8_t macAddress[6];

/* Function protopotypes */
tmosEvents Display_ProcessEvent(tmosTaskID task_id, tmosEvents events);
void IIC_Init(u32 bound, u16 address);
void Scan_I2C_Devices();
void Display_Test();
void Display_Init();
void Display_Clear();
void Display_Show_Logo();

// Vertical menu functions
void Display_Update_Menu();
void Display_Update_VMenu();
void Display_Show_VMenu();
char **Display_Update_VMenu_Options();

// Horizontal menu functions
void Display_Update_HMenu();
void Display_Show_HMenu();
char **Display_Update_HMenu_Banner();
char **Display_Update_HMenu_Options();

void Display_Update_Friends_Counter();

#endif // DISPLAY_H

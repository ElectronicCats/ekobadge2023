#ifndef DISPLAY_H
#define DISPLAY_H

#include "app_mesh_config.h"
#include <stdio.h>
#include "ssd1306_i2c.h"
#include "ssd1306.h"
#include "bomb.h"
#include "HAL.h"

#define I2C_TIMEOUT 1000
#define TxAdderss 0x02
#define DISPLAY_TEST_EVENT 0x0001
#define DISPLAY_CLEAR_EVENT 0x0002
#define DISPLAY_SHOW_LOGO_EVENT 0x0008
#define DISPLAY_SHOW_HELLO_WORLD_EVENT 0x0010
#define DISPLAY_SEND_CHAR_EVENT 0x0020
#define DISPLAY_LISTEN_CHAR_EVENT 0x0040

#define NO_DELAY 0
#define SHOW_LOGO_DELAY 1000

#define WHITE 1
#define BLACK 0

/* Global variables */
extern tmosTaskID displayTaskID;

/* Function protopotypes */
tmosEvents Display_ProcessEvent(tmosTaskID task_id, tmosEvents events);
void IIC_Init(u32 bound, u16 address);
void Scan_I2C_Devices();
void Display_Test();
void Display_Init();
void Display_Clear();
void Display_Show_Logo();

#endif // DISPLAY_H

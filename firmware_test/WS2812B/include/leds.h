#ifndef LEDS_H
#define LEDS_H

#include "ws2812b_dma_spi_led_driver.h"
#include "color_utilities.h"
#include "HAL.h"
#include "app_mesh_config.h"

#define NO_DELAY 0
#define RAINBOW_DELAY_MS 100

#define LEDS_RAINBOW_EVENT 0x0001

#define true 1
#define false 0
#define TURN_OFF_DELAY_MS 5

extern tmosTaskID ledsTaskID;

void Leds_Init();
tmosEvents Leds_ProcessEvent(tmosTaskID taks_id, tmosEvents events);
uint32_t WS2812BLEDCallback(int led_number); // Callback that you must implement.

void Leds_On();
void Leds_Off();
void Leds_Set_Rainbow();

void Led1_On();
void Led1_Off();
void Led1_Set_Red();
void Led1_Set_Green();
void Led1_Set_Blue();

void Led2_On();
void Led2_Off();
void Led2_Set_Red();
void Led2_Set_Green();
void Led2_Set_Blue();

void Led3_On();
void Led3_Off();
void Led3_Set_Red();
void Led3_Set_Green();
void Led3_Set_Blue();

#endif
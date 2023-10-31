#include "Keyboard.h"

#define WS2812DMA_IMPLEMENTATION
// #define WSRBG //For WS2816C's.
#define WSGRB // For SK6805-EC15
#include "leds.h"

void Keyboard_Print_Button(uint8_t keys)
{
    switch (keys)
    {
    case BUTTON_BACK:
        APP_DBG("Button pressed: BACK");
        break;
    case BUTTON_UP:
        APP_DBG("Button pressed: UP");
        break;
    case BUTTON_DOWN:
        APP_DBG("Button pressed: DOWN");
        break;
    case BUTTON_SELECT:
        APP_DBG("Button pressed: SELECT");
        break;
    default:
        break;
    }
}

void Keyboard_Scan_Callback(uint8_t keys)
{
    Keyboard_Print_Button(keys);

    switch (keys)
    {
    case BUTTON_BACK:
        tmos_start_task(ledsTaskID, LEDS_RAINBOW_EVENT, MS1_TO_SYSTEM_TIME(RAINBOW_DELAY_MS));
        break;
    case BUTTON_UP:
        tmos_stop_task(ledsTaskID, LEDS_RAINBOW_EVENT);
        Leds_Off();
        break;
    case BUTTON_DOWN:
        tmos_start_task(displayTaskID, DISPLAY_SEND_CHAR_EVENT, MS1_TO_SYSTEM_TIME(0));
        tmos_start_task(displayTaskID, DISPLAY_LISTEN_CHAR_EVENT, MS1_TO_SYSTEM_TIME(0));
        break;
    case BUTTON_SELECT:
        Display_Test();
        break;
    default:
        break;
    }
}

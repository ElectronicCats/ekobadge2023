#include "leds.h"
#include "debug.h"

#define LED_OFF 0
#define LED1_RED 0x00007F
#define LED1_GREEN 0x7F007F00
#define LED1_BLUE 0x007F0000
#define LED2_RED 0x010002E6
#define LED2_GREEN 0x00002F00
#define LED2_BLUE 0x007F0000
#define LED3_RED 0x007F005D
#define LED3_GREEN 0x00007F00
#define LED3_BLUE 0x000015E2
#define DUMMY_COLOR 0x00007F00

tmosTaskID ledsTaskID;
int leds_number = 0;
int turn_off_leds;
int rainbow_mode;
uint32_t led1_color, led1_color_backup;
uint32_t led2_color, led2_color_backup;
uint32_t led3_color, led3_color_backup;

uint32_t random_color(int led_number);

void Leds_Init()
{
    ledsTaskID = TMOS_ProcessEventRegister(Leds_ProcessEvent);
    leds_number = 3;
    turn_off_leds = 0;
    rainbow_mode = 0;
    led1_color = LED_OFF;
    led2_color = LED_OFF;
    led3_color = LED_OFF;

    WS2812BDMAInit();
    Delay_Ms(10); // Give some time to turn leds on
    Leds_Off();
    // tmos_start_task(ledsTaskID, LEDS_RAINBOW_EVENT, MS1_TO_SYSTEM_TIME(NO_DELAY));
}

tmosEvents Leds_ProcessEvent(tmosTaskID taks_id, tmosEvents events)
{
    if (events & LEDS_RAINBOW_EVENT)
    {
        Leds_Set_Rainbow();
        tmos_start_task(ledsTaskID, LEDS_RAINBOW_EVENT, MS1_TO_SYSTEM_TIME(RAINBOW_DELAY_MS));
        return events ^ LEDS_RAINBOW_EVENT;
    }
}

uint32_t WS2812BLEDCallback(int led_number)
{
    if (turn_off_leds)
    {
        return 0;
    }

    if (rainbow_mode)
    {
        return random_color(led_number);
    }

    if (led_number == 0)
    {
        return led1_color;
    }
    else if (led_number == 1)
    {
        return led2_color;
    }
    else if (led_number == 2)
    {
        return led3_color;
    }
    else
    {
        return DUMMY_COLOR;
    }
}

void Leds_On()
{
    turn_off_leds = false;
    WS2812BDMAStart(leds_number);
}

void Leds_Off()
{
    turn_off_leds = true;
    rainbow_mode = false;
    tmos_stop_task(ledsTaskID, LEDS_RAINBOW_EVENT);
    WS2812BDMAStart(leds_number);
}

uint32_t random_color(int led_number)
{
    uint8_t tween = 0;
    static uint8_t index = 0;
    index += 3;

    uint8_t rsbase = sintable[index];
    uint8_t rs = rsbase >> 3;
    uint32_t fire = ((huetable[(rs + 190) & 0xff] >> 1) << 16) | (huetable[(rs + 30) & 0xff]) | ((huetable[(rs + 0)] >> 1) << 8);
    uint32_t ice = 0x7f0000 | ((rsbase >> 1) << 8) | ((rsbase >> 1));

    // Because this chip doesn't natively support multiplies, we are going to avoid tweening of 1..254.
    return TweenHexColors(fire, ice, ((tween + led_number) > 0) ? 255 : 0); // Where "tween" is a value from 0 ... 255
}

void Leds_Set_Rainbow()
{
    rainbow_mode = 1;
    leds_number = 3;
    Leds_On();
}

void Led1_On()
{
    led1_color = led1_color_backup;
    Leds_Off();
    Delay_Ms(TURN_OFF_DELAY_MS);
    Leds_On();
}

void Led1_Off()
{
    led1_color_backup = led1_color;
    led1_color = LED_OFF;
    Leds_On();
}

void Led1_Set_Red()
{
    led1_color = LED1_RED;
    led1_color_backup = led1_color;
    Led1_On();
}

void Led1_Set_Green()
{
    Led1_Off();
    led1_color = LED1_GREEN;
    led1_color_backup = led1_color;
    Led1_On();
}

void Led1_Set_Blue()
{
    Led1_Off();
    led1_color = LED1_BLUE;
    led1_color_backup = led1_color;
    Led1_On();
}

void Led2_On()
{
    led2_color = led2_color_backup;
    Leds_Off();
    Delay_Ms(TURN_OFF_DELAY_MS);
    Leds_On();
}

void Led2_Off()
{
    led2_color_backup = led2_color;
    led2_color = LED_OFF;
    Leds_On();
}

void Led2_Set_Red()
{
    led2_color = LED2_RED;
    led2_color_backup = led2_color;
    Led2_On();
}

void Led2_Set_Green()
{
    led2_color = LED2_GREEN;
    led2_color_backup = led2_color;
    Led2_On();
}

void Led2_Set_Blue()
{
    led2_color = LED2_BLUE;
    led2_color_backup = led2_color;
    Led2_On();
}

void Led3_On()
{
    led3_color = led3_color_backup;
    Leds_Off();
    Delay_Ms(TURN_OFF_DELAY_MS);

    if (led3_color == LED3_RED)
    {
        Led3_Set_Red();
    }
    else if (led3_color == LED3_GREEN)
    {
        Led3_Set_Green();
    }
    else if (led3_color == LED3_BLUE)
    {
        Led3_Set_Blue();
    }
    else
    {
        Leds_On();
    }
}

void Led3_Off()
{
    led3_color_backup = led3_color;
    led3_color = LED_OFF;
    Leds_On();
}

void Led3_Set_Red()
{
    leds_number = 3;
    led3_color = LED3_RED;
    Leds_Off();
    Delay_Ms(TURN_OFF_DELAY_MS);
    Leds_On();
}

void Led3_Set_Green()
{
    leds_number = 3;
    led3_color = LED3_GREEN;
    Leds_Off();
    Delay_Ms(TURN_OFF_DELAY_MS);
    Leds_On();
}

void Led3_Set_Blue()
{
    leds_number = 4;
    led3_color = LED3_BLUE;
    Leds_Off();
    Delay_Ms(TURN_OFF_DELAY_MS);
    Leds_On();
}

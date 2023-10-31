#include "tests.h"

void test_leds()
{
    printf("Test LED 1\r\n");
    Led1_Set_Red();
    Delay_Ms(500);
    Led1_Set_Green();
    Delay_Ms(500);
    Led1_Set_Blue();
    Delay_Ms(500);
    Led1_Off();

    printf("Test LED 2\r\n");
    Led2_Set_Red();
    Delay_Ms(500);
    Led2_Set_Green();
    Delay_Ms(500);
    Led2_Set_Blue();
    Delay_Ms(500);
    Led2_Off();

    printf("Test LED 3\r\n");
    Led3_Set_Red();
    Delay_Ms(500);
    led3_set_green();
    Delay_Ms(500);
    Led3_Set_Blue();
    Delay_Ms(500);
    Led3_Off();

    printf("Test LEDs\r\n");
    Led1_Set_Red();
    Led2_Set_Red();
    Led3_Set_Red();
    Delay_Ms(500);

    Led1_Set_Green();
    Led2_Set_Green();
    led3_set_green();
    Delay_Ms(500);

    Led1_Set_Blue();
    Led2_Set_Blue();
    Led3_Set_Blue();
    Delay_Ms(500);

    Leds_Off();
    Delay_Ms(500);
    Leds_On();
    Delay_Ms(500);
    Leds_Off();
    Delay_Ms(500);
    Leds_On();
    Delay_Ms(500);
    Leds_Off();
    Delay_Ms(500);

    printf("Test rainbow effect\r\n");
    for (int i = 0; i < 100; i++)
    {
        static uint8_t counter = 1;
        APP_DBG("Counter: %d", counter);
        counter++;
        Leds_Set_Rainbow(); // Change the colors every time its called
        Delay_Ms(100);
    }

    printf("Test finished\r\n");
    Leds_Off();
}
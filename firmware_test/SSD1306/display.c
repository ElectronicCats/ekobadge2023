#include "display.h"
#include "stdint.h"

tmosTaskID displayTaskID;

tmosEvents Display_ProcessEvent(tmosTaskID task_id, tmosEvents events)
{
    if (events & DISPLAY_TEST_EVENT)
    {
        APP_DBG("Display test");
        Display_Test();
        return events ^ DISPLAY_TEST_EVENT;
    }

    if (events & DISPLAY_CLEAR_EVENT)
    {
        Display_Clear();
        return events ^ DISPLAY_CLEAR_EVENT;
    }

    if (events & DISPLAY_SHOW_LOGO_EVENT)
    {
        Display_Show_Logo();
        tmos_start_task(displayTaskID, DISPLAY_CLEAR_EVENT, MS1_TO_SYSTEM_TIME(SHOW_LOGO_DELAY));
        return events ^ DISPLAY_SHOW_LOGO_EVENT;
    }

    if (events & DISPLAY_SHOW_HELLO_WORLD_EVENT)
    {
        ssd1306_setbuf(0);
        ssd1306_drawstr(0, 0, "Hello World!", WHITE);
        ssd1306_refresh();
        tmos_stop_task(displayTaskID, DISPLAY_SEND_CHAR_EVENT);
        tmos_stop_task(displayTaskID, DISPLAY_LISTEN_CHAR_EVENT);
        return events ^ DISPLAY_SHOW_HELLO_WORLD_EVENT;
    }

    if (events & DISPLAY_SEND_CHAR_EVENT)
    {
        printf("x\r\n");
        tmos_start_task(displayTaskID, DISPLAY_SEND_CHAR_EVENT, MS1_TO_SYSTEM_TIME(100));
        return events ^ DISPLAY_SEND_CHAR_EVENT;
    }

    if (events & DISPLAY_LISTEN_CHAR_EVENT)
    {
        // printf("Listening...\r\n");
        if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET)
        {
            // Store incoming data into a variable
            uint8_t data = USART_ReceiveData(USART1);
            printf("USART1 Receive Data: %c\r\n", data);

            // Print hello world if 'x' is received
            if (data == 'x')
            {
                tmos_start_task(displayTaskID, DISPLAY_SHOW_HELLO_WORLD_EVENT, MS1_TO_SYSTEM_TIME(NO_DELAY));
            }
        }
        tmos_start_task(displayTaskID, DISPLAY_LISTEN_CHAR_EVENT, MS1_TO_SYSTEM_TIME(NO_DELAY));
        return events ^ DISPLAY_LISTEN_CHAR_EVENT;
    }
}

/*********************************************************************
 * @fn      IIC_Init
 *
 * @brief   Initializes the IIC peripheral.
 *
 * @return  none
 */
void IIC_Init(u32 bound, u16 address)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    I2C_InitTypeDef I2C_InitTSturcture = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    // GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

    // SCL
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // SDA
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    I2C_InitTSturcture.I2C_ClockSpeed = bound;
    I2C_InitTSturcture.I2C_Mode = I2C_Mode_I2C;
    I2C_InitTSturcture.I2C_DutyCycle = I2C_DutyCycle_16_9;
    I2C_InitTSturcture.I2C_OwnAddress1 = address;
    I2C_InitTSturcture.I2C_Ack = I2C_Ack_Enable;
    I2C_InitTSturcture.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C2, &I2C_InitTSturcture);

    I2C_Cmd(I2C2, ENABLE);
    I2C_AcknowledgeConfig(I2C2, ENABLE);
}

void Scan_I2C_Devices()
{
    APP_DBG("Scanning I2C bus...");
    uint8_t found = 0;

    for (uint8_t address = 1; address < 128; address++)
    {
        uint32_t timeout = I2C_TIMEOUT;
        while (I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY))
        {
            if (--timeout == 0)
            {
                APP_DBG("I2C bus is busy");
                return;
            }
        }
        I2C_GenerateSTART(I2C2, ENABLE);
        timeout = I2C_TIMEOUT;
        while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT))
        {
            if (--timeout == 0)
            {
                APP_DBG("Failed to generate start condition");
                return;
            }
        }
        I2C_Send7bitAddress(I2C2, address << 1, I2C_Direction_Transmitter);
        timeout = I2C_TIMEOUT;
        while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        {
            if (--timeout == 0)
            {
                // APP_DBG("No response from address 0x%02X", address);
                break;
            }
        }
        I2C_GenerateSTOP(I2C2, ENABLE);
        if (timeout > 0)
        {
            APP_DBG("Found device at address 0x%02X", address);
            found++;
            break;
        }
    }

    if (found == 0)
    {
        APP_DBG("No I2C devices found");
    }
}

void Display_Test()
{
    // APP_DBG("Looping on test modes...");
    for (uint8_t mode = 0; mode < 8; mode++)
    {
        // clear buffer for next mode
        ssd1306_setbuf(0);

        switch (mode)
        {
        case 0:
            APP_DBG("buffer fill with binary");
            for (int i = 0; i < sizeof(ssd1306_buffer); i++)
                ssd1306_buffer[i] = i;
            break;

        case 1:
            APP_DBG("pixel plots");
            for (int i = 0; i < SSD1306_W; i++)
            {
                ssd1306_drawPixel(i, i / (SSD1306_W / SSD1306_H), 1);
                ssd1306_drawPixel(i, SSD1306_H - 1 - (i / (SSD1306_W / SSD1306_H)), 1);
            }
            break;

        case 2:
        {
            APP_DBG("Line plots");
            uint8_t y = 0;
            for (uint8_t x = 0; x < SSD1306_W; x += 16)
            {
                ssd1306_drawLine(x, 0, SSD1306_W, y, 1);
                ssd1306_drawLine(SSD1306_W - x, SSD1306_H, 0, SSD1306_H - y, 1);
                y += SSD1306_H / 8;
            }
        }
        break;

        case 3:
            APP_DBG("Circles empty and filled");
            for (uint8_t x = 0; x < SSD1306_W; x += 16)
                if (x < 64)
                    ssd1306_drawCircle(x, SSD1306_H / 2, 15, 1);
                else
                    ssd1306_fillCircle(x, SSD1306_H / 2, 15, 1);
            break;
        case 4:
            APP_DBG("Image");
            ssd1306_drawImage(0, 0, bomb_i_stripped, 32, 32, 0);
            break;
        case 5:
            APP_DBG("Unscaled Text");
            ssd1306_drawstr(0, 0, "This is a test", 1);
            ssd1306_drawstr(0, 8, "of the emergency", 1);
            ssd1306_drawstr(0, 16, "broadcasting", 1);
            ssd1306_drawstr(0, 24, "system.", 1);
            if (SSD1306_H > 32)
            {
                ssd1306_drawstr(0, 32, "Lorem ipsum", 1);
                ssd1306_drawstr(0, 40, "dolor sit amet,", 1);
                ssd1306_drawstr(0, 48, "consectetur", 1);
                ssd1306_drawstr(0, 56, "adipiscing", 1);
            }
            ssd1306_xorrect(SSD1306_W / 2, 0, SSD1306_W / 2, SSD1306_W);
            break;

        case 6:
            APP_DBG("Scaled Text 1, 2");
            ssd1306_drawstr_sz(0, 0, "sz 8x8", 1, fontsize_8x8);
            ssd1306_drawstr_sz(0, 16, "16x16", 1, fontsize_16x16);
            break;

        case 7:
            APP_DBG("Scaled Text 4");
            ssd1306_drawstr_sz(0, 0, "32x32", 1, fontsize_32x32);
            break;

        case 8:
            APP_DBG("Scaled Text 8");
            ssd1306_drawstr_sz(0, 0, "64", 1, fontsize_64x64);
            break;

        default:
            break;
        }

        ssd1306_refresh();
        Delay_Ms(200);
    }

    Display_Clear();
}

void Display_Init(void)
{
    displayTaskID = TMOS_ProcessEventRegister(Display_ProcessEvent);
    IIC_Init(80000, TxAdderss);

    // 48MHz internal clock
    SystemInit();
    // init i2c and oled
    Delay_Ms(100); // give OLED some more time
    Scan_I2C_Devices();
    APP_DBG("Initializing i2c oled...");
    ssd1306_init();
    APP_DBG("Done.");

    // tmos_start_task(displayTaskID, DISPLAY_TEST_EVENT, MS1_TO_SYSTEM_TIME(100));
    // tmos_start_task(displayTaskID, DISPLAY_SHOW_LOGO_EVENT, MS1_TO_SYSTEM_TIME(NO_DELAY));
}

void Display_Clear()
{
    ssd1306_setbuf(0);
    ssd1306_refresh();
}

void Display_Show_Logo()
{
    ssd1306_setbuf(0);
    ssd1306_drawstr_sz(0, 16, "EKOPARTY", 1, fontsize_16x16);
    ssd1306_refresh();
}

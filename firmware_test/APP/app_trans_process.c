/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_trans_process.c
 * Author             : WCH
 * Version            : V1.0
 * Date               : 2022/01/18
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/******************************************************************************/
#include "CONFIG.h"
#include "MESH_LIB.h"
#include "app_vendor_model_srv.h"
#include "app.h"
#include "peripheral.h"
#include "HAL.h"
#include "app_trans_process.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*********************************************************************
 * LOCAL FUNCION
 */

/*********************************************************************
 * GLOBAL TYPEDEFS
 */


/*********************************************************************
 * @fn      read_led_state
 *
 * @brief   ��ȡ��ǰ��״̬.
 *
 * @param   led_pin - ����.
 *
 * @return  ��״̬
 */
BOOL read_led_state(uint32_t led_pin)
{
    return (GPIO_ReadOutputDataBit(MSG_GPIO, led_pin) > 0) ? 0 : 1;
}

/*********************************************************************
 * @fn      set_led_state
 *
 * @brief   ���õ�ǰ��״̬.
 *
 * @param   led_pin - ����.
 * @param   on      - ״̬.
 *
 * @return  none
 */
void set_led_state(uint32_t led_pin, BOOL on)
{
    RCC_APB2PeriphClockCmd(MSG_PCENR, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = led_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(MSG_GPIO, &GPIO_InitStructure);

    on ? GPIO_WriteBit(MSG_GPIO, led_pin, Bit_RESET) : GPIO_WriteBit(MSG_GPIO, led_pin, Bit_SET);
}

/*********************************************************************
 * @fn      toggle_led_state
 *
 * @brief   ��ת��ǰ��״̬
 *
 * @param   led_pin - ����.
 *
 * @return  none
 */
void toggle_led_state(uint32_t led_pin)
{
    RCC_APB2PeriphClockCmd(MSG_PCENR, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = led_pin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_WriteBit(MSG_GPIO, led_pin, Bit_SET-GPIO_ReadOutputDataBit(MSG_GPIO, led_pin));
}

/*********************************************************************
 * @fn      app_trans_process
 *
 * @brief   �����յ���trans����
 *
 * @param   pValue      - ����ָ��.
 *          len         - ���ݳ���.
 *          src_Addr    - ������Դ��ַ.
 *          dst_Addr    - ����Ŀ�ĵ�ַ.
 *
 * @return  none
 */
void app_trans_process(uint8_t *pValue, uint8_t len, uint16_t src_Addr, uint16_t dst_Addr)
{
    uint16_t opcode = (pValue[0]<<8)|pValue[1];
    switch( opcode )
    {
        case BLE_MESH_MODEL_OP_GEN_ONOFF_SET:
        {
            set_led_state(LED_PIN, pValue[2]);
            break;
        }

        default:
        {
            break;
        }
    }

}



/******************************** endfile @ main ******************************/

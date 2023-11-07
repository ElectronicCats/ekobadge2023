/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.c
 * Authors            : WCH, Electronic Cats
 * Version            : V1.1
 * Date               : 2023/10/27
 * Description        : This project is based on the WCH BLE Mesh demo and it was
 *                      adapted to work with the Eko Badge board made by Electronic
 *                      Cats for the Eko Party 2023, Argentina.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 * 
 *******************************************************************************/

/******************************************************************************/
/* Header file contains */
#include "CONFIG.h"
#include "MESH_LIB.h"
#include "HAL.h"
#include "app_mesh_config.h"
#include "app.h"
#include "display.h"
#include "flash.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];
#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif

/*********************************************************************
 * @fn      bt_mesh_lib_init
 *
 * @brief   mesh ���ʼ��
 *
 * @return  state
 */
uint8_t bt_mesh_lib_init(void)
{
    uint8_t ret;

    if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE)
    {
        APP_DBG("mesh head file error...");
        while (1)
            ;
    }

    ret = RF_RoleInit();

#if ((CONFIG_BLE_MESH_PROXY) ||   \
     (CONFIG_BLE_MESH_PB_GATT) || \
     (CONFIG_BLE_MESH_OTA))
    ret = GAPRole_PeripheralInit();
#endif /* PROXY || PB-GATT || OTA */

#if (CONFIG_BLE_MESH_PROXY_CLI)
    ret = GAPRole_CentralInit();
#endif /* CONFIG_BLE_MESH_PROXY_CLI */

    MeshTimer_Init();
    MeshDeamon_Init();
    ble_sm_alg_ecc_init();

#if (CONFIG_BLE_MESH_IV_UPDATE_TEST)
    bt_mesh_iv_update_test(TRUE);
#endif
    return ret;
}

/*********************************************************************
 * @fn      Main_Circulation
 *
 * @brief   Main loop
 *
 * @return  none
 */
__attribute__((section(".highcode")))
__attribute__((noinline)) void
Main_Circulation(void)
{
    while (1)
    {
        TMOS_SystemProcess();
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main function
 *
 * @return  none
 */
int main(void)
{
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    APP_DBG("System clock: %d MHz", SystemCoreClock / 1000000);
    APP_DBG("%s", VER_LIB);

    Flash_Init(); // Must be called before any other function
    APP_DBG("Reboot Counter: %d", Flash_Get_Reboot_Counter());
    APP_DBG("Level: %d", Get_Level());
    // Flash_Erase();
    // Flash_Test();

    WCHBLE_Init();
    HAL_Init();
    bt_mesh_lib_init();

    Leds_Init();
    APP_DBG("LEDs setup ready!");
    App_Init();
    Display_Init();

    Main_Circulation();
}

/******************************** endfile @ main ******************************/

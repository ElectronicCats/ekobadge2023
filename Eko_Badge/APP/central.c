/********************************** (C) COPYRIGHT *******************************
 * File Name          : central.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        : The master routine actively scans the surrounding devices,
 *                      connects to the given slave device address, looks for custom
 *                      services and characteristics, and executes read and write commands.
 *                      It needs to be used in conjunction with the slave routine,
 *                      and the slave device address is modified to the routine target address,
 *                      the default is (84:C2:E4:03:02:02)
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "CONFIG.h"
#include "gattprofile.h"
#include "central.h"
#include "app_mesh_config.h"
#include "friend.h"

/*********************************************************************
 * MACROS
 */

// Length of bd addr as a string
#define B_ADDR_STR_LEN 15

/*********************************************************************
 * CONSTANTS
 */
// Maximum number of scan responses
#define DEFAULT_MAX_SCAN_RES 80

// Scan duration in 0.625ms
#define DEFAULT_SCAN_DURATION 2400

// Connection min interval in 1.25ms
#define DEFAULT_MIN_CONNECTION_INTERVAL 20

// Connection max interval in 1.25ms
#define DEFAULT_MAX_CONNECTION_INTERVAL 60

// Connection supervision timeout in 10ms
#define DEFAULT_CONNECTION_TIMEOUT 100

// Discovey mode (limited, general, all)
#define DEFAULT_DISCOVERY_MODE DEVDISC_MODE_ALL

// TRUE to use active scan
#define DEFAULT_DISCOVERY_ACTIVE_SCAN TRUE

// TRUE to use white list during discovery
#define DEFAULT_DISCOVERY_WHITE_LIST FALSE

// TRUE to use high scan duty cycle when creating link
#define DEFAULT_LINK_HIGH_DUTY_CYCLE FALSE

// TRUE to use white list when creating link
#define DEFAULT_LINK_WHITE_LIST FALSE

// Default read RSSI period in 0.625ms
#define DEFAULT_RSSI_PERIOD 2400

// Minimum connection interval (units of 1.25ms)
#define DEFAULT_UPDATE_MIN_CONN_INTERVAL 20

// Maximum connection interval (units of 1.25ms)
#define DEFAULT_UPDATE_MAX_CONN_INTERVAL 100

// Slave latency to use parameter update
#define DEFAULT_UPDATE_SLAVE_LATENCY 0

// Supervision timeout value (units of 10ms)
#define DEFAULT_UPDATE_CONN_TIMEOUT 600

// Default passcode
#define DEFAULT_PASSCODE 0

// Default GAP pairing mode
#define DEFAULT_PAIRING_MODE GAPBOND_PAIRING_MODE_WAIT_FOR_REQ

// Default MITM mode (TRUE to require passcode or OOB when pairing)
#define DEFAULT_MITM_MODE TRUE

// Default bonding mode, TRUE to bond, max bonding 6 devices
#define DEFAULT_BONDING_MODE TRUE

// Default GAP bonding I/O capabilities
#define DEFAULT_IO_CAPABILITIES GAPBOND_IO_CAP_NO_INPUT_NO_OUTPUT

// Default service discovery timer delay in 0.625ms
#define DEFAULT_SVC_DISCOVERY_DELAY 1800

// Default parameter update delay in 0.625ms
#define DEFAULT_PARAM_UPDATE_DELAY 3200

// Default phy update delay in 0.625ms
#define DEFAULT_PHY_UPDATE_DELAY 2400

// Default read or write timer delay in 0.625ms
#define DEFAULT_READ_OR_WRITE_DELAY 1600

// Default write CCCD delay in 0.625ms
#define DEFAULT_WRITE_CCCD_DELAY 1600

// Establish link timeout in 0.625ms
#define ESTABLISH_LINK_TIMEOUT 3200

// Application states
enum
{
    BLE_STATE_IDLE,
    BLE_STATE_CONNECTING,
    BLE_STATE_CONNECTED,
    BLE_STATE_DISCONNECTING
};

// Discovery states
enum
{
    BLE_DISC_STATE_IDLE, // Idle
    BLE_DISC_STATE_SVC,  // Service discovery
    BLE_DISC_STATE_CHAR, // Characteristic discovery
    BLE_DISC_STATE_CCCD  // client characteristic configuration discovery
};
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

// Task ID for internal task/event processing
static uint8_t centralTaskId;

// Number of scan results
static uint8_t centralScanRes;

// Scan result list
static gapScanRec_t centralDevList[DEFAULT_MAX_SCAN_RES];
static uint8_t nearbyDeviceAddress[B_ADDR_LEN];

// Peer device address
// static uint8_t PeerAddrDef[B_ADDR_LEN] = {0x02, 0x02, 0x03, 0xE4, 0xC2, 0x84};
static uint8_t PeerAddrDef[B_ADDR_LEN] = {0x22, 0x3A, 0x88, 0x26, 0x3B, 0x38}; // Not used

// RSSI polling state
static uint8_t centralRssi = TRUE;

// Parameter update state
static uint8_t centralParamUpdate = TRUE;

// Phy update state
static uint8_t centralPhyUpdate = FALSE;

// Connection handle of current connection
static uint16_t centralConnHandle = GAP_CONNHANDLE_INIT;

// Application state
static uint8_t centralState = BLE_STATE_IDLE;

// Discovery state
static uint8_t centralDiscState = BLE_DISC_STATE_IDLE;

// Discovered service start and end handle
static uint16_t centralSvcStartHdl = 0;
static uint16_t centralSvcEndHdl = 0;

// Discovered characteristic handle
static uint16_t centralCharHdl = 0;

// Discovered Client Characteristic Configuration handle
static uint16_t centralCCCDHdl = 0;

// Value to write
static uint8_t centralCharVal = 0x5A;

// Value read/write toggle
static uint8_t centralDoWrite = TRUE;

// GATT read/write procedure state
static uint8_t centralProcedureInProgress = FALSE;

static BOOL centralConnecting = FALSE;
/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void centralProcessGATTMsg(gattMsgEvent_t *pMsg);
static void centralRssiCB(uint16_t connHandle, int8_t rssi);
static void centralEventCB(gapRoleEvent_t *pEvent);
static void centralDisconnect(uint8_t reason);
static void centralHciMTUChangeCB(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets);
static void centralAddFriend(uint16_t connHandle);
static void centralPasscodeCB(uint8_t *deviceAddr, uint16_t connectionHandle,
                              uint8_t uiInputs, uint8_t uiOutputs);
static void centralPairStateCB(uint16_t connHandle, uint8_t state, uint8_t status);
static void central_ProcessTMOSMsg(tmos_event_hdr_t *pMsg);
static void centralGATTDiscoveryEvent(gattMsgEvent_t *pMsg);
static void centralStartDiscovery(void);
static void centralAddDeviceInfo(uint8_t *pAddr, uint8_t addrType, int8_t rssi);
static void centralConnectingChecker(void);
/*********************************************************************
 * PROFILE CALLBACKS
 */

// GAP Role Callbacks
static gapCentralRoleCB_t centralRoleCB = {
    centralRssiCB,        // RSSI callback
    centralEventCB,       // Event callback
    centralHciMTUChangeCB // MTU change callback
};

// Bond Manager Callbacks
static gapBondCBs_t centralBondCB = {
    centralPasscodeCB,
    centralPairStateCB};

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      Central_Init
 *
 * @brief   Initialization function for the Central App Task.
 *          This is called during initialization and should contain
 *          any application specific initialization (ie. hardware
 *          initialization/setup, table initialization, power up
 *          notification).
 *
 * @param   task_id - the ID assigned by TMOS.  This ID should be
 *                    used to send messages and set timers.
 *
 * @return  none
 */
void Central_Init()
{
    centralTaskId = TMOS_ProcessEventRegister(Central_ProcessEvent);

    // Setup GAP
    GAP_SetParamValue(TGAP_DISC_SCAN, DEFAULT_SCAN_DURATION);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MIN, DEFAULT_MIN_CONNECTION_INTERVAL);
    GAP_SetParamValue(TGAP_CONN_EST_INT_MAX, DEFAULT_MAX_CONNECTION_INTERVAL);
    GAP_SetParamValue(TGAP_CONN_EST_SUPERV_TIMEOUT, DEFAULT_CONNECTION_TIMEOUT);

    // Setup the GAP Bond Manager
    {
        uint32_t passkey = DEFAULT_PASSCODE;
        uint8_t pairMode = DEFAULT_PAIRING_MODE;
        uint8_t mitm = DEFAULT_MITM_MODE;
        uint8_t ioCap = DEFAULT_IO_CAPABILITIES;
        uint8_t bonding = DEFAULT_BONDING_MODE;

        GAPBondMgr_SetParameter(GAPBOND_CENT_DEFAULT_PASSCODE, sizeof(uint32_t), &passkey);
        GAPBondMgr_SetParameter(GAPBOND_CENT_PAIRING_MODE, sizeof(uint8_t), &pairMode);
        GAPBondMgr_SetParameter(GAPBOND_CENT_MITM_PROTECTION, sizeof(uint8_t), &mitm);
        GAPBondMgr_SetParameter(GAPBOND_CENT_IO_CAPABILITIES, sizeof(uint8_t), &ioCap);
        GAPBondMgr_SetParameter(GAPBOND_CENT_BONDING_ENABLED, sizeof(uint8_t), &bonding);
    }
    // Initialize GATT Client
    GATT_InitClient();
    // Register to receive incoming ATT Indications/Notifications
    GATT_RegisterForInd(centralTaskId);
    // Setup a delayed profile startup
    tmos_set_event(centralTaskId, START_DEVICE_EVT);
    tmos_start_task(centralTaskId, CONNECTING_CHECKER_EVT, MS1_TO_SYSTEM_TIME(1000));
}

/*********************************************************************
 * @fn      Central_ProcessEvent
 *
 * @brief   Central Application Task event processor.  This function
 *          is called to process all events for the task.  Events
 *          include timers, messages and any other user defined events.
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
uint16_t Central_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if (events & SYS_EVENT_MSG)
    {
        uint8_t *pMsg;

        if ((pMsg = tmos_msg_receive(centralTaskId)) != NULL)
        {
            central_ProcessTMOSMsg((tmos_event_hdr_t *)pMsg);
            // Release the TMOS message
            tmos_msg_deallocate(pMsg);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }

    if (events & START_DEVICE_EVT)
    {
        // Start the Device
        GAPRole_CentralStartDevice(centralTaskId, &centralBondCB, &centralRoleCB);
        return (events ^ START_DEVICE_EVT);
    }

    if (events & ESTABLISH_LINK_TIMEOUT_EVT)
    {
        GAPRole_TerminateLink(INVALID_CONNHANDLE);
        return (events ^ ESTABLISH_LINK_TIMEOUT_EVT);
    }

    if (events & START_SVC_DISCOVERY_EVT)
    {
        // start service discovery
        centralStartDiscovery();
        return (events ^ START_SVC_DISCOVERY_EVT);
    }

    if (events & START_PARAM_UPDATE_EVT)
    {
        // start connect parameter update
        GAPRole_UpdateLink(centralConnHandle,
                           DEFAULT_UPDATE_MIN_CONN_INTERVAL,
                           DEFAULT_UPDATE_MAX_CONN_INTERVAL,
                           DEFAULT_UPDATE_SLAVE_LATENCY,
                           DEFAULT_UPDATE_CONN_TIMEOUT);
        return (events ^ START_PARAM_UPDATE_EVT);
    }

    if (events & START_PHY_UPDATE_EVT)
    {
        // start phy update
        APP_DBG("PHY Update %x...", GAPRole_UpdatePHY(centralConnHandle, 0, GAP_PHY_BIT_LE_2M,
                                                      GAP_PHY_BIT_LE_2M, 0));

        return (events ^ START_PHY_UPDATE_EVT);
    }

    if (events & START_READ_OR_WRITE_EVT)
    {
        if (centralProcedureInProgress == FALSE)
        {
            if (centralDoWrite)
            {
                // Do a write
                attWriteReq_t req;

                req.cmd = FALSE;
                req.sig = FALSE;
                req.handle = centralCharHdl;
                req.len = 1;
                req.pValue = GATT_bm_alloc(centralConnHandle, ATT_WRITE_REQ, req.len, NULL, 0);
                if (req.pValue != NULL)
                {
                    *req.pValue = centralCharVal;

                    if (GATT_WriteCharValue(centralConnHandle, &req, centralTaskId) == SUCCESS)
                    {
                        centralProcedureInProgress = TRUE;
                        centralDoWrite = !centralDoWrite;
                        tmos_start_task(centralTaskId, START_READ_OR_WRITE_EVT, DEFAULT_READ_OR_WRITE_DELAY);
                    }
                    else
                    {
                        GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
                    }
                }
            }
            else
            {
                // Do a read
                attReadReq_t req;

                req.handle = centralCharHdl;
                if (GATT_ReadCharValue(centralConnHandle, &req, centralTaskId) == SUCCESS)
                {
                    centralProcedureInProgress = TRUE;
                    centralDoWrite = !centralDoWrite;
                }
            }
        }
        return (events ^ START_READ_OR_WRITE_EVT);
    }

    if (events & START_WRITE_CCCD_EVT)
    {
        if (centralProcedureInProgress == FALSE)
        {
            // Do a write
            attWriteReq_t req;

            req.cmd = FALSE;
            req.sig = FALSE;
            req.handle = centralCCCDHdl;
            req.len = 2;
            req.pValue = GATT_bm_alloc(centralConnHandle, ATT_WRITE_REQ, req.len, NULL, 0);
            if (req.pValue != NULL)
            {
                req.pValue[0] = 1;
                req.pValue[1] = 0;

                if (GATT_WriteCharValue(centralConnHandle, &req, centralTaskId) == SUCCESS)
                {
                    centralProcedureInProgress = TRUE;
                }
                else
                {
                    GATT_bm_free((gattMsg_t *)&req, ATT_WRITE_REQ);
                }
            }
        }
        return (events ^ START_WRITE_CCCD_EVT);
    }

    if (events & START_READ_RSSI_EVT)
    {
        GAPRole_ReadRssiCmd(centralConnHandle);
        tmos_start_task(centralTaskId, START_READ_RSSI_EVT, DEFAULT_RSSI_PERIOD);
        return (events ^ START_READ_RSSI_EVT);
    }

    if (events & CONNECTING_CHECKER_EVT)
    {
        centralConnectingChecker();
        tmos_start_task(centralTaskId, CONNECTING_CHECKER_EVT, MS1_TO_SYSTEM_TIME(1000));
        return (events ^ CONNECTING_CHECKER_EVT);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      central_ProcessTMOSMsg
 *
 * @brief   Process an incoming task message.
 *
 * @param   pMsg - message to process
 *
 * @return  none
 */
static void central_ProcessTMOSMsg(tmos_event_hdr_t *pMsg)
{
    switch (pMsg->event)
    {
    case GATT_MSG_EVENT:
        centralProcessGATTMsg((gattMsgEvent_t *)pMsg);
        break;
    }
}

/*********************************************************************
 * @fn      centralProcessGATTMsg
 *
 * @brief   Process GATT messages
 *
 * @return  none
 */
static void centralProcessGATTMsg(gattMsgEvent_t *pMsg)
{
    if (centralState != BLE_STATE_CONNECTED)
    {
        // In case a GATT message came after a connection has dropped,
        // ignore the message
        GATT_bm_free(&pMsg->msg, pMsg->method);
        return;
    }

    if ((pMsg->method == ATT_EXCHANGE_MTU_RSP) ||
        ((pMsg->method == ATT_ERROR_RSP) &&
         (pMsg->msg.errorRsp.reqOpcode == ATT_EXCHANGE_MTU_REQ)))
    {
        if (pMsg->method == ATT_ERROR_RSP)
        {
            uint8_t status = pMsg->msg.errorRsp.errCode;

            APP_DBG("Exchange MTU Error: %x", status);
        }
        centralProcedureInProgress = FALSE;
    }

    if (pMsg->method == ATT_MTU_UPDATED_EVENT)
    {
        APP_DBG("MTU: %d", pMsg->msg.mtuEvt.MTU);
    }

    if ((pMsg->method == ATT_READ_RSP) ||
        ((pMsg->method == ATT_ERROR_RSP) &&
         (pMsg->msg.errorRsp.reqOpcode == ATT_READ_REQ)))
    {
        if (pMsg->method == ATT_ERROR_RSP)
        {
            uint8_t status = pMsg->msg.errorRsp.errCode;

            APP_DBG("Read Error: %x", status);
        }
        else
        {
            // After a successful read, display the read value
            APP_DBG("Read rsp: %X", *pMsg->msg.readRsp.pValue);

            // Eko badge found
            if (*pMsg->msg.readRsp.pValue == 0x5A)
            {
                // centralAddFriend(pMsg->connHandle);
            }
        }
        centralProcedureInProgress = FALSE;
    }
    else if ((pMsg->method == ATT_WRITE_RSP) ||
             ((pMsg->method == ATT_ERROR_RSP) &&
              (pMsg->msg.errorRsp.reqOpcode == ATT_WRITE_REQ)))
    {
        if (pMsg->method == ATT_ERROR_RSP)
        {
            uint8_t status = pMsg->msg.errorRsp.errCode;

            APP_DBG("Write Error: %x", status);
        }
        else
        {
            // Write success
            APP_DBG("Write success ");
        }

        centralProcedureInProgress = FALSE;
    }
    else if (pMsg->method == ATT_HANDLE_VALUE_NOTI)
    {
        APP_DBG("Receive noti: %x", *pMsg->msg.handleValueNoti.pValue);
    }
    else if (centralDiscState != BLE_DISC_STATE_IDLE)
    {
        centralGATTDiscoveryEvent(pMsg);
    }
    GATT_bm_free(&pMsg->msg, pMsg->method);
}

/*********************************************************************
 * @fn      centralRssiCB
 *
 * @brief   RSSI callback.
 *
 * @param   connHandle - connection handle
 * @param   rssi - RSSI
 *
 * @return  none
 */
static void centralRssiCB(uint16_t connHandle, int8_t rssi)
{
    // APP_DBG("RSSI : -%d dB ", -rssi);
}

/*********************************************************************
 * @fn      centralHciMTUChangeCB
 *
 * @brief   MTU changed callback.
 *
 * @param   maxTxOctets - Max tx octets
 * @param   maxRxOctets - Max rx octets
 *
 * @return  none
 */
static void centralHciMTUChangeCB(uint16_t connHandle, uint16_t maxTxOctets, uint16_t maxRxOctets)
{
    APP_DBG(" HCI data length changed, Tx: %d, Rx: %d", maxTxOctets, maxRxOctets);
    centralProcedureInProgress = TRUE;
}

static void centralAddFriend(uint16_t connHandle)
{
    // Verify if nearby device is already in friends list
    uint16_t j;
    for (j = 0; j < friendsCounter; j++)
    {
        if (tmos_memcmp(nearbyDeviceAddress, friends[j].address, B_ADDR_LEN))
            break;
    }

    // If nearby device is not in friends list, add it
    if (j == friendsCounter)
    {
        for (uint8_t i = 0; i < B_ADDR_LEN; i++)
        {
            friends[friendsCounter].address[i] = nearbyDeviceAddress[i];
        }
        // Increase friend counter and close connection with badge
        friendsCounter++;
        
        if (friendsCounter >= 30)
        {
            Flash_Set_Friends_Counter(friendsCounter);
        }

        // Friends_Add(nearbyDeviceAddress);
    }

    if (friendsCounter == FRIENDS_THRESHOLD_1)
    {
        Display_Finish_Level_1();
        
        if (Get_Level() < 2)
            Set_Level(2);
    }
    else if (friendsCounter == FRIENDS_THRESHOLD_2)
    {
        Display_Get_50_Friends();
    }
    else
    {
        Display_Friend_Found();
    }

    GAPRole_TerminateLink(connHandle);
    centralScanRes = 0;
    GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                  DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                  DEFAULT_DISCOVERY_WHITE_LIST);
}

/*********************************************************************
 * @fn      centralEventCB
 *
 * @brief   Central event callback function.
 *
 * @param   pEvent - pointer to event structure
 *
 * @return  none
 */
static void centralEventCB(gapRoleEvent_t *pEvent)
{
    switch (pEvent->gap.opcode)
    {
    case GAP_DEVICE_INIT_DONE_EVENT:
    {
        APP_DBG("Discovering...");
        GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                      DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                      DEFAULT_DISCOVERY_WHITE_LIST);
    }
    break;

    case GAP_DEVICE_INFO_EVENT:
    {
        if (enableFriendSearch)
        {
            // Add device to list
            centralAddDeviceInfo(pEvent->deviceInfo.addr, pEvent->deviceInfo.addrType, pEvent->deviceInfo.rssi);
        }
    }
    break;

    case GAP_DEVICE_DISCOVERY_EVENT:
    {
        if (!enableFriendSearch)
        {
            centralScanRes = 0;
            GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                          DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                          DEFAULT_DISCOVERY_WHITE_LIST);
            break;
        }

        uint8_t i;
        BOOL found = FALSE;

        // See if peer device has been discovered
        for (i = 0; i < centralScanRes; i++)
        {
            if (tmos_memcmp(PeerAddrDef, centralDevList[i].addr, B_ADDR_LEN))
                break;
        }

        APP_DBG("Scan done... %d devices found.", centralScanRes);

        for (i = 0; i < centralScanRes; i++)
        {
            if (centralDevList[i].rssi >= -40 && centralDevList[i].rssi != 0)
            {
                found = TRUE;
                break;
            }
        }

        if (found)
        {
            APP_DBG("Nearby device... %X:%X:%X:%X:%X:%X \t RSSI: %d",
                    centralDevList[i].addr[5], centralDevList[i].addr[4], centralDevList[i].addr[3],
                    centralDevList[i].addr[2], centralDevList[i].addr[1], centralDevList[i].addr[0],
                    centralDevList[i].rssi);

            // Fill friend address
            for (uint8_t j = 0; j < B_ADDR_LEN; j++)
            {
                nearbyDeviceAddress[j] = centralDevList[i].addr[j];
            }

            // Verify if nearby device is already in friends list
            uint16_t j;
            for (j = 0; j < friendsCounter; j++)
            {
                if (tmos_memcmp(nearbyDeviceAddress, friends[j].address, B_ADDR_LEN))
                    break;
            }

            // If nearby device is not in friends list, try to connect to it
            if (j == friendsCounter)
            {
                APP_DBG("Connecting...");
                centralConnecting = TRUE;
                uint8_t status = GAPRole_CentralEstablishLink(DEFAULT_LINK_HIGH_DUTY_CYCLE,
                                                              DEFAULT_LINK_WHITE_LIST,
                                                              centralDevList[i].addrType,
                                                              centralDevList[i].addr);

                APP_DBG("Status: 0x%X", status);

                // If connection status is 0x11, it means that the connection is already in progress, so stop it
                if (status == 0x11)
                {
                    GAPRole_TerminateLink(INVALID_CONNHANDLE);
                    centralScanRes = 0;
                    GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                                  DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                                  DEFAULT_DISCOVERY_WHITE_LIST);
                }
            }
            else
            {
                APP_DBG("Friend already in list...");
                centralScanRes = 0;
                GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                              DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                              DEFAULT_DISCOVERY_WHITE_LIST);
            }
            APP_DBG("Continue...");
        }
        else
        {
            // APP_DBG("Device not found...");
            centralScanRes = 0;
            GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                          DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                          DEFAULT_DISCOVERY_WHITE_LIST);
            // APP_DBG("Discovering...");
        }
    }
    break;

    case GAP_LINK_ESTABLISHED_EVENT:
    {
        APP_DBG("Link Established...");
        tmos_stop_task(centralTaskId, ESTABLISH_LINK_TIMEOUT_EVT);
        if (pEvent->gap.hdr.status == SUCCESS && enableFriendSearch)
        {
            APP_DBG("Trying to pair...");
            centralState = BLE_STATE_CONNECTED;
            centralConnHandle = pEvent->linkCmpl.connectionHandle;
            centralProcedureInProgress = TRUE;

            // Update MTU
            attExchangeMTUReq_t req = {
                .clientRxMTU = BLE_BUFF_MAX_LEN - 4,
            };

            GATT_ExchangeMTU(centralConnHandle, &req, centralTaskId);

            // Initiate service discovery
            tmos_start_task(centralTaskId, START_SVC_DISCOVERY_EVT, DEFAULT_SVC_DISCOVERY_DELAY);

            // See if initiate connect parameter update
            if (centralParamUpdate)
            {
                tmos_start_task(centralTaskId, START_PARAM_UPDATE_EVT, DEFAULT_PARAM_UPDATE_DELAY);
            }
            // See if initiate PHY update
            if (centralPhyUpdate)
            {
                tmos_start_task(centralTaskId, START_PHY_UPDATE_EVT, DEFAULT_PHY_UPDATE_DELAY);
            }
            // See if start RSSI polling
            if (centralRssi)
            {
                tmos_start_task(centralTaskId, START_READ_RSSI_EVT, DEFAULT_RSSI_PERIOD);
            }

            APP_DBG("Connected!");
            centralConnecting = FALSE;
        }
        else
        {
            APP_DBG("Connect Failed... Reason: %X", pEvent->gap.hdr.status);
            APP_DBG("Discovering...");
            centralScanRes = 0;
            GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                          DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                          DEFAULT_DISCOVERY_WHITE_LIST);
        }
    }
    break;

    case GAP_LINK_TERMINATED_EVENT:
    {
        centralDisconnect(pEvent->linkTerminate.reason);
    }
    break;

    case GAP_LINK_PARAM_UPDATE_EVENT:
    {
        APP_DBG("Param Update...");
        // centralDisconnect(pEvent);
    }
    break;

    case GAP_PHY_UPDATE_EVENT:
    {
        APP_DBG("PHY Update...");
    }
    break;

    case GAP_EXT_ADV_DEVICE_INFO_EVENT:
    {
        // Display device addr
        APP_DBG("Recv ext adv ");
        // Add device to list
        // centralAddDeviceInfo(pEvent->deviceExtAdvInfo.addr, pEvent->deviceExtAdvInfo.addrType, pEvent->deviceInfo.rssi);
    }
    break;

    case GAP_DIRECT_DEVICE_INFO_EVENT:
    {
        // Display device addr
        APP_DBG("Recv direct adv ");
        // Add device to list
        // centralAddDeviceInfo(pEvent->deviceDirectInfo.addr, pEvent->deviceDirectInfo.addrType, pEvent->deviceInfo.rssi);
    }
    break;

    default:
        APP_DBG("Unknown event: %d", pEvent->gap.opcode);
        break;
    }
}

static void centralDisconnect(uint8_t reason)
{
    centralState = BLE_STATE_IDLE;
    centralConnHandle = GAP_CONNHANDLE_INIT;
    centralDiscState = BLE_DISC_STATE_IDLE;
    centralCharHdl = 0;
    centralScanRes = 0;
    centralProcedureInProgress = FALSE;
    tmos_stop_task(centralTaskId, START_READ_RSSI_EVT);
    APP_DBG("Disconnected... Reason: %X", reason);
    APP_DBG("Discovering...");
    GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                  DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                  DEFAULT_DISCOVERY_WHITE_LIST);
}

/*********************************************************************
 * @fn      pairStateCB
 *
 * @brief   Pairing state callback.
 *
 * @return  none
 */
static void centralPairStateCB(uint16_t connHandle, uint8_t state, uint8_t status)
{
    if (state == GAPBOND_PAIRING_STATE_STARTED)
    {
        APP_DBG("Pairing started: %d", status);
    }
    else if (state == GAPBOND_PAIRING_STATE_COMPLETE)
    {
        if (status == SUCCESS)
        {
            APP_DBG("Pairing success");
        }
        else
        {
            APP_DBG("Pairing fail");
        }
    }
    else if (state == GAPBOND_PAIRING_STATE_BONDED)
    {
        if (status == SUCCESS)
        {
            APP_DBG("Bonding success");
        }
    }
    else if (state == GAPBOND_PAIRING_STATE_BOND_SAVED)
    {
        if (status == SUCCESS)
        {
            APP_DBG("Bond save success");
        }
        else
        {
            APP_DBG("Bond save failed: %d", status);
        }
    }
}

/*********************************************************************
 * @fn      centralPasscodeCB
 *
 * @brief   Passcode callback.
 *
 * @return  none
 */
static void centralPasscodeCB(uint8_t *deviceAddr, uint16_t connectionHandle,
                              uint8_t uiInputs, uint8_t uiOutputs)
{
    uint32_t passcode;

    // Create random passcode
    passcode = tmos_rand();
    passcode %= 1000000;
    // Display passcode to user
    if (uiOutputs != 0)
    {
        APP_DBG("Passcode: %06d", (int)passcode);
    }
    // Send passcode response
    GAPBondMgr_PasscodeRsp(connectionHandle, SUCCESS, passcode);
}

/*********************************************************************
 * @fn      centralStartDiscovery
 *
 * @brief   Start service discovery.
 *
 * @return  none
 */
static void centralStartDiscovery(void)
{
    uint8_t uuid[ATT_BT_UUID_SIZE] = {LO_UINT16(SIMPLEPROFILE_SERV_UUID),
                                      HI_UINT16(SIMPLEPROFILE_SERV_UUID)};

    // Initialize cached handles
    centralSvcStartHdl = centralSvcEndHdl = centralCharHdl = 0;

    centralDiscState = BLE_DISC_STATE_SVC;

    // Discovery simple BLE service
    GATT_DiscPrimaryServiceByUUID(centralConnHandle,
                                  uuid,
                                  ATT_BT_UUID_SIZE,
                                  centralTaskId);
}

/*********************************************************************
 * @fn      centralGATTDiscoveryEvent
 *
 * @brief   Process GATT discovery event
 *
 * @return  none
 */
static void centralGATTDiscoveryEvent(gattMsgEvent_t *pMsg)
{
    attReadByTypeReq_t req;

    if (centralDiscState == BLE_DISC_STATE_SVC)
    {
        // Service found, store handles
        if (pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
            pMsg->msg.findByTypeValueRsp.numInfo > 0)
        {
            centralSvcStartHdl = ATT_ATTR_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);
            centralSvcEndHdl = ATT_GRP_END_HANDLE(pMsg->msg.findByTypeValueRsp.pHandlesInfo, 0);

            // Display Profile Service handle range
            APP_DBG("Found Profile Service handle : %X ~ %X ", centralSvcStartHdl, centralSvcEndHdl);
        }
        // If procedure complete
        if ((pMsg->method == ATT_FIND_BY_TYPE_VALUE_RSP &&
             pMsg->hdr.status == bleProcedureComplete) ||
            (pMsg->method == ATT_ERROR_RSP))
        {
            if (centralSvcStartHdl != 0)
            {
                // Discover characteristic
                centralDiscState = BLE_DISC_STATE_CHAR;
                req.startHandle = centralSvcStartHdl;
                req.endHandle = centralSvcEndHdl;
                req.type.len = ATT_BT_UUID_SIZE;
                req.type.uuid[0] = LO_UINT16(SIMPLEPROFILE_CHAR1_UUID);
                req.type.uuid[1] = HI_UINT16(SIMPLEPROFILE_CHAR1_UUID);

                GATT_ReadUsingCharUUID(centralConnHandle, &req, centralTaskId);
            }
        }
    }
    else if (centralDiscState == BLE_DISC_STATE_CHAR)
    {
        // Characteristic found, store handle
        if (pMsg->method == ATT_READ_BY_TYPE_RSP &&
            pMsg->msg.readByTypeRsp.numPairs > 0)
        {
            centralCharHdl = BUILD_UINT16(pMsg->msg.readByTypeRsp.pDataList[0],
                                          pMsg->msg.readByTypeRsp.pDataList[1]);

            // Start do read or write
            tmos_start_task(centralTaskId, START_READ_OR_WRITE_EVT, DEFAULT_READ_OR_WRITE_DELAY);

            // Display Characteristic 1 handle
            APP_DBG("Found Characteristic 1 handle : %X ", centralCharHdl);
        }
        if ((pMsg->method == ATT_READ_BY_TYPE_RSP &&
             pMsg->hdr.status == bleProcedureComplete) ||
            (pMsg->method == ATT_ERROR_RSP))
        {
            // Discover characteristic
            centralDiscState = BLE_DISC_STATE_CCCD;
            req.startHandle = centralSvcStartHdl;
            req.endHandle = centralSvcEndHdl;
            req.type.len = ATT_BT_UUID_SIZE;
            req.type.uuid[0] = LO_UINT16(GATT_CLIENT_CHAR_CFG_UUID);
            req.type.uuid[1] = HI_UINT16(GATT_CLIENT_CHAR_CFG_UUID);

            GATT_ReadUsingCharUUID(centralConnHandle, &req, centralTaskId);
        }
    }
    else if (centralDiscState == BLE_DISC_STATE_CCCD)
    {
        // Characteristic found, store handle
        if (pMsg->method == ATT_READ_BY_TYPE_RSP &&
            pMsg->msg.readByTypeRsp.numPairs > 0)
        {
            centralCCCDHdl = BUILD_UINT16(pMsg->msg.readByTypeRsp.pDataList[0],
                                          pMsg->msg.readByTypeRsp.pDataList[1]);
            centralProcedureInProgress = FALSE;

            // Start do write CCCD
            tmos_start_task(centralTaskId, START_WRITE_CCCD_EVT, DEFAULT_WRITE_CCCD_DELAY);

            // Display Characteristic 1 handle
            APP_DBG("Found client characteristic configuration handle : %X ", centralCCCDHdl);

            // Eko Badge found
            if (centralCCCDHdl == 0x38)
            {
                centralAddFriend(pMsg->connHandle);
            }
        }
        centralDiscState = BLE_DISC_STATE_IDLE;
    }
}

/*********************************************************************
 * @fn      centralAddDeviceInfo
 *
 * @brief   Add a device to the device discovery result list
 *
 * @return  none
 */
static void centralAddDeviceInfo(uint8_t *pAddr, uint8_t addrType, int8_t rssi)
{
    uint8_t i;

    // If result count not at max
    if (centralScanRes < DEFAULT_MAX_SCAN_RES)
    {
        // Check if device is already in scan results
        for (i = 0; i < centralScanRes; i++)
        {
            if (tmos_memcmp(pAddr, centralDevList[i].addr, B_ADDR_LEN))
            {
                return;
            }
        }
        // Add addr to scan result list
        tmos_memcpy(centralDevList[centralScanRes].addr, pAddr, B_ADDR_LEN);
        centralDevList[centralScanRes].addrType = addrType;
        centralDevList[centralScanRes].rssi = rssi;
        // Increment scan result count
        centralScanRes++;
        // Display device addr
        // APP_DBG("Device %d - Addr %X:%X:%X:%X:%X:%X \tRSSI: %d", centralScanRes,
        //         centralDevList[centralScanRes - 1].addr[5],
        //         centralDevList[centralScanRes - 1].addr[4],
        //         centralDevList[centralScanRes - 1].addr[3],
        //         centralDevList[centralScanRes - 1].addr[2],
        //         centralDevList[centralScanRes - 1].addr[1],
        //         centralDevList[centralScanRes - 1].addr[0],
        //         centralDevList[centralScanRes - 1].rssi);
    }
}

/*********************************************************************
 * @fn      performPeriodicTask
 *
 * @brief   Perform a periodic application task. This function gets
 *          called every five seconds as a result of the SBP_PERIODIC_EVT
 *          TMOS event.
 *
 * @param   none
 *
 * @return  none
 */
static void centralConnectingChecker(void)
{
    static uint8_t connectingAttepts = 0;

    if (centralConnecting)
    {
        connectingAttepts++;
        APP_DBG("Trying to connect to device...");
    }

    if (connectingAttepts == 5)
    {
        centralConnecting = FALSE;
        connectingAttepts = 0;
        centralScanRes = 0;
        GAPRole_CentralStartDiscovery(DEFAULT_DISCOVERY_MODE,
                                      DEFAULT_DISCOVERY_ACTIVE_SCAN,
                                      DEFAULT_DISCOVERY_WHITE_LIST);
    }
}

/************************ endfile @ central **************************/

/********************************** (C) COPYRIGHT *******************************
 * File Name          : app.c
 * Author             : WCH
 * Version            : V1.1
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
#include "display.h"
#include "keyboard.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
#define ADV_TIMEOUT K_MINUTES(10)

#define SELENCE_ADV_ON 0x01
#define SELENCE_ADV_OF 0x00

// ���ʱ���ã�����е͹��Ľڵ㣬����С�ڵ͹��Ľڵ㻽�Ѽ����Ĭ��10s��
#define APP_CMD_TIMEOUT 1600 * 10

#define APP_DELETE_LOCAL_NODE_DELAY 3200
// shall not less than APP_DELETE_LOCAL_NODE_DELAY
#define APP_DELETE_NODE_INFO_DELAY 3200
/*********************************************************************
 * GLOBAL TYPEDEFS
 */

static halKeyCBack_t Button_Pressed_Callback; /* callback function */

static uint8_t MESH_MEM[1024 * 3] = {0};

extern const ble_mesh_cfg_t app_mesh_cfg;
extern const struct device app_dev;

static uint8_t App_TaskID = 0; // Task ID for internal task/event processing

static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events);

static uint8_t dev_uuid[16] = {0}; // ���豸��UUID

static uint8_t self_prov_net_key[16] = {0};

static const uint8_t self_prov_dev_key[16] = {
    0x00,
    0x23,
    0x45,
    0x67,
    0x89,
    0xab,
    0xcd,
    0xef,
    0x00,
    0x23,
    0x45,
    0x67,
    0x89,
    0xab,
    0xcd,
    0xef,
};

static uint8_t self_prov_app_key[16] = {
    0x00,
    0x23,
    0x45,
    0x67,
    0x89,
    0xab,
    0xcd,
    0xef,
    0x00,
    0x23,
    0x45,
    0x67,
    0x89,
    0xab,
    0xcd,
    0xef,
};

const uint16_t self_prov_net_idx = 0x0000; // ���������õ�net key
const uint16_t self_prov_app_idx = 0x0001; // ���������õ�app key
uint32_t self_prov_iv_index = 0x00000000;  // ��������iv_index��Ĭ��Ϊ0
uint16_t self_prov_addr = 0;               // ��������������Ԫ�ص�ַ
uint8_t self_prov_flags = 0x00;            // �Ƿ���key����״̬��Ĭ��Ϊ��

uint16_t delete_node_address = 0;
uint16_t ask_status_node_address = 0;
uint16_t ota_update_node_address = 0;
uint16_t set_sub_node_address = 0;

#if (!CONFIG_BLE_MESH_PB_GATT)
NET_BUF_SIMPLE_DEFINE_STATIC(rx_buf, 65);
#endif /* !PB_GATT */

/*********************************************************************
 * LOCAL FUNCION
 */

static void cfg_srv_rsp_handler(const cfg_srv_status_t *val);
static void link_open(bt_mesh_prov_bearer_t bearer);
static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason);
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index);
static void cfg_cli_rsp_handler(const cfg_cli_status_t *val);
static void vendor_model_srv_rsp_handler(const vendor_model_srv_status_t *val);
static int vendor_model_srv_send(uint16_t addr, uint8_t *pData, uint16_t len);
static void prov_reset(void);
void App_trans_model_reveived(uint8_t *pValue, uint16_t len, uint16_t addr);
void FLASH_read(uint32_t addr, uint8_t *pData, uint32_t len);

static struct bt_mesh_cfg_srv cfg_srv = {
    .relay = BLE_MESH_RELAY_ENABLED,
    .beacon = BLE_MESH_BEACON_ENABLED,
#if (CONFIG_BLE_MESH_FRIEND)
    .frnd = BLE_MESH_FRIEND_ENABLED,
#endif
#if (CONFIG_BLE_MESH_PROXY)
    .gatt_proxy = BLE_MESH_GATT_PROXY_ENABLED,
#endif
    /* Ĭ��TTLΪ3 */
    .default_ttl = 3,
    /* �ײ㷢����������7�Σ�ÿ�μ��10ms�������ڲ�������� */
    .net_transmit = BLE_MESH_TRANSMIT(7, 10),
    /* �ײ�ת����������7�Σ�ÿ�μ��10ms�������ڲ�������� */
    .relay_retransmit = BLE_MESH_TRANSMIT(7, 10),
    .handler = cfg_srv_rsp_handler,
};

/* Attention on */
void app_prov_attn_on(struct bt_mesh_model *model)
{
    APP_DBG("app_prov_attn_on");
}

/* Attention off */
void app_prov_attn_off(struct bt_mesh_model *model)
{
    APP_DBG("app_prov_attn_off");
}

const struct bt_mesh_health_srv_cb health_srv_cb = {
    .attn_on = app_prov_attn_on,
    .attn_off = app_prov_attn_off,
};

static struct bt_mesh_health_srv health_srv = {
    .cb = &health_srv_cb,
};

BLE_MESH_HEALTH_PUB_DEFINE(health_pub, 8);

struct bt_mesh_cfg_cli cfg_cli = {
    .handler = cfg_cli_rsp_handler,
};

uint16_t cfg_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t cfg_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t cfg_cli_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t cfg_cli_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

uint16_t health_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t health_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

// rootģ�ͼ���
static struct bt_mesh_model root_models[] = {
    BLE_MESH_MODEL_CFG_SRV(cfg_srv_keys, cfg_srv_groups, &cfg_srv),
    BLE_MESH_MODEL_CFG_CLI(cfg_cli_keys, cfg_cli_groups, &cfg_cli),
    BLE_MESH_MODEL_HEALTH_SRV(health_srv_keys, health_srv_groups, &health_srv, &health_pub),
};

struct bt_mesh_vendor_model_srv vendor_model_srv = {
    .srv_tid.trans_tid = 0xFF,
    .handler = vendor_model_srv_rsp_handler,
};

uint16_t vnd_model_srv_keys[CONFIG_MESH_MOD_KEY_COUNT_DEF] = {BLE_MESH_KEY_UNUSED};
uint16_t vnd_model_srv_groups[CONFIG_MESH_MOD_GROUP_COUNT_DEF] = {BLE_MESH_ADDR_UNASSIGNED};

// �Զ���ģ�ͼ���
struct bt_mesh_model vnd_models[] = {
    BLE_MESH_MODEL_VND_CB(CID_WCH, BLE_MESH_MODEL_ID_WCH_SRV, vnd_model_srv_op, NULL, vnd_model_srv_keys,
                          vnd_model_srv_groups, &vendor_model_srv, NULL),
};

// ģ����� elements
static struct bt_mesh_elem elements[] = {
    {
        /* Location Descriptor (GATT Bluetooth Namespace Descriptors) */
        .loc = (0),
        .model_count = ARRAY_SIZE(root_models),
        .models = (root_models),
        .vnd_model_count = ARRAY_SIZE(vnd_models),
        .vnd_models = (vnd_models),
    }};

// elements ���� Node Composition
const struct bt_mesh_comp app_comp = {
    .cid = 0x07D7, // WCH ��˾id
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

// ���������ͻص�
static const struct bt_mesh_prov app_prov = {
    .uuid = dev_uuid,
    .link_open = link_open,
    .link_close = link_close,
    .complete = prov_complete,
    .reset = prov_reset,
};

// �����߹����Ľڵ㣬��0��Ϊ�Լ�����1��2����Ϊ����˳��Ľڵ�
node_t app_nodes[1] = {0};

app_mesh_manage_t app_mesh_manage;

uint16_t block_buf_len = 0;
uint32_t prom_addr = 0;

/* flash��������ʱ�洢 */
__attribute__((aligned(8))) uint8_t block_buf[512];

/*********************************************************************
 * GLOBAL TYPEDEFS
 */

/*********************************************************************
 * @fn      prov_enable
 *
 * @brief   ʹ����������
 *
 * @return  none
 */
static void prov_enable(void)
{
    if (bt_mesh_is_provisioned())
    {
        return;
    }

    // Make sure we're scanning for provisioning inviations
    bt_mesh_scan_enable();
    // Enable unprovisioned beacon sending
    bt_mesh_beacon_enable();

    if (CONFIG_BLE_MESH_PB_GATT)
    {
        bt_mesh_proxy_prov_enable();
    }
}

/*********************************************************************
 * @fn      link_open
 *
 * @brief   ����ʱ���link�򿪻ص�
 *
 * @param   bearer  - ��ǰlink��PB_ADV����PB_GATT
 *
 * @return  none
 */
static void link_open(bt_mesh_prov_bearer_t bearer)
{
    APP_DBG("");
}

/*********************************************************************
 * @fn      link_close
 *
 * @brief   �������link�رջص�
 *
 * @param   bearer  - ��ǰlink��PB_ADV����PB_GATT
 * @param   reason  - link�ر�ԭ��
 *
 * @return  none
 */
static void link_close(bt_mesh_prov_bearer_t bearer, uint8_t reason)
{
    if (reason != CLOSE_REASON_SUCCESS)
        APP_DBG("reason %x", reason);
}

/*********************************************************************
 * @fn      node_cfg_process
 *
 * @brief   ��һ�����еĽڵ㣬ִ����������
 *
 * @param   node        - �սڵ�ָ��
 * @param   net_idx     - ����key���
 * @param   addr        - �����ַ
 * @param   num_elem    - Ԫ������
 *
 * @return  node_t / NULL
 */
static node_t *node_cfg_process(node_t *node, uint16_t net_idx, uint16_t addr, uint8_t num_elem)
{
    node = app_nodes;
    node->net_idx = net_idx;
    node->node_addr = addr;
    node->elem_count = num_elem;
    return node;
}

/*********************************************************************
 * @fn      prov_complete
 *
 * @brief   ������ɻص�
 *
 * @param   net_idx     - ����key��index
 * @param   addr        - link�ر�ԭ�������ַ
 * @param   flags       - �Ƿ���key refresh״̬
 * @param   iv_index    - ��ǰ����iv��index
 *
 * @return  none
 */
static void prov_complete(uint16_t net_idx, uint16_t addr, uint8_t flags, uint32_t iv_index)
{
    int err;
    node_t *node;

    APP_DBG("");

    node = node_cfg_process(node, net_idx, addr, ARRAY_SIZE(elements));
    if (!node)
    {
        APP_DBG("Unable allocate node object");
        return;
    }
    set_led_state(LED_PIN, TRUE);
    Peripheral_AdvertData_Privisioned(TRUE);

    // ���δ���ù�������Ϣ�����˳��ص���ִ�����ñ���������Ϣ����
    if (vnd_models[0].keys[0] == BLE_MESH_KEY_UNUSED)
    {
        tmos_start_task(App_TaskID, APP_NODE_EVT, 160);
    }
}

/*********************************************************************
 * @fn      prov_reset
 *
 * @brief   ��λ�������ܻص�
 *
 * @param   none
 *
 * @return  none
 */
static void prov_reset(void)
{
    if (app_mesh_manage.local_reset.cmd == CMD_LOCAL_RESET)
    {
        app_mesh_manage.local_reset_ack.cmd = CMD_LOCAL_RESET_ACK;
        app_mesh_manage.local_reset_ack.status = STATUS_SUCCESS;
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, LOCAL_RESET_ACK_DATA_LEN);
    }
    Peripheral_TerminateLink();
    self_prov_iv_index = 0x00000000;
    self_prov_addr = 0;
    self_prov_flags = 0x00;
    delete_node_address = 0;
    ask_status_node_address = 0;
    ota_update_node_address = 0;
    set_sub_node_address = 0;
    set_led_state(LED_PIN, FALSE);
    Peripheral_AdvertData_Privisioned(FALSE);
    APP_DBG("Waiting for privisioning data");
#if (CONFIG_BLE_MESH_LOW_POWER)
    bt_mesh_lpn_set(FALSE);
    APP_DBG("Low power disable");
#endif /* LPN */
}

/*********************************************************************
 * @fn      cfg_local_net_info
 *
 * @brief   ���ñ���������Ϣ��������������������ֱ��������������Կ�Ͷ��ĵ�ַ
 *
 * @param   none
 *
 * @return  none
 */
static void cfg_local_net_info(void)
{
    uint8_t status;

    // ����ֱ������Ӧ����Կ
    status = bt_mesh_app_key_set(app_nodes->net_idx, self_prov_app_idx, self_prov_app_key, FALSE);
    if (status)
    {
        APP_DBG("Unable set app key");
    }
    APP_DBG("lcoal app key added");
    // ��Ӧ����Կ���ߺ��Զ���ģ��
    vnd_models[0].keys[0] = (uint16_t)self_prov_app_idx;
    bt_mesh_store_mod_bind(&vnd_models[0]);
    APP_DBG("lcoal app key binded");
#if (CONFIG_BLE_MESH_LOW_POWER)
    bt_mesh_lpn_set(TRUE);
    APP_DBG("Low power enable");
#endif /* LPN */
}

/*********************************************************************
 * @fn      App_model_find_group
 *
 * @brief   ��ģ�͵Ķ��ĵ�ַ�в�ѯ��Ӧ�ĵ�ַ
 *
 * @param   mod     - ָ���Ӧģ��
 * @param   addr    - ���ĵ�ַ
 *
 * @return  �鵽�Ķ��ĵ�ַָ��
 */
static uint16_t *App_model_find_group(struct bt_mesh_model *mod, u16_t addr)
{
    int i;

    for (i = 0; i < CONFIG_MESH_MOD_GROUP_COUNT_DEF; i++)
    {
        if (mod->groups[i] == addr)
        {
            return &mod->groups[i];
        }
    }

    return NULL;
}
/*********************************************************************
 * @fn      cfg_srv_rsp_handler
 *
 * @brief   config ģ�ͷ���ص�
 *
 * @param   val     - �ص������������������͡���������ִ��״̬
 *
 * @return  none
 */
static void cfg_srv_rsp_handler(const cfg_srv_status_t *val)
{
    if (val->cfgHdr.status)
    {
        // ��������ִ�в��ɹ�
        APP_DBG("warning opcode 0x%02x", val->cfgHdr.opcode);
        return;
    }
    if (val->cfgHdr.opcode == OP_APP_KEY_ADD)
    {
        APP_DBG("App Key Added");
    }
    else if (val->cfgHdr.opcode == OP_MOD_APP_BIND)
    {
        APP_DBG("Vendor Model Binded");
    }
    else if (val->cfgHdr.opcode == OP_MOD_SUB_ADD)
    {
        APP_DBG("Vendor Model Subscription Set");
    }
    else
    {
        APP_DBG("Unknow opcode 0x%02x", val->cfgHdr.opcode);
    }
}

/*********************************************************************
 * @fn      cfg_cli_rsp_handler
 *
 * @brief   �յ�cfg�����Ӧ��ص�
 *
 * @param   val     - �ص������������������ͺͷ�������
 *
 * @return  none
 */
static void cfg_cli_rsp_handler(const cfg_cli_status_t *val)
{
    APP_DBG("opcode 0x%04x", val->cfgHdr.opcode);

    if (val->cfgHdr.status == 0xFF)
    {
        APP_DBG("Opcode 0x%04x, timeout", val->cfgHdr.opcode);
    }
}

/*********************************************************************
 * @fn      vendor_model_srv_rsp_handler
 *
 * @brief   �Զ���ģ�ͷ���ص�
 *
 * @param   val     - �ص�������������Ϣ���͡��������ݡ����ȡ���Դ��ַ
 *
 * @return  none
 */
static void vendor_model_srv_rsp_handler(const vendor_model_srv_status_t *val)
{
    if (val->vendor_model_srv_Hdr.status)
    {
        // ��Ӧ�����ݴ��� ��ʱδ�յ�Ӧ��
        APP_DBG("Timeout opcode 0x%02x", val->vendor_model_srv_Hdr.opcode);
        return;
    }
    if (val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_MSG)
    {
        // �յ�͸������
        APP_DBG("len %d, data 0x%02x from 0x%04x", val->vendor_model_srv_Event.trans.len,
                val->vendor_model_srv_Event.trans.pdata[0],
                val->vendor_model_srv_Event.trans.addr);
        App_trans_model_reveived(val->vendor_model_srv_Event.trans.pdata, val->vendor_model_srv_Event.trans.len,
                                 val->vendor_model_srv_Event.trans.addr);
        //        // ת��������(���������)
        // peripheralChar4Notify(val->vendor_model_srv_Event.trans.pdata, val->vendor_model_srv_Event.trans.len);
    }
    else if (val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_WRT)
    {
        // �յ�write����
        APP_DBG("len %d, data 0x%02x from 0x%04x", val->vendor_model_srv_Event.write.len,
                val->vendor_model_srv_Event.write.pdata[0],
                val->vendor_model_srv_Event.write.addr);
        //        // ת��������(���������)
        //        peripheralChar4Notify(val->vendor_model_srv_Event.write.pdata, val->vendor_model_srv_Event.write.len);
    }
    else if (val->vendor_model_srv_Hdr.opcode == OP_VENDOR_MESSAGE_TRANSPARENT_IND)
    {
        // ���͵�indicate���յ�Ӧ��
    }
    else
    {
        APP_DBG("Unknow opcode 0x%02x", val->vendor_model_srv_Hdr.opcode);
    }
}

/*********************************************************************
 * @fn      vendor_model_srv_send
 *
 * @brief   ͨ�������Զ���ģ�ͷ�������
 *
 * @param   addr    - ��Ҫ���͵�Ŀ�ĵ�ַ
 *          pData   - ��Ҫ���͵�����ָ��
 *          len     - ��Ҫ���͵����ݳ���
 *
 * @return  �ο�Global_Error_Code
 */
static int vendor_model_srv_send(uint16_t addr, uint8_t *pData, uint16_t len)
{
    struct send_param param = {
        .app_idx = vnd_models[0].keys[0], // ����Ϣʹ�õ�app key�������ض���ʹ�õ�0��key
        .addr = addr,                     // ����Ϣ������Ŀ�ĵص�ַ������Ϊ�������ĵ�ַ�������Լ�
        .trans_cnt = 0x05,                // ����Ϣ���û��㷢�ʹ���
        .period = K_MSEC(500),            // ����Ϣ�ش��ļ�������鲻С��(200+50*TTL)ms�������ݽϴ�����ӳ�
        .rand = (0),                      // ����Ϣ���͵�����ӳ�
        .tid = vendor_srv_tid_get(),      // tid��ÿ��������Ϣ����ѭ����srvʹ��128~191
        .send_ttl = BLE_MESH_TTL_DEFAULT, // ttl�����ض���ʹ��Ĭ��ֵ
    };
    vendor_message_srv_trans_reset();
    return vendor_message_srv_send_trans(&param, pData, len); // ���ߵ����Զ���ģ�ͷ����͸�������������ݣ�ֻ���ͣ���Ӧ�����
}

/*********************************************************************
 * @fn      keyPress
 *
 * @brief   �����ص�
 *
 * @param   keys    - ��������
 *
 * @return  none
 */
void keyPress(uint8_t keys)
{
    Button_Pressed_Callback(keys);

    switch (keys)
    {
    default:
    {
        uint8_t status;
        uint8_t data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
        // status = vendor_model_srv_send(BLE_MESH_ADDR_ALL_NODES, data, 8);
        if (status)
        {
            APP_DBG("send failed %d", status);
        }
        break;
    }
    }
}

#if (CONFIG_BLE_MESH_FRIEND)
/*********************************************************************
 * @fn      friend_state
 *
 * @brief   ���ѹ�ϵ�����ص�
 *
 * @param   lpn_addr    - �͹��Ľڵ�������ַ
 * @param   state       - �ص�״̬
 *
 * @return  none
 */
static void friend_state(uint16_t lpn_addr, uint8_t state)
{
    if (state == FRIEND_FRIENDSHIP_ESTABLISHED)
    {
        APP_DBG("friend friendship established, lpn addr 0x%04x", lpn_addr);
    }
    else if (state == FRIEND_FRIENDSHIP_TERMINATED)
    {
        APP_DBG("friend friendship terminated, lpn addr 0x%04x", lpn_addr);
    }
    else
    {
        APP_DBG("unknow state %x", state);
    }
}
#endif

#if (CONFIG_BLE_MESH_LOW_POWER)
/*********************************************************************
 * @fn      lpn_state
 *
 * @brief   ���ѹ�ϵ�����ص�
 *
 * @param   friend_addr - ���ѽڵ��ַ
 *          state       - �ص�״̬
 *
 * @return  none
 */
static void lpn_state(uint16_t friend_addr, uint8_t state)
{
    if (state == LPN_FRIENDSHIP_ESTABLISHED)
    {
        APP_DBG("lpn friendship established");
    }
    else if (state == LPN_FRIENDSHIP_TERMINATED)
    {
        APP_DBG("lpn friendship terminated");
    }
    else
    {
        APP_DBG("unknow state %x", state);
    }
}
#endif

/*********************************************************************
 * @fn      blemesh_on_sync
 *
 * @brief   ͬ��mesh���������ö�Ӧ���ܣ��������޸�
 *
 * @return  none
 */
void blemesh_on_sync(void)
{
    int err;
    mem_info_t info;

    if (tmos_memcmp(VER_MESH_LIB, VER_MESH_FILE, strlen(VER_MESH_FILE)) == FALSE)
    {
        PRINT("head file error...\r\n");
        while (1)
            ;
    }

    info.base_addr = MESH_MEM;
    info.mem_len = ARRAY_SIZE(MESH_MEM);

#if (CONFIG_BLE_MESH_FRIEND)
    friend_init_register(bt_mesh_friend_init, friend_state);
#endif /* FRIEND */
#if (CONFIG_BLE_MESH_LOW_POWER)
    lpn_init_register(bt_mesh_lpn_init, lpn_state);
#endif /* LPN */

#if (defined(BLE_MAC)) && (BLE_MAC == TRUE)
    tmos_memcpy(dev_uuid, MacAddr, 6);
    err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, MacAddr, &info);
#else
    {
        uint8_t MacAddr[6];
        FLASH_GetMACAddress(MacAddr);
        tmos_memcpy(dev_uuid, MacAddr, 6);
        // ʹ��оƬmac��ַ
        err = bt_mesh_cfg_set(&app_mesh_cfg, &app_dev, MacAddr, &info);

        // Print MAC address
        APP_DBG("MAC address: %02X:%02X:%02X:%02X:%02X:%02X",
                MacAddr[5], MacAddr[4], MacAddr[3], MacAddr[2], MacAddr[1], MacAddr[0]);
    }
#endif
    if (err)
    {
        APP_DBG("Unable set configuration (err:%d)", err);
        return;
    }
    hal_rf_init();
    err = bt_mesh_comp_register(&app_comp);

#if (CONFIG_BLE_MESH_RELAY)
    bt_mesh_relay_init();
#endif /* RELAY  */
#if (CONFIG_BLE_MESH_PROXY || CONFIG_BLE_MESH_PB_GATT)
#if (CONFIG_BLE_MESH_PROXY)
    bt_mesh_proxy_beacon_init_register((void *)bt_mesh_proxy_beacon_init);
    gatts_notify_register(bt_mesh_gatts_notify);
    proxy_gatt_enable_register(bt_mesh_proxy_gatt_enable);
#endif /* PROXY  */
#if (CONFIG_BLE_MESH_PB_GATT)
    proxy_prov_enable_register(bt_mesh_proxy_prov_enable);
#endif /* PB_GATT  */

    bt_mesh_proxy_init();
#endif /* PROXY || PB-GATT */

#if (CONFIG_BLE_MESH_PROXY_CLI)
    bt_mesh_proxy_client_init(cli); // ������
#endif                              /* PROXY_CLI */

    bt_mesh_prov_retransmit_init();
#if (!CONFIG_BLE_MESH_PB_GATT)
    adv_link_rx_buf_register(&rx_buf);
#endif /* !PB_GATT */
    err = bt_mesh_prov_init(&app_prov);

    bt_mesh_mod_init();
    bt_mesh_net_init();
    bt_mesh_trans_init();
    bt_mesh_beacon_init();

    bt_mesh_adv_init();

#if ((CONFIG_BLE_MESH_PB_GATT) || (CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_OTA))
    bt_mesh_conn_adv_init();
#endif /* PROXY || PB-GATT || OTA */

#if (CONFIG_BLE_MESH_SETTINGS)
    bt_mesh_settings_init();
#endif /* SETTINGS */

#if (CONFIG_BLE_MESH_PROXY_CLI)
    bt_mesh_proxy_cli_adapt_init();
#endif /* PROXY_CLI */

#if ((CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_PB_GATT) || \
     (CONFIG_BLE_MESH_PROXY_CLI) || (CONFIG_BLE_MESH_OTA))
    bt_mesh_adapt_init();
#endif /* PROXY || PB-GATT || PROXY_CLI || OTA */

    if (err)
    {
        APP_DBG("Initializing mesh failed (err %d)", err);
        return;
    }

    APP_DBG("Bluetooth initialized");

#if (CONFIG_BLE_MESH_SETTINGS)
    settings_load();
#endif /* SETTINGS */

    if (bt_mesh_is_provisioned())
    {
        set_led_state(LED_PIN, TRUE);
        APP_DBG("Mesh network restored from flash");
#if (CONFIG_BLE_MESH_LOW_POWER)
        bt_mesh_lpn_set(TRUE);
        APP_DBG("Low power enable");
#endif /* LPN */
    }
    else
    {
        set_led_state(LED_PIN, FALSE);
        APP_DBG("Waiting for privisioning data");
        Peripheral_AdvertData_Privisioned(FALSE);
    }

    APP_DBG("Mesh initialized");
}

/*********************************************************************
 * @fn      App_Init
 *
 * @brief   Ӧ�ò��ʼ��
 *
 * @return  none
 */
void App_Init()
{
    GAPRole_PeripheralInit();
    Peripheral_Init();

    App_TaskID = TMOS_ProcessEventRegister(App_ProcessEvent);
    Button_Pressed_Callback = Keyboard_Scan_Callback;

    vendor_model_srv_init(vnd_models);
    blemesh_on_sync();
    HAL_KeyInit();
    HalKeyConfig(keyPress);
}

/*********************************************************************
 * @fn      App_ProcessEvent
 *
 * @brief   Ӧ�ò��¼���������
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
static uint16_t App_ProcessEvent(uint8_t task_id, uint16_t events)
{
    // �ڵ����������¼�����
    if (events & APP_NODE_EVT)
    {
        cfg_local_net_info();
        app_mesh_manage.provision_ack.cmd = CMD_PROVISION_ACK;
        app_mesh_manage.provision_ack.addr[0] = app_nodes[0].node_addr & 0xFF;
        app_mesh_manage.provision_ack.addr[1] = (app_nodes[0].node_addr >> 8) & 0xFF;
        app_mesh_manage.provision_ack.status = STATUS_SUCCESS;
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, PROVISION_ACK_DATA_LEN);
        return (events ^ APP_NODE_EVT);
    }

    if (events & APP_NODE_PROVISION_EVT)
    {
        if (self_prov_addr)
        {
            int err;
            err = bt_mesh_provision(self_prov_net_key, self_prov_net_idx, self_prov_flags,
                                    self_prov_iv_index, self_prov_addr, self_prov_dev_key);
            if (err)
            {
                APP_DBG("Self Privisioning (err %d)", err);
                self_prov_addr = 0;
                app_mesh_manage.provision_ack.cmd = CMD_PROVISION_ACK;
                app_mesh_manage.provision_ack.addr[0] = app_nodes[0].node_addr & 0xFF;
                app_mesh_manage.provision_ack.addr[1] = (app_nodes[0].node_addr >> 8) & 0xFF;
                app_mesh_manage.provision_ack.status = STATUS_INVALID;
                // ֪ͨ����(���������)
                peripheralChar4Notify(app_mesh_manage.data.buf, PROVISION_ACK_DATA_LEN);
            }
        }
        return (events ^ APP_NODE_PROVISION_EVT);
    }

    if (events & APP_DELETE_NODE_TIMEOUT_EVT)
    {
        // ͨ��Ӧ�ò��Զ�Э��ɾ����ʱ����������������
        APP_DBG("Delete node failed ");
        app_mesh_manage.delete_node_ack.cmd = CMD_DELETE_NODE_ACK;
        app_mesh_manage.delete_node_ack.addr[0] = delete_node_address & 0xFF;
        app_mesh_manage.delete_node_ack.addr[1] = (delete_node_address >> 8) & 0xFF;
        app_mesh_manage.delete_node_ack.status = STATUS_TIMEOUT;
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, DELETE_NODE_ACK_DATA_LEN);
        return (events ^ APP_DELETE_NODE_TIMEOUT_EVT);
    }

    if (events & APP_DELETE_LOCAL_NODE_EVT)
    {
        // �յ�ɾ�����ɾ������������Ϣ
        APP_DBG("Delete local node");
        self_prov_addr = 0;
        // ��λ��������״̬
        bt_mesh_reset();
        return (events ^ APP_DELETE_LOCAL_NODE_EVT);
    }

    if (events & APP_DELETE_NODE_INFO_EVT)
    {
        // ɾ���Ѵ洢�ı�ɾ���ڵ����Ϣ
        bt_mesh_delete_node_info(delete_node_address, app_comp.elem_count);
        APP_DBG("Delete stored node info complete");
        app_mesh_manage.delete_node_info_ack.cmd = CMD_DELETE_NODE_INFO_ACK;
        app_mesh_manage.delete_node_info_ack.addr[0] = delete_node_address & 0xFF;
        app_mesh_manage.delete_node_info_ack.addr[1] = (delete_node_address >> 8) & 0xFF;
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, DELETE_NODE_INFO_ACK_DATA_LEN);
        return (events ^ APP_DELETE_NODE_INFO_EVT);
    }

    if (events & APP_ASK_STATUS_NODE_TIMEOUT_EVT)
    {
        // ��ѯ�ڵ�״̬��ʱ
        APP_DBG("Ask status node failed ");
        app_mesh_manage.ask_status_ack.cmd = CMD_ASK_STATUS_ACK;
        app_mesh_manage.ask_status_ack.addr[0] = ask_status_node_address & 0xFF;
        app_mesh_manage.ask_status_ack.addr[1] = (ask_status_node_address >> 8) & 0xFF;
        app_mesh_manage.ask_status_ack.status = STATUS_TIMEOUT;
        ask_status_node_address = 0;
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, ASK_STATUS_ACK_DATA_LEN);
        return (events ^ APP_ASK_STATUS_NODE_TIMEOUT_EVT);
    }

    if (events & APP_OTA_UPDATE_TIMEOUT_EVT)
    {
        // OTA ������ʱ
        APP_DBG("OTA update node failed ");
        switch (app_mesh_manage.data.buf[0])
        {
        case CMD_IMAGE_INFO:
        {
            app_mesh_manage.image_info_ack.cmd = CMD_DELETE_NODE_ACK;
            app_mesh_manage.image_info_ack.addr[0] = ota_update_node_address & 0xFF;
            app_mesh_manage.image_info_ack.addr[1] = (ota_update_node_address >> 8) & 0xFF;
            app_mesh_manage.image_info_ack.status = STATUS_TIMEOUT;
            peripheralChar4Notify(app_mesh_manage.data.buf, IMAGE_INFO_ACK_DATA_LEN);
            break;
        }
        case CMD_UPDATE:
        {
            app_mesh_manage.update_ack.cmd = CMD_UPDATE_ACK;
            app_mesh_manage.update_ack.addr[0] = ota_update_node_address & 0xFF;
            app_mesh_manage.update_ack.addr[1] = (ota_update_node_address >> 8) & 0xFF;
            app_mesh_manage.update_ack.status = STATUS_TIMEOUT;
            peripheralChar4Notify(app_mesh_manage.data.buf, UPDATE_ACK_DATA_LEN);
            break;
        }
        case CMD_VERIFY:
        {
            app_mesh_manage.verify_ack.cmd = CMD_VERIFY_ACK;
            app_mesh_manage.verify_ack.addr[0] = ota_update_node_address & 0xFF;
            app_mesh_manage.verify_ack.addr[1] = (ota_update_node_address >> 8) & 0xFF;
            app_mesh_manage.verify_ack.status = STATUS_TIMEOUT;
            peripheralChar4Notify(app_mesh_manage.data.buf, VERIFY_ACK_DATA_LEN);
            break;
        }
        }
        vendor_message_srv_trans_reset();
        ota_update_node_address = 0;
        // ֪ͨ����(���������)
        return (events ^ APP_OTA_UPDATE_TIMEOUT_EVT);
    }

    if (events & APP_SET_SUB_TIMEOUT_EVT)
    {
        // ���ýڵ㶩�ĵ�ַ��ʱ
        APP_DBG("Set sub node failed ");
        app_mesh_manage.set_sub_ack.cmd = CMD_SET_SUB_ACK;
        app_mesh_manage.set_sub_ack.addr[0] = set_sub_node_address & 0xFF;
        app_mesh_manage.set_sub_ack.addr[1] = (set_sub_node_address >> 8) & 0xFF;
        app_mesh_manage.set_sub_ack.status = STATUS_TIMEOUT;
        set_sub_node_address = 0;
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, SET_SUB_ACK_DATA_LEN);
        return (events ^ APP_SET_SUB_TIMEOUT_EVT);
    }

    // Discard unknown events
    return 0;
}

/*********************************************************************
 * @fn      SwitchImageFlag
 *
 * @brief   �л�dataflash���ImageFlag
 *
 * @param   new_flag    - �л���ImageFlag
 *
 * @return  none
 */
void SwitchImageFlag(uint8_t new_flag)
{
    uint16_t i;
    uint32_t ver_flag;

    /* ��ȡ��һ�� */
    FLASH_read(OTA_DATAFLASH_ADD, &block_buf[0], 4);

    FLASH_Unlock_Fast();
    /* ������һ�� */
    FLASH_ErasePage_Fast(OTA_DATAFLASH_ADD);

    /* ����Image��Ϣ */
    block_buf[0] = new_flag;
    block_buf[1] = 0x5A;
    block_buf[2] = 0x5A;
    block_buf[3] = 0x5A;

    /* ���DataFlash */
    FLASH_ProgramPage_Fast(OTA_DATAFLASH_ADD, (uint32_t *)&block_buf[0]);
    FLASH_Lock_Fast();
}

/*********************************************************************
 * @fn      App_trans_model_reveived
 *
 * @brief   ͸��ģ���յ�����
 *
 * @param   pValue - pointer to data that was changed
 *          len - length of data
 *          addr - ������Դ��ַ
 *
 * @return  none
 */
void App_trans_model_reveived(uint8_t *pValue, uint16_t len, uint16_t addr)
{
    tmos_memcpy(&app_mesh_manage, pValue, len);
    APP_DBG("CMD: 0x%02X", app_mesh_manage.data.buf[0]);
    APP_DBG("CMD: 0x%02X", app_mesh_manage.data.buf[1]);
    APP_DBG("CMD: 0x%02X", app_mesh_manage.data.buf[2]);
    APP_DBG("CMD: 0x%02X", app_mesh_manage.data.buf[3]);
    APP_DBG("CMD: 0x%02X", app_mesh_manage.data.buf[4]);
    APP_DBG("CMD: 0x%02X", app_mesh_manage.data.buf[5]);
    APP_DBG("CMD: 0x%02X", app_mesh_manage.data.buf[6]);
    APP_DBG("CMD: 0x%02X", app_mesh_manage.data.buf[7]);

    if (app_mesh_manage.data.buf[7] == 0x07)
    {
        // tmos_start_task(displayTaskID, DISPLAY_TEST_EVENT, MS1_TO_SYSTEM_TIME(1000));
    }

    switch (app_mesh_manage.data.buf[0])
    {
    // �ж��Ƿ�Ϊɾ������
    case CMD_DELETE_NODE:
    {
        if (len != DELETE_NODE_DATA_LEN)
        {
            APP_DBG("Delete node data err!");
            return;
        }
        int status;
        APP_DBG("receive delete cmd, send ack and start delete node delay");
        app_mesh_manage.delete_node_ack.cmd = CMD_DELETE_NODE_ACK;
        app_mesh_manage.delete_node_ack.status = STATUS_SUCCESS;
        status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, DELETE_NODE_ACK_DATA_LEN);
        if (status)
        {
            APP_DBG("send ack failed %d", status);
        }
        // ����ɾ���������ȷ���CMD_DELETE_NODE_INFO����
        APP_DBG("send to all node to let them delete stored info ");
        app_mesh_manage.delete_node_info.cmd = CMD_DELETE_NODE_INFO;
        status = vendor_model_srv_send(BLE_MESH_ADDR_ALL_NODES,
                                       app_mesh_manage.data.buf, DELETE_NODE_INFO_DATA_LEN);
        if (status)
        {
            APP_DBG("send ack failed %d", status);
        }
        tmos_start_task(App_TaskID, APP_DELETE_LOCAL_NODE_EVT, APP_DELETE_LOCAL_NODE_DELAY);
        break;
    }

    // �ж��Ƿ�ΪӦ�ò��Զ���ɾ������Ӧ��
    case CMD_DELETE_NODE_ACK:
    {
        if (len != DELETE_NODE_ACK_DATA_LEN)
        {
            APP_DBG("Delete node ack data err!");
            return;
        }
        tmos_stop_task(App_TaskID, APP_DELETE_NODE_TIMEOUT_EVT);
        APP_DBG("Delete node complete");
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, PROVISION_ACK_DATA_LEN);
        break;
    }

    // �ж��Ƿ�Ϊ�нڵ㱻ɾ������Ҫɾ���洢�Ľڵ���Ϣ
    case CMD_DELETE_NODE_INFO:
    {
        if (len != DELETE_NODE_INFO_DATA_LEN)
        {
            APP_DBG("Delete node info data err!");
            return;
        }
        delete_node_address = addr;
        tmos_start_task(App_TaskID, APP_DELETE_NODE_INFO_EVT, APP_DELETE_NODE_INFO_DELAY);
        break;
    }

    // �ж��Ƿ�Ϊ��ѯ�ڵ���Ϣ����
    case CMD_ASK_STATUS:
    {
        if (len != ASK_STATUS_DATA_LEN)
        {
            APP_DBG("ask status data err!");
            return;
        }
        int status;
        app_mesh_manage.ask_status_ack.cmd = CMD_ASK_STATUS_ACK;
        app_mesh_manage.ask_status_ack.status = STATUS_SUCCESS; // �û��Զ���״̬��
        status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, ASK_STATUS_ACK_DATA_LEN);
        if (status)
        {
            APP_DBG("send ack failed %d", status);
        }
        break;
    }

    // �ж��Ƿ�Ϊ��ѯ�ڵ���Ϣ����Ӧ��
    case CMD_ASK_STATUS_ACK:
    {
        if (len != ASK_STATUS_ACK_DATA_LEN)
        {
            APP_DBG("ask status data err!");
            return;
        }
        tmos_stop_task(App_TaskID, APP_ASK_STATUS_NODE_TIMEOUT_EVT);
        APP_DBG("ask status complete");
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, ASK_STATUS_ACK_DATA_LEN);
        break;
    }

    case CMD_TRANSFER:
    {
        if (len < TRANSFER_DATA_LEN)
        {
            APP_DBG("transfer data err!");
            return;
        }
        int status;
        uint16 dst_addr;
        dst_addr = app_mesh_manage.transfer.addr[0] | (app_mesh_manage.transfer.addr[1] << 8);
        APP_DBG("Receive trans data, len: %d", len);
        app_trans_process(app_mesh_manage.transfer.data, len - TRANSFER_DATA_LEN, addr, dst_addr);
        // �ж��ǵ�����ַ����Ⱥ�����ַ
        if (BLE_MESH_ADDR_IS_UNICAST(dst_addr))
        {
            // �յ�������Ϣ��������ʾԭ·���յ������ݵ�һ�ֽڼ�һ����
            app_mesh_manage.transfer_receive.cmd = CMD_TRANSFER_RECEIVE;
            app_mesh_manage.transfer_receive.data[0]++;
            status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, len);
            if (status)
            {
                APP_DBG("send failed %d", status);
            }
        }
        else
        {
            // �յ�Ⱥ����Ϣ
        }
        break;
    }

    case CMD_TRANSFER_RECEIVE:
    {
        if (len < TRANSFER_RECEIVE_DATA_LEN)
        {
            APP_DBG("transfer receive data err!");
            return;
        }
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, len);
        break;
    }

    case CMD_IMAGE_INFO:
    {
        if (len != IMAGE_INFO_DATA_LEN)
        {
            APP_DBG("image info data err!");
            return;
        }
        int status;
        uint8_t i;
        // ���codeflash
        FLASH_Unlock();
        for (i = 0; i < (IMAGE_B_SIZE + 4095) / 4096; i++)
        {
            PRINT("ERASE:%08x num:%d\r\r\n", (int)(IMAGE_B_START_ADD + i * FLASH_BLOCK_SIZE), (int)(IMAGE_B_SIZE + 4095) / 4096);
            status = FLASH_ErasePage(IMAGE_B_START_ADD + i * FLASH_BLOCK_SIZE);
            if (status != FLASH_COMPLETE)
            {
                break;
            }
        }
        FLASH_Lock();
        // ��� image info
        app_mesh_manage.image_info_ack.cmd = CMD_IMAGE_INFO_ACK;
        /* IMAGE_SIZE */
        app_mesh_manage.image_info_ack.image_size[0] = (uint8_t)(IMAGE_SIZE & 0xff);
        app_mesh_manage.image_info_ack.image_size[1] = (uint8_t)((IMAGE_SIZE >> 8) & 0xff);
        app_mesh_manage.image_info_ack.image_size[2] = (uint8_t)((IMAGE_SIZE >> 16) & 0xff);
        app_mesh_manage.image_info_ack.image_size[3] = (uint8_t)((IMAGE_SIZE >> 24) & 0xff);
        /* BLOCK SIZE */
        app_mesh_manage.image_info_ack.block_size[0] = (uint8_t)(FLASH_BLOCK_SIZE & 0xff);
        app_mesh_manage.image_info_ack.block_size[1] = (uint8_t)((FLASH_BLOCK_SIZE >> 8) & 0xff);
        /* CHIP ID */
        app_mesh_manage.image_info_ack.chip_id[0] = CHIP_ID & 0xFF;
        app_mesh_manage.image_info_ack.chip_id[1] = (CHIP_ID >> 8) & 0xFF;
        /* STATUS */
        /* ����ʧ�� */
        if (status != FLASH_COMPLETE)
        {
            app_mesh_manage.image_info_ack.status = status;
        }
        else
        {
            prom_addr = IMAGE_B_START_ADD;
            app_mesh_manage.image_info_ack.status = SUCCESS;
        }
        status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, IMAGE_INFO_ACK_DATA_LEN);
        if (status)
        {
            APP_DBG("image info ack failed %d", status);
        }
        break;
    }

    case CMD_IMAGE_INFO_ACK:
    {
        if (len != IMAGE_INFO_ACK_DATA_LEN)
        {
            APP_DBG("image info ack data err!");
            return;
        }
        tmos_stop_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT);
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, IMAGE_INFO_ACK_DATA_LEN);
        break;
    }

    case CMD_UPDATE:
    {
        if (len < UPDATE_DATA_LEN)
        {
            APP_DBG("update data err!");
            return;
        }
        int status;
        uint32_t OpParaDataLen = 0;
        uint32_t OpAdd = 0;
        // дflash
        OpParaDataLen = len - UPDATE_DATA_LEN;
        OpAdd = (uint32_t)(app_mesh_manage.update.update_addr[0]);
        OpAdd |= ((uint32_t)(app_mesh_manage.update.update_addr[1]) << 8);
        OpAdd = OpAdd * 8;

        OpAdd += IMAGE_A_SIZE + 0x08000000;

        PRINT("IAP_PROM: %08x len:%d \r\r\n", (int)OpAdd, (int)OpParaDataLen);
        /* ��ǰ��ImageA��ֱ�ӱ�� */
        tmos_memcpy(&block_buf[block_buf_len], app_mesh_manage.update.data, OpParaDataLen);
        block_buf_len += OpParaDataLen;
        if (block_buf_len >= FLASH_PAGE_SIZE)
        {
            FLASH_Unlock_Fast();
            FLASH_ProgramPage_Fast(prom_addr, (uint32_t *)block_buf);
            FLASH_Lock_Fast();
            tmos_memcpy(block_buf, &block_buf[FLASH_PAGE_SIZE], block_buf_len - FLASH_PAGE_SIZE);
            block_buf_len -= FLASH_PAGE_SIZE;
            prom_addr += FLASH_PAGE_SIZE;
        }
        app_mesh_manage.update_ack.cmd = CMD_UPDATE_ACK;
        app_mesh_manage.update_ack.status = SUCCESS;
        status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, UPDATE_ACK_DATA_LEN);
        if (status)
        {
            APP_DBG("update ack failed %d", status);
        }
        break;
    }

    case CMD_UPDATE_ACK:
    {
        if (len != UPDATE_ACK_DATA_LEN)
        {
            APP_DBG("update ack data err!");
            return;
        }
        tmos_stop_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT);
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, UPDATE_ACK_DATA_LEN);
        break;
    }

    case CMD_VERIFY:
    {
        if (len < VERIFY_DATA_LEN)
        {
            APP_DBG("verify data err!");
            return;
        }
        int status;
        // У��flash
        uint32_t OpParaDataLen = 0;
        uint32_t OpAdd = 0;

        if (block_buf_len)
        {
            FLASH_Unlock_Fast();
            FLASH_ProgramPage_Fast(prom_addr, (uint32_t *)block_buf);
            FLASH_Lock_Fast();
            block_buf_len = 0;
            prom_addr = 0;
        }

        OpParaDataLen = len - VERIFY_DATA_LEN;
        OpAdd = (uint32_t)(app_mesh_manage.verify.update_addr[0]);
        OpAdd |= ((uint32_t)(app_mesh_manage.verify.update_addr[1]) << 8);
        OpAdd = OpAdd * 8;

        OpAdd += IMAGE_A_SIZE + 0x08000000;

        PRINT("IAP_VERIFY: %08x len:%d \r\r\n", (int)OpAdd, (int)OpParaDataLen);
        FLASH_read(OpAdd, block_buf, OpParaDataLen);
        /* ��ǰ��ImageA��ֱ�Ӷ�ȡImageBУ�� */
        status = tmos_memcmp(block_buf, app_mesh_manage.verify.data, OpParaDataLen);
        if (status == FALSE)
        {
            PRINT("IAP_VERIFY err \r\r\n");
            app_mesh_manage.verify_ack.status = FAILURE;
        }
        else
        {
            app_mesh_manage.verify_ack.status = SUCCESS;
        }
        app_mesh_manage.verify_ack.cmd = CMD_VERIFY_ACK;
        status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, VERIFY_ACK_DATA_LEN);
        if (status)
        {
            APP_DBG("verify ack failed %d", status);
        }
        break;
    }

    case CMD_VERIFY_ACK:
    {
        if (len != VERIFY_ACK_DATA_LEN)
        {
            APP_DBG("verify ack data err!");
            return;
        }
        tmos_stop_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT);
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, VERIFY_ACK_DATA_LEN);
        break;
    }

    case CMD_END:
    {
        if (len != END_DATA_LEN)
        {
            APP_DBG("end data err!");
            return;
        }
        int status;
        PRINT("IAP_END \r\r\n");

        /* �رյ�ǰ����ʹ���жϣ����߷���һ��ֱ��ȫ���ر� */
        __disable_irq();

        /* �޸�DataFlash���л���ImageIAP */
        SwitchImageFlag(IMAGE_IAP_FLAG);

        /* �ȴ���ӡ��� ����λ*/
        Delay_Ms(10);
        NVIC_SystemReset();
        break;
    }

    case CMD_SET_SUB:
    {
        if (len != SET_SUB_DATA_LEN)
        {
            APP_DBG("set sub data err!");
            return;
        }
        int status;
        uint16_t sub_addr;
        uint16_t *match;
        sub_addr = app_mesh_manage.set_sub.sub_addr[0] | (app_mesh_manage.set_sub.sub_addr[1] << 8);
        if (BLE_MESH_ADDR_IS_GROUP(sub_addr))
        {
            if (app_mesh_manage.set_sub.add_flag)
            {
                match = App_model_find_group(&vnd_models[0], BLE_MESH_ADDR_UNASSIGNED);
                if (match)
                {
                    // �������Ӷ��ĵ�ַ
                    *match = (uint16_t)sub_addr;
                    bt_mesh_store_mod_sub(&vnd_models[0]);
                    status = STATUS_SUCCESS;
                    APP_DBG("lcoal sub addr added");
                }
                else
                {
                    status = STATUS_NOMEM;
                }
            }
            else
            {
                match = App_model_find_group(&vnd_models[0], sub_addr);
                if (match)
                {
                    // ����ɾ�����ĵ�ַ
                    *match = (uint16_t)BLE_MESH_ADDR_UNASSIGNED;
                    bt_mesh_store_mod_sub(&vnd_models[0]);
                    status = STATUS_SUCCESS;
                    APP_DBG("lcoal sub addr deleted");
                }
                else
                {
                    status = STATUS_INVALID;
                }
            }
        }
        else
        {
            status = STATUS_INVALID;
        }
        app_mesh_manage.set_sub_ack.cmd = CMD_SET_SUB_ACK;
        app_mesh_manage.set_sub_ack.status = status; // �û��Զ���״̬��
        status = vendor_model_srv_send(addr, app_mesh_manage.data.buf, SET_SUB_ACK_DATA_LEN);
        if (status)
        {
            APP_DBG("set sub ack failed %d", status);
        }
        break;
    }

    case CMD_SET_SUB_ACK:
    {
        if (len != SET_SUB_ACK_DATA_LEN)
        {
            APP_DBG("set sub ack data err!");
            return;
        }
        tmos_stop_task(App_TaskID, APP_SET_SUB_TIMEOUT_EVT);
        vendor_message_srv_trans_reset();
        // ֪ͨ����(���������)
        peripheralChar4Notify(app_mesh_manage.data.buf, SET_SUB_ACK_DATA_LEN);
        break;
    }

    default:
    {
        APP_DBG("Invalid CMD ");
        break;
    }
    }
}

/*********************************************************************
 * @fn      App_peripheral_reveived
 *
 * @brief   ble����ӻ��յ�����
 *
 * @param   pValue - pointer to data that was changed
 *          len - length of data
 *
 * @return  none
 */
void App_peripheral_reveived(uint8_t *pValue, uint16_t len)
{
    tmos_memcpy(&app_mesh_manage, pValue, len);
    APP_DBG("CMD: %x", app_mesh_manage.data.buf[0]);
    switch (app_mesh_manage.data.buf[0])
    {
    // ������Ϣ����
    case CMD_PROVISION_INFO:
    {
        if (len != PROVISION_INFO_DATA_LEN)
        {
            APP_DBG("Privisioning info data err!");
            return;
        }
        // �ж������û��ǲ�ѯ
        if (app_mesh_manage.provision_info.set_flag)
        {
            self_prov_iv_index = app_mesh_manage.provision_info.iv_index[0] |
                                 app_mesh_manage.provision_info.iv_index[1] << 8 |
                                 app_mesh_manage.provision_info.iv_index[2] << 16 |
                                 app_mesh_manage.provision_info.iv_index[3] << 24;
            self_prov_flags = app_mesh_manage.provision_info.flag;
            APP_DBG("set iv 0x%x, flag %d ", self_prov_iv_index, self_prov_flags);
            app_mesh_manage.provision_info_ack.status = STATUS_SUCCESS;
        }
        else
        {
            uint32_t iv_index = bt_mesh_iv_index_get();
            app_mesh_manage.provision_info_ack.iv_index[0] = iv_index & 0xFF;
            app_mesh_manage.provision_info_ack.iv_index[1] = (iv_index >> 8) & 0xFF;
            app_mesh_manage.provision_info_ack.iv_index[2] = (iv_index >> 16) & 0xFF;
            app_mesh_manage.provision_info_ack.iv_index[3] = (iv_index >> 24) & 0xFF;
            app_mesh_manage.provision_info_ack.flag = bt_mesh_net_flags_get(self_prov_net_idx);
            APP_DBG("ask iv 0x%x, flag %d ", iv_index, app_mesh_manage.provision_info_ack.flag);
            app_mesh_manage.provision_info_ack.status = STATUS_SUCCESS;
        }
        app_mesh_manage.provision_info_ack.cmd = CMD_PROVISION_INFO_ACK;
        peripheralChar4Notify(app_mesh_manage.data.buf, PROVISION_INFO_ACK_DATA_LEN);
        break;
    }

    // �������� ���� 1�ֽ�������+16�ֽ�������Կ+2�ֽ������ַ
    case CMD_PROVISION:
    {
        if (len != PROVISION_DATA_LEN)
        {
            APP_DBG("Privisioning data err!");
            return;
        }
        tmos_memcpy(self_prov_net_key, app_mesh_manage.provision.net_key, PROVISION_NET_KEY_LEN);
        self_prov_addr = app_mesh_manage.provision.addr[0] | (app_mesh_manage.provision.addr[1] << 8);
        tmos_start_task(App_TaskID, APP_NODE_PROVISION_EVT, 160);
        break;
    }

    // ɾ�������нڵ�(�����Լ�������ʹ�ô˷�ʽ)��ͨ��Ӧ�ò��Զ�Э��ɾ��
    // ���� 1�ֽ�������+2�ֽ���Ҫɾ���Ľڵ��ַ
    case CMD_DELETE_NODE:
    {
        if (len != DELETE_NODE_DATA_LEN)
        {
            APP_DBG("Delete node data err!");
            return;
        }
        uint16 remote_addr;
        int status;
        remote_addr = app_mesh_manage.delete_node.addr[0] | (app_mesh_manage.delete_node.addr[1] << 8);
        APP_DBG("CMD_DELETE_NODE %x ", remote_addr);
        status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, DELETE_NODE_DATA_LEN);
        if (status)
        {
            APP_DBG("delete failed %d", status);
        }
        else
        {
            delete_node_address = remote_addr;
            // ��ʱ��δ�յ�Ӧ��ͳ�ʱ
            tmos_start_task(App_TaskID, APP_DELETE_NODE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
        }
        break;
    }

    case CMD_ASK_STATUS:
    {
        if (len != ASK_STATUS_DATA_LEN)
        {
            APP_DBG("ask status data err!");
            return;
        }
        uint16 remote_addr;
        int status;
        remote_addr = app_mesh_manage.ask_status.addr[0] | (app_mesh_manage.ask_status.addr[1] << 8);
        APP_DBG("CMD_ASK_STATUS %x ", remote_addr);
        status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, ASK_STATUS_DATA_LEN);
        if (status)
        {
            APP_DBG("ask_status failed %d", status);
        }
        else
        {
            ask_status_node_address = remote_addr;
            // ��ʱ��δ�յ�Ӧ��ͳ�ʱ
            tmos_start_task(App_TaskID, APP_ASK_STATUS_NODE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
        }
        break;
    }

    case CMD_TRANSFER:
    {
        if (len < TRANSFER_DATA_LEN)
        {
            APP_DBG("transfer data err!");
            return;
        }
        uint16 remote_addr;
        int status;
        remote_addr = app_mesh_manage.transfer.addr[0] | (app_mesh_manage.transfer.addr[1] << 8);
        APP_DBG("CMD_TRANSFER %x ", remote_addr);
        status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, len);
        if (status)
        {
            APP_DBG("transfer failed %d", status);
        }
        break;
    }

    case CMD_IMAGE_INFO:
    {
        if (len != IMAGE_INFO_DATA_LEN)
        {
            APP_DBG("image info data err!");
            return;
        }
        uint16 remote_addr;
        int status;
        remote_addr = app_mesh_manage.image_info.addr[0] | (app_mesh_manage.image_info.addr[1] << 8);
        APP_DBG("CMD_IMAGE_INFO %x ", remote_addr);
        status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, IMAGE_INFO_DATA_LEN);
        if (status)
        {
            APP_DBG("image info failed %d", status);
            break;
        }
        else
        {
            ota_update_node_address = remote_addr;
            tmos_start_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
        }
        break;
    }

    case CMD_UPDATE:
    {
        if (len < UPDATE_DATA_LEN)
        {
            APP_DBG("update data err!");
            return;
        }
        uint16 remote_addr;
        int status;
        APP_DBG("CMD_UPDATE len: %d", len);
        remote_addr = app_mesh_manage.transfer.addr[0] | (app_mesh_manage.transfer.addr[1] << 8);
        status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, len);
        if (status)
        {
            APP_DBG("update failed %d", status);
        }
        else
        {
            ota_update_node_address = remote_addr;
            tmos_start_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
        }
        break;
    }

    case CMD_VERIFY:
    {
        if (len < VERIFY_DATA_LEN)
        {
            APP_DBG("verify data err!");
            return;
        }
        uint16 remote_addr;
        int status;
        remote_addr = app_mesh_manage.verify.addr[0] | (app_mesh_manage.verify.addr[1] << 8);
        status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, len);
        if (status)
        {
            APP_DBG("verify failed %d", status);
        }
        else
        {
            ota_update_node_address = remote_addr;
            tmos_start_task(App_TaskID, APP_OTA_UPDATE_TIMEOUT_EVT, APP_CMD_TIMEOUT);
        }
        break;
    }

    case CMD_END:
    {
        if (len != END_DATA_LEN)
        {
            APP_DBG("end data err!");
            return;
        }
        uint16 remote_addr;
        int status;
        remote_addr = app_mesh_manage.end.addr[0] | (app_mesh_manage.end.addr[1] << 8);
        status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, END_DATA_LEN);
        if (status)
        {
            APP_DBG("end failed %d", status);
        }
        break;
    }

    case CMD_SET_SUB:
    {
        if (len != SET_SUB_DATA_LEN)
        {
            APP_DBG("set sub data err!");
            return;
        }
        uint16 remote_addr;
        int status;
        remote_addr = app_mesh_manage.set_sub.addr[0] | (app_mesh_manage.set_sub.addr[1] << 8);
        APP_DBG("CMD_SET_SUB %x ", remote_addr);
        status = vendor_model_srv_send(remote_addr, app_mesh_manage.data.buf, SET_SUB_DATA_LEN);
        if (status)
        {
            APP_DBG("set sub failed %d", status);
        }
        else
        {
            set_sub_node_address = remote_addr;
            tmos_start_task(App_TaskID, APP_SET_SUB_TIMEOUT_EVT, APP_CMD_TIMEOUT);
        }
        break;
    }

    // ���ظ�λ������� 1�ֽ�������
    case CMD_LOCAL_RESET:
    {
        if (len != LOCAL_RESET_DATA_LEN)
        {
            APP_DBG("local reset data err!");
            return;
        }
        APP_DBG("Local mesh reset");
        self_prov_addr = 0;
        bt_mesh_reset();
        break;
    }

    default:
    {
        APP_DBG("Invalid CMD ");
        break;
    }
    }
}

/*********************************************************************
 * @fn      FLASH_read
 *
 * @brief   �� flash
 *
 * @return  none
 */
void FLASH_read(uint32_t addr, uint8_t *pData, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++)
    {
        *pData++ = *(uint8_t *)addr++;
    }
}

/******************************** endfile @ main ******************************/

/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_vendor_model_srv.c
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
#include "app_mesh_config.h"
#include "app_vendor_model_srv.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
// Ӧ�ò�����ͳ��ȣ����ְ����ΪCONFIG_MESH_UNSEG_LENGTH_DEF���ְ����ΪCONFIG_MESH_TX_SEG_DEF*BLE_MESH_APP_SEG_SDU_MAX-8������RAMʹ�����������
#define APP_MAX_TX_SIZE    MAX(CONFIG_MESH_UNSEG_LENGTH_DEF, CONFIG_MESH_TX_SEG_DEF *BLE_MESH_APP_SEG_SDU_MAX - 8)

static uint8_t vendor_model_srv_TaskID = 0; // Task ID for internal task/event processing
static uint8_t srv_send_tid = 128;
static int32_t srv_msg_timeout = K_SECONDS(2); //��Ӧ������ݴ��䳬ʱʱ�䣬Ĭ��2��

static struct net_buf          ind_buf;
static struct bt_mesh_indicate indicate = {
    .buf = &ind_buf,
};

static struct net_buf       srv_trans_buf;
static struct bt_mesh_trans srv_trans = {
    .buf = &srv_trans_buf,
};

static struct bt_mesh_vendor_model_srv *vendor_model_srv;

static void ind_reset(struct bt_mesh_indicate *ind, int err);

static uint16_t vendor_model_srv_ProcessEvent(uint8_t task_id, uint16_t events);
static void     ind_reset(struct bt_mesh_indicate *ind, int err);
static void     adv_srv_trans_send(void);

/*********************************************************************
 * @fn      vendor_srv_tid_get
 *
 * @brief   TID selection method
 *
 * @return  TID
 */
uint8_t vendor_srv_tid_get(void)
{
    srv_send_tid++;
    if(srv_send_tid > 191)
        srv_send_tid = 128;
    return srv_send_tid;
}

/*********************************************************************
 * @fn      vendor_model_srv_reset
 *
 * @brief   ��λ����ģ�ͷ���ȡ���������ڷ��͵�����
 *
 * @return  none
 */
static void vendor_model_srv_reset(void)
{
    APP_DBG("");
    vendor_model_srv->op_pending = 0U;
    vendor_model_srv->op_req = 0U;

    if(indicate.buf->__buf)
    {
        ind_reset(&indicate, -ECANCELED);
    }
    if(srv_trans.buf->__buf)
    {
        vendor_message_srv_trans_reset();
    }

    tmos_stop_task(vendor_model_srv_TaskID, VENDOR_MODEL_SRV_RSP_TIMEOUT_EVT);
    tmos_stop_task(vendor_model_srv_TaskID, VENDOR_MODEL_SRV_INDICATE_EVT);
}

/*********************************************************************
 * @fn      vendor_model_srv_rsp_recv
 *
 * @brief   ����Ӧ�ò㴫��Ļص�
 *
 * @param   val     - �ص�������������Ϣ���͡��������ݡ����ȡ���Դ��ַ
 * @param   statu   - ״̬
 *
 * @return  none
 */
static void vendor_model_srv_rsp_recv(vendor_model_srv_status_t *val,
                                      uint8_t                    status)
{
    if(vendor_model_srv == NULL || (!vendor_model_srv->op_req))
    {
        return;
    }

    val->vendor_model_srv_Hdr.opcode = vendor_model_srv->op_req;
    val->vendor_model_srv_Hdr.status = status;

    vendor_model_srv_reset();

    if(vendor_model_srv->handler)
    {
        vendor_model_srv->handler(val);
    }
}

/*********************************************************************
 * @fn      vendor_model_srv_wait
 *
 * @brief   Ĭ�����볬ʱ��֪ͨӦ�ò�
 *
 * @return  �ο�BLE_LIB err code
 */
static int vendor_model_srv_wait(void)
{
    int err;

    err = tmos_start_task(vendor_model_srv_TaskID,
                          VENDOR_MODEL_SRV_RSP_TIMEOUT_EVT, srv_msg_timeout);

    return err;
}

/*********************************************************************
 * @fn      vendor_model_srv_prepare
 *
 * @brief   Ԥ���ͣ���¼��ǰ��Ϣ����
 *
 * @param   op_req  - ����ȥ����Ϣ����
 * @param   op      - �ڴ��յ�����Ϣ����
 *
 * @return  ������
 */
static int vendor_model_srv_prepare(uint32_t op_req, uint32_t op)
{
    if(!vendor_model_srv)
    {
        APP_DBG("No available Configuration Client context!");
        return -EINVAL;
    }

    if(vendor_model_srv->op_pending)
    {
        APP_DBG("Another synchronous operation pending");
        return -EBUSY;
    }

    vendor_model_srv->op_req = op_req;
    vendor_model_srv->op_pending = op;

    return 0;
}

/*********************************************************************
 * @fn      vendor_srv_sync_handler
 *
 * @brief   ֪ͨӦ�ò㵱ǰop_code��ʱ��
 *
 * @return  none
 */
static void vendor_srv_sync_handler(void)
{
    vendor_model_srv_status_t vendor_model_srv_status;

    tmos_memset(&vendor_model_srv_status, 0,
                sizeof(vendor_model_srv_status_t));

    ind_reset(&indicate, -ETIMEDOUT);

    vendor_model_srv_rsp_recv(&vendor_model_srv_status, 0xFF);
}

/*********************************************************************
 * @fn      vendor_message_srv_confirm
 *
 * @brief   ����vendor_message_srv_confirm - ����Ϣ����Vendor Model Server�ظ���Vendor Model Client��
 *          ���ڱ�ʾ���յ�Vendor Model Client������ Write
 *
 * @param   model   - ģ�Ͳ���.
 * @param   ctx     - ���ݲ���.
 * @param   buf     - ��������.
 *
 * @return  none
 */
static void vendor_message_srv_confirm(struct bt_mesh_model   *model,
                                       struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, APP_MAX_TX_SIZE + 8);
    uint8_t recv_tid;
    int     err;

    recv_tid = net_buf_simple_pull_u8(buf);

    APP_DBG("tid 0x%02x ", recv_tid);

    /* Init indication opcode */
    bt_mesh_model_msg_init(&msg, OP_VENDOR_MESSAGE_TRANSPARENT_CFM);

    /* Add tid field */
    net_buf_simple_add_u8(&msg, recv_tid);

    err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
    if(err)
    {
        APP_DBG("#mesh-onoff STATUS: send status failed: %d", err);
    }
}

/*********************************************************************
 * @fn      vendor_message_srv_trans
 *
 * @brief   �յ�͸������
 *
 * @param   model   - ģ�Ͳ���.
 * @param   ctx     - ���ݲ���.
 * @param   buf     - ��������.
 *
 * @return  none
 */
static void vendor_message_srv_trans(struct bt_mesh_model   *model,
                                     struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    vendor_model_srv_status_t vendor_model_srv_status;
    uint8_t                  *pData = buf->data;
    uint16_t                  len = buf->len;

    if((pData[0] != vendor_model_srv->srv_tid.trans_tid) ||
       (ctx->addr != vendor_model_srv->srv_tid.trans_addr))
    {
        vendor_model_srv->srv_tid.trans_tid = pData[0];
        vendor_model_srv->srv_tid.trans_addr = ctx->addr;
        // ��ͷΪtid
        pData++;
        len--;
        vendor_model_srv_status.vendor_model_srv_Hdr.opcode =
            OP_VENDOR_MESSAGE_TRANSPARENT_MSG;
        vendor_model_srv_status.vendor_model_srv_Hdr.status = 0;
        vendor_model_srv_status.vendor_model_srv_Event.trans.pdata = pData;
        vendor_model_srv_status.vendor_model_srv_Event.trans.len = len;
        vendor_model_srv_status.vendor_model_srv_Event.trans.addr = ctx->addr;
        if(vendor_model_srv->handler)
        {
            vendor_model_srv->handler(&vendor_model_srv_status);
        }
    }
}

/*********************************************************************
 * @fn      vendor_message_srv_write
 *
 * @brief   �յ�write ���ݣ����Ӧ��vendor_message_srv_confirm
 *
 * @param   model   - ģ�Ͳ���.
 * @param   ctx     - ���ݲ���.
 * @param   buf     - ��������.
 *
 * @return  none
 */
static void vendor_message_srv_write(struct bt_mesh_model   *model,
                                     struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    vendor_model_srv_status_t vendor_model_srv_status;
    uint8_t                  *pData = buf->data;
    uint16_t                  len = buf->len;

    if((pData[0] != vendor_model_srv->srv_tid.write_tid) ||
       (ctx->addr != vendor_model_srv->srv_tid.write_addr) )
    {
        vendor_model_srv->srv_tid.write_tid = pData[0];
        vendor_model_srv->srv_tid.write_addr = ctx->addr;
        // ��ͷΪtid
        pData++;
        len--;
        vendor_model_srv_status.vendor_model_srv_Hdr.opcode =
            OP_VENDOR_MESSAGE_TRANSPARENT_WRT;
        vendor_model_srv_status.vendor_model_srv_Hdr.status = 0;
        vendor_model_srv_status.vendor_model_srv_Event.write.pdata = pData;
        vendor_model_srv_status.vendor_model_srv_Event.write.len = len;
        vendor_model_srv_status.vendor_model_srv_Event.write.addr = ctx->addr;
        if(vendor_model_srv->handler)
        {
            vendor_model_srv->handler(&vendor_model_srv_status);
        }
    }
    vendor_message_srv_confirm(model, ctx, buf);
}

/*********************************************************************
 * @fn      vendor_message_srv_ack
 *
 * @brief   �յ�vendor_message_srv_ack - ����Ϣ����Vendor Model Client�ظ���Vendor Model Server��
 *          ���ڱ�ʾ���յ�Vendor Model Server������Indication
 *
 * @param   model   - ģ�Ͳ���.
 * @param   ctx     - ���ݲ���.
 * @param   buf     - ��������.
 *
 * @return  none
 */
static void vendor_message_srv_ack(struct bt_mesh_model   *model,
                                   struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf)
{
    uint8_t                   recv_tid;
    vendor_model_srv_status_t vendor_model_srv_status;

    recv_tid = net_buf_simple_pull_u8(buf);

    APP_DBG("src: 0x%04x dst: 0x%04x tid 0x%02x rssi: %d", ctx->addr,
            ctx->recv_dst, recv_tid, ctx->recv_rssi);

    if(indicate.param.tid == recv_tid)
    {
        ind_reset(&indicate, 0);
        vendor_model_srv_rsp_recv(&vendor_model_srv_status, 0);
    }
}

// opcode ��Ӧ�Ĵ�������
const struct bt_mesh_model_op vnd_model_srv_op[] = {
    {OP_VENDOR_MESSAGE_TRANSPARENT_MSG, 0, vendor_message_srv_trans},
    {OP_VENDOR_MESSAGE_TRANSPARENT_WRT, 0, vendor_message_srv_write},
    {OP_VENDOR_MESSAGE_TRANSPARENT_ACK, 0, vendor_message_srv_ack},
    BLE_MESH_MODEL_OP_END,
};

/*********************************************************************
 * @fn      vendor_message_srv_indicate
 *
 * @brief   indicate,��Ӧ��������ͨ��
 *
 * @param   param   - ���Ͳ���.
 * @param   pData   - ����ָ��.
 * @param   len     - ���ݳ���,���Ϊ(APP_MAX_TX_SIZE).
 *
 * @return  �ο�Global_Error_Code
 */
int vendor_message_srv_indicate(struct send_param *param, uint8_t *pData,
                                uint16_t len)
{
    if(!param->addr)
        return -EINVAL;
    if(indicate.buf->__buf)
        return -EBUSY;
    if(len > (APP_MAX_TX_SIZE))
        return -EINVAL;

    indicate.buf->__buf = tmos_msg_allocate(len + 8);
    if(!(indicate.buf->__buf))
    {
        APP_DBG("No enough space!");
        return -ENOMEM;
    }
    indicate.buf->size = len + 4;
    /* Init indication opcode */
    bt_mesh_model_msg_init(&(indicate.buf->b),
                           OP_VENDOR_MESSAGE_TRANSPARENT_IND);

    /* Add tid field */
    net_buf_simple_add_u8(&(indicate.buf->b), param->tid);

    net_buf_simple_add_mem(&(indicate.buf->b), pData, len);

    memcpy(&indicate.param, param, sizeof(struct send_param));

    vendor_model_srv_prepare(OP_VENDOR_MESSAGE_TRANSPARENT_IND,
                             OP_VENDOR_MESSAGE_TRANSPARENT_ACK);

    tmos_start_task(vendor_model_srv_TaskID, VENDOR_MODEL_SRV_INDICATE_EVT,
                    param->rand);

    vendor_model_srv_wait();
    return 0;
}

/*********************************************************************
 * @fn      vendor_message_srv_send_trans
 *
 * @brief   send_trans,͸������ͨ��
 *
 * @param   param   - ���Ͳ���.
 * @param   pData   - ����ָ��.
 * @param   len     - ���ݳ���,���Ϊ(APP_MAX_TX_SIZE).
 *
 * @return  �ο�Global_Error_Code
 */
int vendor_message_srv_send_trans(struct send_param *param, uint8_t *pData,
                                  uint16_t len)
{
    if(!param->addr)
        return -EINVAL;
    if(srv_trans.buf->__buf)
        return -EBUSY;
    if(len > (APP_MAX_TX_SIZE))
        return -EINVAL;

    srv_trans.buf->__buf = tmos_msg_allocate(len + 8);
    if(!(srv_trans.buf->__buf))
    {
        APP_DBG("No enough space!");
        return -ENOMEM;
    }
    srv_trans.buf->size = len + 4;
    /* Init indication opcode */
    bt_mesh_model_msg_init(&(srv_trans.buf->b),
                           OP_VENDOR_MESSAGE_TRANSPARENT_MSG);

    /* Add tid field */
    net_buf_simple_add_u8(&(srv_trans.buf->b), param->tid);

    net_buf_simple_add_mem(&(srv_trans.buf->b), pData, len);

    memcpy(&srv_trans.param, param, sizeof(struct send_param));

    if(param->rand)
    {
        // �ӳٷ���
        tmos_start_task(vendor_model_srv_TaskID, VENDOR_MODEL_SRV_TRANS_EVT,
                        param->rand);
    }
    else
    {
        // ֱ�ӷ���
        adv_srv_trans_send();
    }
    return 0;
}

/*********************************************************************
 * @fn      vendor_message_srv_trans_reset
 *
 * @brief   ȡ������trans���ݵ������ͷŻ���
 *
 * @return  none
 */
void vendor_message_srv_trans_reset(void)
{
    tmos_msg_deallocate(srv_trans.buf->__buf);
    srv_trans.buf->__buf = NULL;
    tmos_stop_task(vendor_model_srv_TaskID, VENDOR_MODEL_SRV_TRANS_EVT);
}

/*********************************************************************
 * @fn      ind_reset
 *
 * @brief   ����indicate������ɻص����ͷŻ���
 *
 * @param   ind     - ��Ҫ�ͷŵ�indicate.
 * @param   err     - ����״̬.
 *
 * @return  none
 */
static void ind_reset(struct bt_mesh_indicate *ind, int err)
{
    if(ind->param.cb && ind->param.cb->end)
    {
        ind->param.cb->end(err, ind->param.cb_data);
    }

    tmos_msg_deallocate(ind->buf->__buf);
    ind->buf->__buf = NULL;
}

/*********************************************************************
 * @fn      ind_start
 *
 * @brief   ���� indicate ��ʼ�ص�
 *
 * @param   duration    - �˴η��ͳ�����ʱ����ms��.
 * @param   err         - ����״̬.
 * @param   cb_data     - ����״̬����ʱ��д�Ļص�����.
 *
 * @return  none
 */
static void ind_start(uint16_t duration, int err, void *cb_data)
{
    struct bt_mesh_indicate *ind = cb_data;

    if(ind->buf->__buf == NULL)
    {
        return;
    }

    if(err)
    {
        APP_DBG("Unable send indicate (err:%d)", err);
        tmos_start_task(vendor_model_srv_TaskID, VENDOR_MODEL_SRV_INDICATE_EVT,
                        K_MSEC(100));
        return;
    }
}

/*********************************************************************
 * @fn      ind_end
 *
 * @brief   ���� indicate �����ص�
 *
 * @param   err         - ����״̬.
 * @param   cb_data     - ����״̬����ʱ��д�Ļص�����.
 *
 * @return  none
 */
static void ind_end(int err, void *cb_data)
{
    struct bt_mesh_indicate *ind = cb_data;

    if(ind->buf->__buf == NULL)
    {
        return;
    }

    tmos_start_task(vendor_model_srv_TaskID, VENDOR_MODEL_SRV_INDICATE_EVT,
                    ind->param.period);
}

// ���� indicate �ص��ṹ��
const struct bt_mesh_send_cb ind_cb = {
    .start = ind_start,
    .end = ind_end,
};

/*********************************************************************
 * @fn      adv_ind_send
 *
 * @brief   ���� indicate
 *
 * @return  none
 */
static void adv_ind_send(void)
{
    int err;
    NET_BUF_SIMPLE_DEFINE(msg, APP_MAX_TX_SIZE + 8);

    struct bt_mesh_msg_ctx ctx = {
        .net_idx = indicate.param.net_idx ? indicate.param.net_idx : BLE_MESH_KEY_ANY,
        .app_idx = indicate.param.app_idx ? indicate.param.app_idx : vendor_model_srv->model->keys[0],
        .addr = indicate.param.addr,
    };

    if(indicate.buf->__buf == NULL)
    {
        APP_DBG("NULL buf");
        return;
    }

    if(indicate.param.trans_cnt == 0)
    {
        //		APP_DBG("indicate.buf.trans_cnt over");
        ind_reset(&indicate, -ETIMEDOUT);
        return;
    }

    indicate.param.trans_cnt--;

    ctx.send_ttl = indicate.param.send_ttl;

    net_buf_simple_add_mem(&msg, indicate.buf->data, indicate.buf->len);

    err = bt_mesh_model_send(vendor_model_srv->model, &ctx, &msg, &ind_cb,
                             &indicate);
    if(err)
    {
        APP_DBG("Unable send model message (err:%d)", err);

        ind_reset(&indicate, err);
        return;
    }
}

/*********************************************************************
 * @fn      adv_srv_trans_send
 *
 * @brief   ���� ͸�� srv_trans
 *
 * @return  none
 */
static void adv_srv_trans_send(void)
{
    int err;
    NET_BUF_SIMPLE_DEFINE(msg, APP_MAX_TX_SIZE + 8);

    struct bt_mesh_msg_ctx ctx = {
        .net_idx = srv_trans.param.net_idx ? srv_trans.param.net_idx : BLE_MESH_KEY_ANY,
        .app_idx = srv_trans.param.app_idx ? srv_trans.param.app_idx : vendor_model_srv->model->keys[0],
        .addr = srv_trans.param.addr,
    };

    if(srv_trans.buf->__buf == NULL)
    {
        APP_DBG("NULL buf");
        return;
    }

    if(srv_trans.param.trans_cnt == 0)
    {
        //		APP_DBG("srv_trans.buf.trans_cnt over");
        tmos_msg_deallocate(srv_trans.buf->__buf);
        srv_trans.buf->__buf = NULL;
        return;
    }

    srv_trans.param.trans_cnt--;

    ctx.send_ttl = srv_trans.param.send_ttl;

    net_buf_simple_add_mem(&msg, srv_trans.buf->data, srv_trans.buf->len);

    err = bt_mesh_model_send(vendor_model_srv->model, &ctx, &msg, NULL, NULL);
    if(err)
    {
        APP_DBG("Unable send model message (err:%d)", err);
        tmos_msg_deallocate(srv_trans.buf->__buf);
        srv_trans.buf->__buf = NULL;
        return;
    }

    if(srv_trans.param.trans_cnt == 0)
    {
        //    APP_DBG("srv_trans.buf.trans_cnt over");
        tmos_msg_deallocate(srv_trans.buf->__buf);
        srv_trans.buf->__buf = NULL;
        return;
    }
    // �ش�
    tmos_start_task(vendor_model_srv_TaskID, VENDOR_MODEL_SRV_TRANS_EVT,
                    srv_trans.param.period);
}

/*********************************************************************
 * @fn      vendor_model_srv_init
 *
 * @brief   ����ģ�ͳ�ʼ��
 *
 * @param   model       - ָ����ģ�ͽṹ��
 *
 * @return  always SUCCESS
 */
int vendor_model_srv_init(struct bt_mesh_model *model)
{
    vendor_model_srv = model->user_data;
    vendor_model_srv->model = model;

    vendor_model_srv_TaskID = TMOS_ProcessEventRegister(
        vendor_model_srv_ProcessEvent);
    return 0;
}

/*********************************************************************
 * @fn      vendor_model_srv_ProcessEvent
 *
 * @brief   ����ģ���¼���������
 *
 * @param   task_id  - The TMOS assigned task ID.
 * @param   events - events to process.  This is a bit map and can
 *                   contain more than one event.
 *
 * @return  events not processed
 */
static uint16_t vendor_model_srv_ProcessEvent(uint8_t task_id, uint16_t events)
{
    if(events & VENDOR_MODEL_SRV_INDICATE_EVT)
    {
        adv_ind_send();
        return (events ^ VENDOR_MODEL_SRV_INDICATE_EVT);
    }

    if(events & VENDOR_MODEL_SRV_RSP_TIMEOUT_EVT)
    {
        vendor_srv_sync_handler();
        return (events ^ VENDOR_MODEL_SRV_RSP_TIMEOUT_EVT);
    }

    if(events & VENDOR_MODEL_SRV_TRANS_EVT)
    {
        adv_srv_trans_send();
        return (events ^ VENDOR_MODEL_SRV_TRANS_EVT);
    }
    // Discard unknown events
    return 0;
}

/******************************** endfile @ main ******************************/

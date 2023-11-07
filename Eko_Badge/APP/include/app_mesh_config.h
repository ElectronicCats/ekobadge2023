/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_mesh_config.h
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2021/11/18
 * Description        :
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

#ifndef APP_MESH_CONFIG_H
#define APP_MESH_CONFIG_H

#ifdef __cplusplus
extern "C"
{
#endif
/**************************��������,����ֱ�Ӳο���ͬ����*****************************/

// relay����
#define CONFIG_BLE_MESH_RELAY 1
// ��������
#define CONFIG_BLE_MESH_PROXY 0
// GATT����������
#define CONFIG_BLE_MESH_PB_GATT 0
// FLASH�洢����
#define CONFIG_BLE_MESH_SETTINGS 1
// ���ѽڵ㹦��
#define CONFIG_BLE_MESH_FRIEND 0
// �͹��Ľڵ㹦��
#define CONFIG_BLE_MESH_LOW_POWER 0
// configģ�Ϳͻ��˹���
#define CONFIG_BLE_MESH_CFG_CLI 1
// healthģ�Ϳͻ��˹���
#define CONFIG_BLE_MESH_HLTH_CLI 0

/************************************��������******************************************/

// Net���ݻ������
#define CONFIG_MESH_ADV_BUF_COUNT_MIN (6)
#define CONFIG_MESH_ADV_BUF_COUNT_DEF (10)
#define CONFIG_MESH_ADV_BUF_COUNT_MAX (256)

// RPL���ݻ������,�費С������������֧�ֵ������豸�ڵ����
#define CONFIG_MESH_RPL_COUNT_MIN (6)
#define CONFIG_MESH_RPL_COUNT_DEF (20)
#define CONFIG_MESH_RPL_COUNT_MAX (128)

// RPL����ѭ��ʹ�ã������������нڵ���������RPL���ƣ���NVS�����洢RPL����
#define CONFIG_MESH_ALLOW_RPL_CYCLE (TRUE)

// IV Update State Timer ����96H�ķ�Ƶϵ��
#define CONFIG_MESH_IVU_DIVIDER_MIN (1)
#define CONFIG_MESH_IVU_DIVIDER_DEF (96)
#define CONFIG_MESH_IVU_DIVIDER_MAX (96)

// �����ڰ��������ܴ洢����
#define CONFIG_MESH_PROXY_FILTER_MIN (2)
#define CONFIG_MESH_PROXY_FILTER_DEF (5)
#define CONFIG_MESH_PROXY_FILTER_MAX (20)

// ��Ϣ�������
#define CONFIG_MESH_MSG_CACHE_MIN (3)
#define CONFIG_MESH_MSG_CACHE_DEF (20)
#define CONFIG_MESH_MSG_CACHE_MAX (20)

// ���������
#define CONFIG_MESH_SUBNET_COUNT_MIN (1)
#define CONFIG_MESH_SUBNET_COUNT_DEF (1)
#define CONFIG_MESH_SUBNET_COUNT_MAX (4)

// APP key����
#define CONFIG_MESH_APPKEY_COUNT_MIN (1)
#define CONFIG_MESH_APPKEY_COUNT_DEF (3)
#define CONFIG_MESH_APPKEY_COUNT_MAX (5)

// �ɴ洢��ģ����Կ����
#define CONFIG_MESH_MOD_KEY_COUNT_MIN (1)
#define CONFIG_MESH_MOD_KEY_COUNT_DEF (1)
#define CONFIG_MESH_MOD_KEY_COUNT_MAX (3)

// �ɴ洢�Ķ��ĵ�ַ����
#define CONFIG_MESH_MOD_GROUP_COUNT_MIN (1)
#define CONFIG_MESH_MOD_GROUP_COUNT_DEF (6)
#define CONFIG_MESH_MOD_GROUP_COUNT_MAX (64)

// �Ƿ�����һ�������д���ͬ��ַ�Ľڵ㣨ʹ�ܺ�ְ����ܲ����ã�
#define CONFIG_MESH_ALLOW_SAME_ADDR (FALSE)

// ���ְ���Ϣ֧�ֵĳ��ȣ������˳�������Ҫ�ְ���Ĭ��ֵΪ7��ע��ͬ�����������豸������ͳһ��
#define CONFIG_MESH_UNSEG_LENGTH_MIN (7)
#define CONFIG_MESH_UNSEG_LENGTH_DEF (221)
#define CONFIG_MESH_UNSEG_LENGTH_MAX (221)

// ÿ����Ϣ�����ְ���
#define CONFIG_MESH_TX_SEG_MIN (2)
#define CONFIG_MESH_TX_SEG_DEF (8)
#define CONFIG_MESH_TX_SEG_MAX (32)

// ����ͬʱ���ڵķְ���Ϣ������͸���
#define CONFIG_MESH_TX_SEG_COUNT_MIN (1)
#define CONFIG_MESH_TX_SEG_COUNT_DEF (2)
#define CONFIG_MESH_TX_SEG_COUNT_MAX (4)

// ����ͬʱ���ڵķְ���Ϣ�������ո���
#define CONFIG_MESH_RX_SEG_COUNT_MIN (1)
#define CONFIG_MESH_RX_SEG_COUNT_DEF (2)
#define CONFIG_MESH_RX_SEG_COUNT_MAX (4)

// ÿ�����յķְ���Ϣ������ֽ���
#define CONFIG_MESH_RX_SDU_MIN (12)
#define CONFIG_MESH_RX_SDU_DEF (192)
#define CONFIG_MESH_RX_SDU_MAX (384)

// �����ַ����
#define CONFIG_MESH_LABEL_COUNT_MIN (1)
#define CONFIG_MESH_LABEL_COUNT_DEF (2)
#define CONFIG_MESH_LABEL_COUNT_MAX (4)

// NVS�洢ʹ����������
#define CONFIG_MESH_SECTOR_COUNT_MIN (2)
#define CONFIG_MESH_SECTOR_COUNT_DEF (3)

// NVS�洢������С
#define CONFIG_MESH_SECTOR_SIZE_DEF (4096)

// NVS�洢�׵�ַ
#define CONFIG_MESH_NVS_ADDR_DEF (448 * 1024 + 0x08000000)

// RPL���³������ٴκ�洢
#define CONFIG_MESH_RPL_STORE_RATE_MIN (5)
#define CONFIG_MESH_RPL_STORE_RATE_DEF (60)
#define CONFIG_MESH_RPL_STORE_RATE_MAX (3600)

// SEQ���³������ٴκ�洢
#define CONFIG_MESH_SEQ_STORE_RATE_MIN (5)
#define CONFIG_MESH_SEQ_STORE_RATE_DEF (60)
#define CONFIG_MESH_SEQ_STORE_RATE_MAX (3600)

// ������Ϣ���º�洢�ĳ�ʱʱ��(s)
#define CONFIG_MESH_STORE_RATE_MIN (2)
#define CONFIG_MESH_STORE_RATE_DEF (2)
#define CONFIG_MESH_STORE_RATE_MAX (5)

// ���ѽڵ�֧�ֵ�ÿ����Ϣ�ķְ�����
#define CONFIG_MESH_FRIEND_SEG_RX_COUNT_MIN (1)
#define CONFIG_MESH_FRIEND_SEG_RX_COUNT_DEF (2)
#define CONFIG_MESH_FRIEND_SEG_RX_COUNT_MAX (4)

// ���ѽڵ�֧�ֵĶ��ĸ���
#define CONFIG_MESH_FRIEND_SUB_SIZE_MIN (1)
#define CONFIG_MESH_FRIEND_SUB_SIZE_DEF (4)
#define CONFIG_MESH_FRIEND_SUB_SIZE_MAX (8)

// ���ѽڵ�֧�ֵĵ͹��Ľڵ����
#define CONFIG_MESH_FRIEND_LPN_COUNT_MIN (1)
#define CONFIG_MESH_FRIEND_LPN_COUNT_DEF (1)
#define CONFIG_MESH_FRIEND_LPN_COUNT_MAX (4)

// ���ѽڵ�洢����Ϣ���д�С
#define CONFIG_MESH_QUEUE_SIZE_MIN (2)
#define CONFIG_MESH_QUEUE_SIZE_DEF (4)
#define CONFIG_MESH_QUEUE_SIZE_MAX (30)

// ���ѽڵ���մ��ڴ�С(ms)
#define CONFIG_MESH_FRIEND_RECV_WIN_MIN (1)
#define CONFIG_MESH_FRIEND_RECV_WIN_DEF (30)
#define CONFIG_MESH_FRIEND_RECV_WIN_MAX (255)

// �͹��Ľڵ��������Ϣ���д�С(log2(N)),������Ϊ4�����СΪ2^4=16
#define CONFIG_MESH_LPN_REQ_QUEUE_SIZE_MIN (2)
#define CONFIG_MESH_LPN_REQ_QUEUE_SIZE_DEF (2)
#define CONFIG_MESH_LPN_REQ_QUEUE_SIZE_MAX (4)

// �͹��Ľڵ��������Ϣ���(100ms)
#define CONFIG_MESH_LPN_POLLINTERVAL_MIN (1)
#define CONFIG_MESH_LPN_POLLINTERVAL_DEF (80)
#define CONFIG_MESH_LPN_POLLINTERVAL_MAX (3455999)

// �͹��Ľڵ��������Ϣ��ʱʱ��(100ms)
#define CONFIG_MESH_LPN_POLLTIMEOUT_MIN (10)
#define CONFIG_MESH_LPN_POLLTIMEOUT_DEF (300)
#define CONFIG_MESH_LPN_POLLTIMEOUT_MAX (3455999)

// �͹��Ľڵ�֧�ֵĽ����ӳ�(ms)
#define CONFIG_MESH_LPN_RECV_DELAY_MIN (10)
#define CONFIG_MESH_LPN_RECV_DELAY_DEF (100)
#define CONFIG_MESH_LPN_RECV_DELAY_MAX (255)

// ���ѹ�ϵ�ؽ��ȴ�ʱ��(s)
#define CONFIG_MESH_RETRY_TIMEOUT_MIN (3)
#define CONFIG_MESH_RETRY_TIMEOUT_DEF (10)
#define CONFIG_MESH_RETRY_TIMEOUT_MAX (60)

// ����������֧�ֵ������豸�ڵ����
#define CONFIG_MESH_PROV_NODE_COUNT_DEF (0)

// ADV_RF����
#define CONFIG_MESH_RF_ACCESSADDRESS (0x8E89BED6)
#define CONFIG_MESH_RF_CHANNEL_37 (3)
#define CONFIG_MESH_RF_CHANNEL_38 (16)
#define CONFIG_MESH_RF_CHANNEL_39 (34)

#define APP_DBG(X...)                         \
  if (0)                                    \
  {                                         \
    PRINT("[APP_DBG] <%s> ", __FUNCTION__); \
    PRINT(X);                               \
    PRINT("\r\n");                          \
  }

  /******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif

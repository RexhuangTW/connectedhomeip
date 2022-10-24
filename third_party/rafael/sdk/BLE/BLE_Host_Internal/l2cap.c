/*******************************************************************
 *
 * File Name  : L2CAP.C
 * Description:
 *
 *******************************************************************
 *
 *      Copyright (c) 2020, All Right Reserved
 *      Rafael Microelectronics Co. Ltd.
 *      Taiwan, R.O.C.
 *
 *******************************************************************/

#include <stdint.h>
#include <string.h>
#include "cm3_mcu.h"
#include "sys_arch.h"
#include "mem_mgmt.h"
#include "l2cap.h"
#include "att.h"
#include "smp.h"
#include "ble_att_gatt.h"
#include "host_management.h"
#include "task_hci.h"
#include "task_ble_app.h"
#include "ble_hci.h"
#include "ble_event_module.h"
#include "ble_host_cmd.h"
#include "ble_api.h"

ble_l2cap_buffer_t g_l2cap_buffer;
uint8_t g_l2cap_dataTx[251];

static int8_t Prcss_L2CAP_CID_ATTRIBUTE_PROTOCOL(void)
{
    int8_t err_code;
    ble_le_l2cap_pkt_hdr_t *pkt_hdr;

    pkt_hdr = (ble_le_l2cap_pkt_hdr_t *)g_l2cap_buffer.data;
    err_code = ERR_OK;
    if ((pkt_hdr->opcode & 0xE0) == 0)
    {
        if (pkt_hdr->opcode <= OPCODE_ATT_HANDLE_VALUE_CONFIRMATION)
        {
            err_code = Prcss_BLE_OPCODE_ATT[pkt_hdr->opcode]();
        }
        else
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, pkt_hdr->opcode, 0, ERR_CODE_ATT_REQUEST_NOT_SUPPORTED);
        }
    }
    else
    {
        switch (pkt_hdr->opcode)
        {
        case OPCODE_ATT_WRITE_COMMAND:
            err_code = Prcss_OPCODE_ATT_Write_Command();
            break;

        case OPCODE_ATT_SIGNED_WRITE_COMMAND:
            err_code = Prcss_OPCODE_ATT_Signed_Write_Command();
            break;

        default:
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, pkt_hdr->opcode, 0, ERR_CODE_ATT_REQUEST_NOT_SUPPORTED);
            break;
        }
    }

    return err_code;
}

static int8_t Prcss_L2CAP_CID_LE_L2CAP_SIGNALING_CHANNEL(void)
{
    int8_t err_code;
    uint8_t host_id;
    ble_le_l2cap_pkt_hdr_t *pkt_hdr;
    ACL_Data *ACL_data;
    ACL_Data *ACL_dataTx;

    err_code = ERR_OK;
    pkt_hdr = (ble_le_l2cap_pkt_hdr_t *)g_l2cap_buffer.data;
    ACL_data = (ACL_Data *)g_l2cap_buffer.data;
    ACL_dataTx = (ACL_Data *)g_l2cap_dataTx;
    switch (pkt_hdr->opcode)
    {
    case CODE_SIGNL_CMD_CONNECTION_PARAMETER_UPDATE_RESPONSE:
    {
        uint32_t timeout_base;

        uint8_t host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        if (ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Response.Result == 0x01)
        {
            ble_host_to_app_evt_t *p_queue_param;
            ble_evt_param_t *p_evt_param;

            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
            if (p_queue_param != NULL)
            {
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

                p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
                p_evt_param->event = BLE_GAP_EVT_CONN_PARAM_UPDATE;
                p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.host_id = host_id;
                p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.status = ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Response.Result;
                p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.conn_interval = 0;
                p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.periph_latency = 0;
                p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.supv_timeout = 0;

                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
                bhc_timer_clear(host_id, TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP);
            }
            else
            {
                err_code = ERR_MEM;
            }
        }
        else
        {
            if (bhc_timer_evt_get(host_id, TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP) == TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP)
            {
                bhc_timer_clear(host_id, TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP);

                timeout_base = (LE_Host[host_id].ConnInterval * (LE_Host[host_id].ConnLatency + 1)) << 4;
                bhc_timer_set(host_id, TIMER_EVENT_CONN_UPDATE_COMPLETE, timeout_base); //(ConnInterval*(ConnLatency+1)*160 ms)
            }
        }
    }
    break;

    case CODE_SIGNL_CMD_COMMAND_REJECT:
        break;

    case CODE_SIGNL_CMD_CONNECTION_PARAMETER_UPDATE_REQUEST:
    {
        ble_hci_command_hdr_t *p_hci_msg;
        ble_hci_cmd_conn_updated_param_t *p_hci_data;

        p_hci_msg = NULL;
        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);

        if (LE_Host[host_id].Role == BLE_GAP_ROLE_CENTRAL)
        {
            if ( (ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmin < 6 || ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmin > 3200) ||
                    (ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmax < 6 || ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmax > 3200) ||
                    (ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmin > ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmax) ||
                    (ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.SlaveLatency > 500) ||
                    (ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.TimeoutMultiplier < 10 || ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.TimeoutMultiplier > 3200) ||
                    ((ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.TimeoutMultiplier << 2) < ((1 + ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.SlaveLatency) * ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmax )))
            {
                ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Response.Result = 0x01;
            }
            else
            {
                ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Response.Result = 0x00;

                p_hci_msg = mem_malloc(sizeof(ble_hci_command_hdr_t) + sizeof(ble_hci_cmd_conn_updated_param_t));
                if (p_hci_msg != NULL)
                {
                    p_hci_msg->transport_id = BLE_TRANSPORT_HCI_COMMAND;
                    p_hci_msg->ocf = LE_CONNECTION_UPDATE;
                    p_hci_msg->ogf = HCI_CMD_LE_CONTROLLER_CMD;
                    p_hci_msg->length = sizeof(ble_hci_cmd_conn_updated_param_t);
                    p_hci_data = (ble_hci_cmd_conn_updated_param_t *)p_hci_msg->parameter;
                    p_hci_data->conn_handle = g_l2cap_buffer.conn_handle;
                    memcpy((uint8_t *)&p_hci_data->conn_interval_min, (uint8_t *)&ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.Intervalmin, 8);
                    p_hci_data->min_celength = 0;
                    p_hci_data->max_celength = 0;
                }
                else
                {
                    err_code = ERR_MEM;
                }
            }
            if (err_code != ERR_MEM)
            {
                ACL_dataTx->Length_PDU = 6;
                ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Response.Code = CODE_SIGNL_CMD_CONNECTION_PARAMETER_UPDATE_RESPONSE;
                ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Response.Identifier = ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.identifier;
                ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Response.Length = 2;
                err_code = setBLE_ConnTxData(g_l2cap_buffer.conn_handle, (uint8_t *)ACL_dataTx, (ACL_dataTx->Length_PDU + SIZE_BASIC_L2CAP_HEADER));
                if (p_hci_msg != NULL)
                {
                    if (err_code == ERR_OK)
                    {
                        hci_task_common_queue_t hci_comm_msg;

                        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
                        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_msg;
                        sys_queue_send(&g_hci_common_handle, (void *)&hci_comm_msg);
                    }
                    else
                    {
                        mem_free(p_hci_msg);
                    }
                }
            }
        }
        else
        {
            ACL_dataTx->Length_PDU = 6;
            ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Command_Reject_Response.code = CODE_SIGNL_CMD_COMMAND_REJECT;
            ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Command_Reject_Response.identifier = ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.identifier;
            ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Command_Reject_Response.length = 2;
            ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Command_Reject_Response.reason = 0;
            err_code = setBLE_ConnTxData(g_l2cap_buffer.conn_handle, (uint8_t *)ACL_dataTx, (ACL_dataTx->Length_PDU + SIZE_BASIC_L2CAP_HEADER));
        }
    }
    break;

    default:
        ACL_dataTx->Length_PDU = 6;
        ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Command_Reject_Response.code = CODE_SIGNL_CMD_COMMAND_REJECT;
        ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Command_Reject_Response.identifier = ACL_data->PDU.L2CAPR_SIGNL_OP_Connection_Parameter_Update_Request.identifier;
        ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Command_Reject_Response.length = 2;
        ACL_dataTx->PDU.L2CAPR_SIGNL_OP_Command_Reject_Response.reason = 0;
        err_code = setBLE_ConnTxData(g_l2cap_buffer.conn_handle, (uint8_t *)ACL_dataTx, (ACL_dataTx->Length_PDU + SIZE_BASIC_L2CAP_HEADER));
        break;
    }

    return err_code;
}

static int8_t Prcss_L2CAP_CID_SECURITY_MANAGER_PROTOCOL(void)
{
    int8_t err_code;
    ble_l2cap_pkt_hdr_t *l2cap_header;

    err_code = ERR_OK;
    l2cap_header = (ble_l2cap_pkt_hdr_t *)g_l2cap_buffer.data;
    if (l2cap_header->code <= CODE_SMP_MAX_HANDLE_RANGE)
    {
        err_code = Prcss_BLE_CODE_SMP[l2cap_header->code]();
    }

    return err_code;
}

//=============================================================================
//                Global Functions
//=============================================================================
int8_t bhc_host_prcss_l2cap_cid(uint16_t conn_handle, uint8_t *p_data, uint16_t length)
{
    int8_t err_code;
    ble_l2cap_pkt_hdr_t *l2cap_header;

    err_code = ERR_OK;
    l2cap_header = (ble_l2cap_pkt_hdr_t *)g_l2cap_buffer.data;
    g_l2cap_buffer.conn_handle = conn_handle;
    memcpy(g_l2cap_buffer.data, p_data, length);
    memcpy(g_l2cap_dataTx, g_l2cap_buffer.data, length);

    switch (l2cap_header->cid)
    {
    case L2CAP_CID_NULL_IDENTIFIER:
    case L2CAP_CID_L2CAP_SIGNALING_CHANNEL:
    case L2CAP_CID_CONNECTIONLESS_CHANNEL:
    case L2CAP_CID_AMP_MANAGER_PROTOCOL:
    case L2CAP_CID_BREDR_SECURITY_MANAGER:
        break;

    case L2CAP_CID_ATTRIBUTE_PROTOCOL:
        err_code = Prcss_L2CAP_CID_ATTRIBUTE_PROTOCOL();
        break;

    case L2CAP_CID_LE_L2CAP_SIGNALING_CHANNEL:
        err_code = Prcss_L2CAP_CID_LE_L2CAP_SIGNALING_CHANNEL();
        break;

    case L2CAP_CID_SECURITY_MANAGER_PROTOCOL:
        err_code = Prcss_L2CAP_CID_SECURITY_MANAGER_PROTOCOL();
        break;

    default:
        break;
    }

    return err_code;
}

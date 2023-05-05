/*******************************************************************
 *
 * File Name  : ATT.C
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
#include "l2cap.h"
#include "att.h"
#include "smp.h"
#include "host_management.h"
#include "ble_att_gatt.h"
#include "ble_uuid.h"
#include "lib_config.h"
#include "task_hci.h"
#include "task_ble_app.h"
#include "ble_event_module.h"
#include "ble_host_ref.h"
#include "ble_hci.h"
#include "ble_bonding.h"
#include "ble_host_cmd.h"
#include "ble_api.h"
#include "ble_printf.h"
#include "ble_memory.h"

uint8_t acl_data_buff[251];
const uint16_t UUID_BASE_BLUETOOTH[8] =
    {
        0x34FB, 0x5F9B, // 0000xxxx, the 16-bit Attribute UUID replaces the x's,
        0x0080, 0x8000, // For example, the 16-bit Attribute UUID of 0x1234 is equivalent to
        0x1000, 0x0000, // the 128-bit UUID of 0x00001234-0000-1000-8000-00805F9B34FB
        0x0000, 0x0000  // Bluetooth Spec., Vol.3, Part F, 3.1
};

int8_t Prcss_OPCODE_ATT_NULL(void);
int8_t Prcss_OPCODE_ATT_Error_Response(void);
int8_t Prcss_OPCODE_ATT_Exchange_MTU_Request(void);
int8_t Prcss_OPCODE_ATT_Exchange_MTU_Response(void);
int8_t Prcss_OPCODE_ATT_Find_Information_Request(void);
int8_t Prcss_OPCODE_ATT_Find_Information_Response(void);
int8_t Prcss_OPCODE_ATT_Find_By_Type_Value_Request(void);
int8_t Prcss_OPCODE_ATT_Find_By_Type_Value_Response(void);
int8_t Prcss_OPCODE_ATT_Read_By_Type_Request(void);
int8_t Prcss_OPCODE_ATT_Read_By_Type_Response(void);
int8_t Prcss_OPCODE_ATT_Read_Request(void);
int8_t Prcss_OPCODE_ATT_Read_Response(void);
int8_t Prcss_OPCODE_ATT_Read_Blob_Request(void);
int8_t Prcss_OPCODE_ATT_Read_Blob_Response(void);
int8_t Prcss_OPCODE_ATT_Read_Multiple_Request(void);
int8_t Prcss_OPCODE_ATT_Read_Multiple_Response(void);
int8_t Prcss_OPCODE_ATT_Read_by_Group_Type_Request(void);
int8_t Prcss_OPCODE_ATT_Read_by_Group_Type_Response(void);
int8_t Prcss_OPCODE_ATT_Write_Request(void);
int8_t Prcss_OPCODE_ATT_Write_Response(void);
int8_t Prcss_OPCODE_ATT_Prepare_Write_Request(void);
int8_t Prcss_OPCODE_ATT_Prepare_Write_Response(void);
int8_t Prcss_OPCODE_ATT_Execute_Write_Request(void);
int8_t Prcss_OPCODE_ATT_Execute_Write_Response(void);
int8_t Prcss_OPCODE_ATT_Handle_Value_Notification(void);
int8_t Prcss_OPCODE_ATT_Handle_Value_Indication(void);
int8_t Prcss_OPCODE_ATT_Handle_Value_Confirmation(void);
int8_t Prcss_OPCODE_ATT_Read_Multiple_Variable_Request(void);
int8_t Prcss_OPCODE_ATT_Read_Multiple_Variable_Response(void);
int8_t Prcss_OPCODE_ATT_Multiple_Handle_Value_Notification(void);
int8_t Prcss_OPCODE_ATT_Write_Command(void);
int8_t Prcss_OPCODE_ATT_Signed_Write_Command(void);

int8_t (*const Prcss_BLE_OPCODE_ATT[])(void) =
    {
        Prcss_OPCODE_ATT_NULL,
        Prcss_OPCODE_ATT_Error_Response,
        Prcss_OPCODE_ATT_Exchange_MTU_Request,
        Prcss_OPCODE_ATT_Exchange_MTU_Response,
        Prcss_OPCODE_ATT_Find_Information_Request,
        Prcss_OPCODE_ATT_Find_Information_Response,
        Prcss_OPCODE_ATT_Find_By_Type_Value_Request,
        Prcss_OPCODE_ATT_Find_By_Type_Value_Response,
        Prcss_OPCODE_ATT_Read_By_Type_Request,
        Prcss_OPCODE_ATT_Read_By_Type_Response,
        Prcss_OPCODE_ATT_Read_Request,
        Prcss_OPCODE_ATT_Read_Response,
        Prcss_OPCODE_ATT_Read_Blob_Request,
        Prcss_OPCODE_ATT_Read_Blob_Response,
        Prcss_OPCODE_ATT_Read_Multiple_Request,
        Prcss_OPCODE_ATT_Read_Multiple_Response,
        Prcss_OPCODE_ATT_Read_by_Group_Type_Request,
        Prcss_OPCODE_ATT_Read_by_Group_Type_Response,
        Prcss_OPCODE_ATT_Write_Request,
        Prcss_OPCODE_ATT_Write_Response,
        Prcss_OPCODE_ATT_NULL,
        Prcss_OPCODE_ATT_NULL,
        Prcss_OPCODE_ATT_Prepare_Write_Request,
        Prcss_OPCODE_ATT_Prepare_Write_Response,
        Prcss_OPCODE_ATT_Execute_Write_Request,
        Prcss_OPCODE_ATT_Execute_Write_Response,
        Prcss_OPCODE_ATT_NULL,
        Prcss_OPCODE_ATT_Handle_Value_Notification,
        Prcss_OPCODE_ATT_NULL,
        Prcss_OPCODE_ATT_Handle_Value_Indication,
        Prcss_OPCODE_ATT_Handle_Value_Confirmation,
        Prcss_OPCODE_ATT_NULL,
        Prcss_OPCODE_ATT_Read_Multiple_Variable_Request,
        Prcss_OPCODE_ATT_Read_Multiple_Variable_Response,
        Prcss_OPCODE_ATT_NULL,
        Prcss_OPCODE_ATT_Multiple_Handle_Value_Notification,
};

int8_t setBLE_ConnTxData(uint16_t connID, uint8_t *ScanRspData, uint8_t Length);
int8_t ATT_Request(uint16_t connID, uint8_t AttOpcode, uint8_t *paramGroup, uint16_t dataLength);
int8_t ATT_Request_ERROR_RESPONSE(uint16_t connID, uint8_t ReqOpcode_inError, uint16_t AttHandle_inError, uint8_t ErrorCode);

int8_t bhc_att_req(uint16_t conn_id, uint8_t att_opcode, uint16_t handle_num, uint8_t *p_data, uint16_t length);
int8_t bhc_att_error_rsp_req(uint16_t conn_id, uint8_t att_opcode, uint16_t handle_num, uint8_t error_code);
int8_t bhc_acl_data_send(uint16_t connID, uint8_t *ScanRspData, uint8_t Length);

uint8_t chkATT_PERMISSION_Read(uint8_t host_id, uint16_t ATT_Handle);
uint8_t chkATT_PERMISSION_Write(uint8_t host_id, uint16_t ATT_Handle);

int8_t Prcss_OPCODE_ATT_NULL(void)
{
    return ERR_OK;
}

int8_t Prcss_OPCODE_ATT_Error_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ble_att_handle_param_t *p_att_hdl_db_mapping_para;
    uint8_t i, host_id;
    uint16_t numHDL, size_DB_Mapping;
    int8_t err_code;

    err_code = ERR_OK;
    ACL_data = (ACL_Data *)g_l2cap_buffer.data;
    host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_att_hdl_db_mapping_para = att_db_mapping[host_id].map_client_db;
    size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;
    p_host_current = &LE_Host[host_id];

    switch (ACL_data->PDU.ATTR_OP_Error_Response.Req_Opcode)
    {
    case OPCODE_ATT_READ_BY_TYPE_REQUEST:
        switch (ACL_data->PDU.ATTR_OP_Error_Response.ErrorCode)
        {
        case ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND:
            p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = ACL_data->PDU.ATTR_OP_Error_Response.Attr_HandleInError + 1;
            p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = ACL_data->PDU.ATTR_OP_Error_Response.Attr_HandleInError + 1;
            p_host_current->state_DB_parse = HS_DBP_S1;
            if (p_host_current->state_authe == HS_ATE_S0)
            {
                if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_READ_BY_GROUP_TYPE_REQUEST, (uint8_t *)&LE_Host[host_id].mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting, 6) != ERR_OK)
                {
                    HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
                }
            }
            break;

        default:
            break;
        }
        break;

    case OPCODE_ATT_READ_BY_GROUP_TYPE_REQUEST:
        switch (ACL_data->PDU.ATTR_OP_Error_Response.ErrorCode)
        {
        case ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND:
        {
            ble_host_to_app_evt_t *p_queue_param;
            ble_evt_param_t *p_evt_param;

            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
            if (p_queue_param != NULL)
            {
                // DB parsing ok.
                p_host_current->state_DB_parse = HS_DBP_S0;

                // release resource.
                bhc_host_release_db_parsing_resource(host_id);

                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

                p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
                p_evt_param->event = BLE_ATT_GATT_EVT_DB_PARSE_COMPLETE;
                p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_db_parse_complete.host_id = host_id;
                p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_db_parse_complete.result = ERR_CODE_ATT_NO_ERROR;
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                bhc_timer_clear(host_id, TIMER_EVENT_CLIENT_DB_PARSING);
            }
            else
            {
                err_code = ERR_MEM;
            }
        }
        break;

        default:
            break;
        }
        break;

    case OPCODE_ATT_READ_REQUEST:
    case OPCODE_ATT_READ_BLOB_REQUEST:
    case OPCODE_ATT_WRITE_REQUEST:
    case OPCODE_ATT_PREPARE_WRITE_REQUEST:
    case OPCODE_ATT_EXECUTE_WRITE_REQUEST:
        if (p_host_current->numHdl_fc_cl)
        {
            numHDL = p_host_current->numHdl_fc_cl;
            if (numHDL == ACL_data->PDU.ATTR_OP_Error_Response.Attr_HandleInError)
            {
                for (i = 0; i < size_DB_Mapping; i++)
                {
                    if (p_att_hdl_db_mapping_para[i].handle_num == numHDL)
                    {
                        break;
                    }
                }
                if (i < size_DB_Mapping)
                {
                    ble_host_to_app_evt_t *p_queue_param;
                    ble_evt_att_param_t *pt_evt;

                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 1));
                    if (p_queue_param != NULL)
                    {
                        p_queue_param->u32_send_systick = sys_now();
                        p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                        pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                        pt_evt->host_id = host_id;
                        pt_evt->gatt_role = BLE_GATT_ROLE_CLIENT;
                        pt_evt->cb_index = i;
                        pt_evt->handle_num = numHDL;
                        pt_evt->opcode = OPCODE_ATT_ERROR_RESPONSE;
                        pt_evt->event = 0;
                        pt_evt->length = (ACL_data->Length_PDU - 1);
                        memcpy(pt_evt->data, &ACL_data->PDU.ATTR_OP_Error_Response.Req_Opcode, pt_evt->length);
                        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
                    }
                    else
                    {
                        err_code = ERR_MEM;
                    }
                }
                if (err_code == ERR_OK)
                {
                    p_host_current->numHdl_fc_cl = 0;
                }
            }
        }
        break;

    default:
        break;
    }

    return err_code;
#else
    return ERR_OK;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Exchange_MTU_Request(void)
{
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ble_evt_param_t *p_evt_param;
    uint16_t mtu;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];

    ACL_data = (ACL_Data *)g_l2cap_buffer.data;

    mtu = ACL_data->PDU.ATTR_OP_Exchange_MTU_Request.MTU_ClientRx;
    if (p_host_current->att_MTU_planed)
    {
        if (mtu > p_host_current->att_MTU_planed)
        {
            mtu = p_host_current->att_MTU_planed;
        }
        p_host_current->att_MTU = mtu;
    }
    err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_EXCHANGE_MTU_RESPONSE, (uint8_t *)&p_host_current->att_MTU, 2);
    if (err_code == ERR_OK)
    {
        ble_host_to_app_evt_t *p_queue_param;

        p_host_current->state_att_MTU_opr |= STATE_ATT_MTU_OPR_MSK_RSP_RDY;

        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        while (p_queue_param == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
            sys_task_delay(2);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        }
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        p_evt_param->event = BLE_ATT_GATT_EVT_MTU_EXCHANGE;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_mtu.host_id = host_id;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_mtu.mtu = mtu;
        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
    }

    return err_code;
}

int8_t Prcss_OPCODE_ATT_Exchange_MTU_Response(void)
{
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ble_evt_param_t *p_evt_param;
    ble_host_to_app_evt_t *p_queue_param;
    uint16_t mtu;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];

    ACL_data = (ACL_Data *)g_l2cap_buffer.data;

    mtu = ACL_data->PDU.ATTR_OP_Exchange_MTU_Response.MTU_ServerRx;
    if (mtu > p_host_current->att_MTU_planed)
    {
        mtu = p_host_current->att_MTU_planed;
    }
    if (mtu)
    {
        p_host_current->att_MTU = mtu;
    }
    p_host_current->state_att_MTU_opr |= STATE_ATT_MTU_OPR_MSK_REQ_RDY;

    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
    if (p_queue_param != NULL)
    {
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        p_evt_param->event = BLE_ATT_GATT_EVT_MTU_EXCHANGE;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_mtu.host_id = host_id;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_mtu.mtu = mtu;
        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

int8_t Prcss_OPCODE_ATT_Find_Information_Request(void)
{
#if (_HOST_SERVER_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ACL_Data *ACL_dataTx;
    uint8_t host_id, Rsp_Error, attType, Len_AttType;
    uint16_t end_DB_Mapping, idx, DataAccu;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        ACL_dataTx = (ACL_Data *)g_l2cap_dataTx;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];

        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        idx = ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting;
        if (ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting <= ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending)
        {
            if (ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting != 0)
            {
                if (att_db_mapping_size[host_id].size_map_server_db == 0)
                {
                    Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
                }
                else
                {
                    end_DB_Mapping = att_db_mapping_size[host_id].size_map_server_db - 1;
                    if (end_DB_Mapping > ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending)
                    {
                        end_DB_Mapping = ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending;
                    }
                    if (idx > end_DB_Mapping)
                    {
                        Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
                    }
                    else
                    {
                        Rsp_Error = ERR_CODE_ATT_NO_ERROR;
                    }
                }
            }
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_FIND_INFORMATION_REQUEST, idx, Rsp_Error);
            break;
        }

        DataAccu = 0;
        attType = (att_db_link[host_id].p_server_db[idx]->db_permission_format & ATT_TYPE_FORMAT_16UUID);
        if (attType == ATT_TYPE_FORMAT_16UUID)
        {
            ACL_dataTx->PDU.ATTR_OP_Find_Information_Response.Type_Format = ATT_RSP_FORMAT_16UUID;
            Len_AttType = 2 + 2;
        }
        else
        {
            ACL_dataTx->PDU.ATTR_OP_Find_Information_Response.Type_Format = ATT_RSP_FORMAT_128UUID;
            Len_AttType = 16 + 2;
        }
        while (1)
        {
            if ((att_db_link[host_id].p_server_db[idx]->db_permission_format & ATT_TYPE_FORMAT_16UUID) != attType)
            {
                break;
            }
            memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Find_Information_Response.Info.Data[DataAccu], (uint8_t *)&idx, 2);
            memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Find_Information_Response.Info.Data[DataAccu + 2], (uint8_t *)att_db_link[host_id].p_server_db[idx]->p_uuid_type, Len_AttType);
            DataAccu += (Len_AttType);
            if ((DataAccu + Len_AttType + 2) > p_host_current->att_MTU)
            {
                break;
            }
            if (idx == end_DB_Mapping)
            {
                break;
            }
            idx++;
        }

        err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_FIND_INFORMATION_RESPONSE, &ACL_dataTx->PDU.ATTR_OP_Find_Information_Response.Type_Format, (DataAccu + 1));
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Find_Information_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    Pairs_Handle_typeUUID16 *RspData_DataPair;
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    uint16_t i, j;
    uint8_t host_id, len_check, evtGen;

    ACL_data = (ACL_Data *)g_l2cap_buffer.data;
    RspData_DataPair = ACL_data->PDU.ATTR_OP_Find_Information_Response.Info.Pairs_Handle_typeUUID16;

    host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];

    len_check = 0;
    for (i = 0; i < 3; i++)
    {
        if (p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i])
        {
            if (p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i])
            {
                if (ACL_data->PDU.ATTR_OP_Find_Information_Response.Type_Format == ATT_TYPE_FORMAT_16UUID)
                {
                    if ((ACL_data->PDU.ATTR_OP_Find_Information_Response.Info.Pairs_Handle_typeUUID16[0].uuid == GATT_DECL_CHARACTERISTIC) || (ACL_data->PDU.ATTR_OP_Find_Information_Response.Info.Pairs_Handle_typeUUID16[0].uuid == GATT_DECL_PRIMARY_SERVICE))
                    {
                        p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[i] = 0;
                        p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i] = 0;
                    }
                    else
                    {
                        len_check = 2;
                    }
                }
                else
                {
                    len_check = 16;
                }
                break;
            }
        }
    }
    if (len_check)
    {
        for (j = p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i]; j <= p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[i]; j++)
        {
            if (memcmp((uint8_t *)att_db_link[host_id].p_client_db[j]->p_uuid_type, (uint8_t *)&RspData_DataPair->uuid, len_check) == 0)
            {
                att_db_mapping[host_id].map_client_db[j].handle_num = RspData_DataPair->Attr_Handle;
                att_db_mapping[host_id].map_client_db[j].property_value = att_db_link[host_id].p_client_db[j]->property_value;
            }
        }
        if (p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i] == p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[i])
        {
            p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i] = 0;
            p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[i] = 0;
        }
        else
        {
            p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i]++;
        }
    }
    evtGen = 0;
    for (i = 0; i < 3; i++)
    {
        if (p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i])
        {
            if (p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i])
            {
                p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB = p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i];
                p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB = p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i];
                if (p_host_current->state_authe == HS_ATE_S0)
                {
                    if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_FIND_INFORMATION_REQUEST, (uint8_t *)&p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB, 4) != ERR_OK)
                    {
                        evtGen = 1;
                    }
                }
                break;
            }
        }
    }
    if (i == 3)
    {
        p_host_current->state_DB_parse = HS_DBP_S2;
        if (p_host_current->state_authe == HS_ATE_S0)
        {
            if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_READ_BY_TYPE_REQUEST, (uint8_t *)&p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting, 6) != ERR_OK)
            {
                evtGen = 1;
            }
        }
    }
    if (evtGen)
    {
        HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
    }

    return ERR_OK;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Find_By_Type_Value_Request(void)
{
#if (_HOST_SERVER_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ACL_Data *ACL_dataTx;
    ble_att_param_t *ATT_SERVER_content;
    uint8_t host_id;
    uint16_t end_DB_Mapping, idx, DataAccu;
    uint8_t Rsp_Error, IsFound, Len_AttType;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        ACL_dataTx = (ACL_Data *)g_l2cap_dataTx;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];

        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        idx = ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting;
        if (ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting <= ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending)
        {
            if (ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting != 0)
            {
                if (att_db_mapping_size[host_id].size_map_server_db == 0)
                {
                    Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
                }
                else
                {
                    end_DB_Mapping = att_db_mapping_size[host_id].size_map_server_db - 1;
                    if (end_DB_Mapping > ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending)
                    {
                        end_DB_Mapping = ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending;
                    }
                    if (idx > end_DB_Mapping)
                    {
                        Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
                    }
                    else
                    {
                        Rsp_Error = ERR_CODE_ATT_NO_ERROR;
                    }
                }
            }
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_FIND_BY_TYPE_VALUE_REQUEST, idx, Rsp_Error);
            break;
        }

        if (((uint8_t *)&ACL_data->PDU.ATTR_OP_Find_By_Type_Value_Request.Attr_Type)[1] != 0x28)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_FIND_BY_TYPE_VALUE_REQUEST, idx, ERR_CODE_ATT_REQUEST_NOT_SUPPORTED);
            break;
        }

        DataAccu = 0;
        Len_AttType = ACL_data->Length_PDU - 7; // Attribute Opcode:1, Starting and Ending Handle:4, Attribute Type:2
        IsFound = 0;
        while (1)
        {
            while (1)
            {
                ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_server_db[idx]);

                if (ACL_data->PDU.ATTR_OP_Find_By_Type_Value_Request.Attr_Type == *((uint16_t *)ATT_SERVER_content->p_uuid_type))
                {
                    if ((ATT_SERVER_content->db_permission_format & ATT_TYPE_FORMAT_16UUID) == ATT_TYPE_FORMAT_16UUID)
                    {
                        if (memcmp((uint8_t *)ACL_data->PDU.ATTR_OP_Find_By_Type_Value_Request.Attr_Value, (uint8_t *)ATT_SERVER_content->p_uuid_value, Len_AttType) == 0)
                        {
                            memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Find_By_Type_Value_Response.Hdl_Info_List[DataAccu], (uint8_t *)&idx, 2);
                            DataAccu += 2;
                            IsFound = 1;
                            break;
                        }
                    }
                }
                if (idx == end_DB_Mapping)
                {
                    break;
                }
                idx++;
            }
            while (1)
            {
                if (idx == end_DB_Mapping)
                {
                    if (IsFound)
                    {
                        IsFound = 0;
                        memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Find_By_Type_Value_Response.Hdl_Info_List[DataAccu], (uint8_t *)&idx, 2);
                        DataAccu += 2;
                    }
                    break;
                }
                memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Find_By_Type_Value_Response.Hdl_Info_List[DataAccu], (uint8_t *)&idx, 2);
                idx++;
                if (ACL_data->PDU.ATTR_OP_Find_By_Type_Value_Request.Attr_Type == *((uint16_t *)att_db_link[host_id].p_server_db[idx]->p_uuid_type))
                {
                    IsFound = 0;
                    DataAccu += 2;
                    break;
                }
            }
            if ((DataAccu + (1 + 4 + 2)) > p_host_current->att_MTU)
            {
                break;
            }
            if (idx == end_DB_Mapping)
            {
                break;
            }
        }
        if (DataAccu)
        {
            err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_FIND_BY_TYPE_VALUE_RESPONSE, (uint8_t *)ACL_dataTx->PDU.ATTR_OP_Find_By_Type_Value_Response.Hdl_Info_List, DataAccu);
            break;
        }
        else
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_FIND_BY_TYPE_VALUE_REQUEST, ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting, ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND);
            break;
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Find_By_Type_Value_Response(void)
{
    return ERR_OK;
}

int8_t Prcss_OPCODE_ATT_Read_By_Type_Request(void)
{
#if (_HOST_SERVER_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ACL_Data *ACL_dataTx;
    ble_att_param_t *ATT_SERVER_content;
    uint16_t i16, end_DB_Mapping, idx, DataAccu;
    uint8_t host_id, Rsp_Error, attType, Len_AttType, length_pair;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        ACL_dataTx = (ACL_Data *)g_l2cap_dataTx;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];

        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        idx = ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting;
        if (ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting <= ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending)
        {
            if (ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting != 0)
            {
                if (att_db_mapping_size[host_id].size_map_server_db == 0)
                {
                    Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
                }
                else
                {
                    end_DB_Mapping = att_db_mapping_size[host_id].size_map_server_db - 1;
                    if (end_DB_Mapping > ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending)
                    {
                        end_DB_Mapping = ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending;
                    }
                    if (idx > end_DB_Mapping)
                    {
                        Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
                    }
                    else
                    {
                        Rsp_Error = ERR_CODE_ATT_NO_ERROR;
                    }
                }
            }
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BY_TYPE_REQUEST, idx, Rsp_Error);
            break;
        }

        DataAccu = 0;
        attType = 0;
        Len_AttType = ACL_data->Length_PDU - 5;
        if (Len_AttType == 2)
        {
            attType = ATT_TYPE_FORMAT_16UUID;
        }

        length_pair = 0;
        Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
        while (1)
        {
            ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_server_db[idx]);
            if ((ATT_SERVER_content->db_permission_format & ATT_TYPE_FORMAT_16UUID) == attType)
            {
                if (memcmp((uint8_t *)ACL_data->PDU.ATTR_OP_Read_By_Type_Request.typeUUID, (uint8_t *)ATT_SERVER_content->p_uuid_type, Len_AttType) == 0)
                {
                    Rsp_Error = chkATT_PERMISSION_Read(host_id, idx);
                    if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
                    {
                        break;
                    }
                    i16 = 0;
                    memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_By_Type_Response.Attr_Data_pair[DataAccu], (uint8_t *)&idx, 2);
                    if ((ATT_SERVER_content->db_permission_format & ATT_TYPE_FORMAT_16UUID))
                    {
                        if (*((uint16_t *)ATT_SERVER_content->p_uuid_type) == GATT_DECL_CHARACTERISTIC)
                        {
                            i16 = idx + 1;
                        }
                    }
                    if (i16 != 0)
                    {
                        ACL_dataTx->PDU.ATTR_OP_Read_By_Type_Response.Attr_Data_pair[DataAccu + 2] = att_db_link[host_id].p_server_db[i16]->property_value;
                        memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_By_Type_Response.Attr_Data_pair[DataAccu + 3], (uint8_t *)&i16, 2);
                        DataAccu += 3;
                        i16 = 3;
                    }
                    else
                    {
                        ble_host_to_app_evt_t *p_queue_param;
                        ble_evt_att_param_t *pt_evt;

                        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t));
                        if (p_queue_param != NULL)
                        {
                            p_queue_param->u32_send_systick = sys_now();
                            p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                            pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                            pt_evt->host_id = host_id;
                            pt_evt->gatt_role = BLE_GATT_ROLE_SERVER;
                            pt_evt->cb_index = idx;
                            pt_evt->handle_num = idx;
                            pt_evt->opcode = OPCODE_ATT_READ_BY_TYPE_REQUEST;
                            pt_evt->event = 0;
                            pt_evt->length = 0;
                            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                            if (ATT_SERVER_content->att_len == 0)
                            {
                                break;
                            }
                        }
                        else
                        {
                            err_code = ERR_MEM;
                            break;
                        }
                    }
                    memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_By_Type_Response.Attr_Data_pair[DataAccu + 2], (uint8_t *)ATT_SERVER_content->p_uuid_value, ATT_SERVER_content->att_len);
                    i16 += (ATT_SERVER_content->att_len + 2);
                    DataAccu += (ATT_SERVER_content->att_len + 2);
                    if (length_pair == 0)
                    {
                        length_pair = DataAccu;
                        ACL_dataTx->PDU.ATTR_OP_Read_By_Type_Response.length_pair = DataAccu;
                    }
                    else
                    {
                        if (i16 != length_pair)
                        {
                            DataAccu -= i16;
                            break;
                        }
                    }
                }
            }
            if ((DataAccu + length_pair + 2) > p_host_current->att_MTU)
            {
                break;
            }
            if (idx == end_DB_Mapping)
            {
                break;
            }
            idx++;
        }

        if (err_code == ERR_OK)
        {
            if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
            {
                if (DataAccu)
                {
                    err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BY_TYPE_RESPONSE, &ACL_dataTx->PDU.ATTR_OP_Read_By_Type_Response.length_pair, (DataAccu + 1));
                }
            }
            else
            {
                err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BY_TYPE_REQUEST, ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting, Rsp_Error);
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Read_By_Type_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    ATTR_OP_VALUE_DataPair_UUID16_CHARACTERISTIC *RspData_DataPair;
    ble_att_param_t *ATT_SERVER_content;
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    uint16_t i, j;
    uint8_t host_id, Idx_RspData_Pair, len_check, idx_DB_CharcGrp;

    Idx_RspData_Pair = 0;
    idx_DB_CharcGrp = 0;
    ACL_data = (ACL_Data *)g_l2cap_buffer.data;
    RspData_DataPair = (ATTR_OP_VALUE_DataPair_UUID16_CHARACTERISTIC *)&ACL_data->PDU.ATTR_OP_Read_By_Type_Response.Attr_Data_pair[Idx_RspData_Pair];

    host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];
    for (i = 0; i < 3; i++)
    {
        p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i] = 0;
        p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[i] = 0;
        p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i] = 0;
        p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[i] = 0;
    }

    while (1)
    {
        for (i = p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB; i < p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB; i++)
        {
            ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_client_db[i]);

            if ((*((uint16_t *)ATT_SERVER_content->p_uuid_type) == GATT_DECL_CHARACTERISTIC) && (att_db_mapping[host_id].map_client_db[i].handle_num == 0))
            {
                len_check = ACL_data->PDU.ATTR_OP_Read_By_Type_Response.length_pair - 5;
                if (len_check != (ATT_SERVER_content->att_len))
                {
                    len_check = 0;
                }
                if (len_check)
                {
                    if (memcmp((uint8_t *)att_db_link[host_id].p_client_db[i + 1]->p_uuid_type, (uint8_t *)&RspData_DataPair->typeUUID, len_check) == 0)
                    {
                        att_db_mapping[host_id].map_client_db[i].handle_num = RspData_DataPair->Hdl;
                        att_db_mapping[host_id].map_client_db[i].property_value = ATT_SERVER_content->property_value;
                        i++;
                        if (att_db_link[host_id].p_client_db[i]->property_value & RspData_DataPair->Property)
                        {
                            att_db_mapping[host_id].map_client_db[i].handle_num = RspData_DataPair->Hdl_Value;
                            att_db_mapping[host_id].map_client_db[i].property_value = RspData_DataPair->Property;
                        }
                        if (RspData_DataPair->Hdl_Value != p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Ending)
                        {
                            p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[idx_DB_CharcGrp] = RspData_DataPair->Hdl_Value + 1;
                            p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[idx_DB_CharcGrp] = p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Ending;
                            for (j = i; j < p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB; j++)
                            {
                                if ((*((uint16_t *)att_db_link[host_id].p_client_db[j]->p_uuid_type) == GATT_DECL_PRIMARY_SERVICE) || (*((uint16_t *)att_db_link[host_id].p_client_db[j]->p_uuid_type) == GATT_DECL_CHARACTERISTIC))
                                {
                                    break;
                                }
                            }
                            j--;
                            if (j != i)
                            {
                                p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[idx_DB_CharcGrp] = i;
                                p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[idx_DB_CharcGrp] = j;
                            }
                        }
                        idx_DB_CharcGrp++;
                        break;
                    }
                }
            }
        }
        Idx_RspData_Pair += ACL_data->PDU.ATTR_OP_Read_By_Type_Response.length_pair;

        if (ACL_data->Length_PDU > SIZE_DEFAULT_ATT_MTU)
        {
            ACL_data->Length_PDU = SIZE_DEFAULT_ATT_MTU;
        }
        if ((ACL_data->Length_PDU) > (Idx_RspData_Pair + 2))
        {
            RspData_DataPair = (struct ATTR_OP_VALUE_DataPair_UUID16_CHARACTERISTIC *)&ACL_data->PDU.ATTR_OP_Read_By_Type_Response.Attr_Data_pair[Idx_RspData_Pair];
            if (p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[idx_DB_CharcGrp - 1] == RspData_DataPair->Hdl)
            {
                p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[idx_DB_CharcGrp - 1] = 0;
                p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[idx_DB_CharcGrp - 1] = 0;
            }
            else
            {
                p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB_CharcGrp[idx_DB_CharcGrp - 1] = RspData_DataPair->Hdl;
            }
        }
        else
        {
            break;
        }
    }
    if (RspData_DataPair->Hdl_Value <= p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Ending)
    {
        p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = RspData_DataPair->Hdl_Value;
        for (i = 0; i < 3; i++)
        {
            if (p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i])
            {
                if (p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i])
                {
                    p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB = p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i];
                    p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB = p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB_CharcGrp[i];
                    p_host_current->state_DB_parse = HS_DBP_S3;
                    if (p_host_current->state_authe == HS_ATE_S0)
                    {
                        if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_FIND_INFORMATION_REQUEST, (uint8_t *)&p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB, 4) != ERR_OK)
                        {
                            HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
                        }
                    }
                    break;
                }
            }
        }
        if (i == 3)
        {
            if (p_host_current->state_authe == HS_ATE_S0)
            {
                if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_READ_BY_TYPE_REQUEST, (uint8_t *)&p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting, 6) != ERR_OK)
                {
                    HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
                }
            }
        }
    }
    else
    {
    }

    return ERR_OK;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Read_Request(void)
{
#if (_HOST_SERVER_ == 1)
    ACL_Data *ACL_data;
    ACL_Data *ACL_dataTx;
    ble_att_param_t *ATT_SERVER_content;
    uint16_t idx, end_DB_Mapping, DataAccu;
    uint8_t host_id, Rsp_Error;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        ACL_dataTx = (ACL_Data *)g_l2cap_dataTx;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);

        idx = ACL_data->PDU.ATTR_OP_Read_Request.Hdl;
        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        if (idx)
        {
            if (idx < att_db_mapping_size[host_id].size_map_server_db)
            {
                Rsp_Error = chkATT_PERMISSION_Read(host_id, idx);
            }
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_REQUEST, idx, Rsp_Error);
            break;
        }

        ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_server_db[idx]);
        Rsp_Error = ERR_CODE_ATT_NO_ERROR;
        if (ATT_SERVER_content->p_uuid_value != (void *)0)
        {
            DataAccu = ATT_SERVER_content->att_len;
            if (DataAccu > (LE_Host[host_id].att_MTU - 1))
            {
                DataAccu = (LE_Host[host_id].att_MTU - 1);
            }
            if (*((uint16_t *)ATT_SERVER_content->p_uuid_type) == GATT_DECL_CHARACTERISTIC)
            {
                end_DB_Mapping = idx + 1;

                ACL_dataTx->PDU.ATTR_OP_Read_Response.Attr_Data[0] = att_db_link[host_id].p_server_db[end_DB_Mapping]->property_value;
                memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_Response.Attr_Data[1], (uint8_t *)&end_DB_Mapping, 2);
                memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_Response.Attr_Data[3], (uint8_t *)ATT_SERVER_content->p_uuid_value, ATT_SERVER_content->att_len);
                DataAccu = ATT_SERVER_content->att_len + 3;
                err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_RESPONSE, ACL_dataTx->PDU.ATTR_OP_Read_Response.Attr_Data, DataAccu);
            }
            else
            {
                err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_RESPONSE, (uint8_t *)ATT_SERVER_content->p_uuid_value, DataAccu);
            }
        }
        if (err_code == ERR_OK)
        {
            if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t));
                while (p_queue_param == NULL)
                {
                    BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                    sys_task_delay(2);
                    Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t));
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_SERVER;
                pt_evt->cb_index = idx;
                pt_evt->handle_num = idx;
                pt_evt->opcode = OPCODE_ATT_READ_REQUEST;
                pt_evt->event = 0;
                pt_evt->length = 0;
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Read_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ble_att_handle_param_t *ATT_Hdl_DB_MAPPING_Para;
    uint16_t i, size_DB_Mapping, numHDL;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];
        size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;
        ATT_Hdl_DB_MAPPING_Para = att_db_mapping[host_id].map_client_db;

        if (p_host_current->numHdl_fc_cl)
        {
            numHDL = p_host_current->numHdl_fc_cl;
            for (i = 0; i < size_DB_Mapping; i++)
            {
                if (ATT_Hdl_DB_MAPPING_Para[i].handle_num == numHDL)
                {
                    break;
                }
            }
            if (i < size_DB_Mapping)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                if (p_host_current->att_MTU < ACL_data->Length_PDU)
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (p_host_current->att_MTU - 1));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (p_host_current->att_MTU - 1);
                }
                else
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 1));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (ACL_data->Length_PDU - 1);
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_CLIENT;
                pt_evt->cb_index = i;
                pt_evt->handle_num = numHDL;
                pt_evt->opcode = OPCODE_ATT_READ_RESPONSE;
                pt_evt->event = 0;
                memcpy(pt_evt->data, &ACL_data->PDU.ATTR_OP_Read_Response.Attr_Data, pt_evt->length);
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                p_host_current->numHdl_fc_cl = 0;
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Read_Blob_Request(void)
{
#if (_HOST_SERVER_ == 1)
    ACL_Data *ACL_data;
    ACL_Data *ACL_dataTx;
    ble_att_param_t *ATT_SERVER_content;
    uint8_t host_id, Rsp_Error;
    uint16_t end_DB_Mapping, offset, idx, DataAccu;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        ACL_dataTx = (ACL_Data *)g_l2cap_dataTx;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);

        idx = ACL_data->PDU.ATTR_OP_Read_Blob_Request.Hdl;
        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        if (idx)
        {
            if (idx < att_db_mapping_size[host_id].size_map_server_db)
            {
                Rsp_Error = chkATT_PERMISSION_Read(host_id, idx);
            }
        }
        if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
        {
            ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_server_db[idx]);
            DataAccu = ATT_SERVER_content->att_len;
            if (DataAccu)
            {
                offset = ACL_data->PDU.ATTR_OP_Read_Blob_Request.value_offset;

                if (offset > DataAccu)
                {
                    Rsp_Error = ERR_CODE_ATT_INVALID_OFFSET;
                }
                else
                {
                    if (DataAccu > (LE_Host[host_id].att_MTU - 1))
                    {
                        DataAccu -= offset;
                    }
                    else
                    {
                        if (offset != 0)
                        {
                            Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_LONG;
                        }
                    }
                    if (DataAccu > (LE_Host[host_id].att_MTU - 1))
                    {
                        DataAccu = (LE_Host[host_id].att_MTU - 1);
                    }
                }
            }
        }

        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BLOB_REQUEST, idx, Rsp_Error);
            break;
        }

        if (ATT_SERVER_content->p_uuid_value != (void *)0)
        {
            if (offset == 0)
            {
                if (*((uint16_t *)ATT_SERVER_content->p_uuid_type) == GATT_DECL_CHARACTERISTIC)
                {
                    end_DB_Mapping = idx + 1;

                    ACL_dataTx->PDU.ATTR_OP_Read_Blob_Response.Attr_Data_part[0] = att_db_link[host_id].p_server_db[end_DB_Mapping]->property_value;
                    memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_Blob_Response.Attr_Data_part[1], (uint8_t *)&end_DB_Mapping, 2);
                    memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_Blob_Response.Attr_Data_part[3], (uint8_t *)ATT_SERVER_content->p_uuid_value, ATT_SERVER_content->att_len);
                    DataAccu = ATT_SERVER_content->att_len + 3;
                    err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BLOB_RESPONSE, ACL_dataTx->PDU.ATTR_OP_Read_Blob_Response.Attr_Data_part, DataAccu);
                }
                else
                {
                    err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BLOB_RESPONSE, ((uint8_t *)ATT_SERVER_content->p_uuid_value) + offset, DataAccu);
                }
            }
            else
            {
                err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BLOB_RESPONSE, ((uint8_t *)ATT_SERVER_content->p_uuid_value) + offset, DataAccu);
            }
        }
        if (err_code == ERR_OK)
        {
            if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + 2);
                while (p_queue_param == NULL)
                {
                    BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                    sys_task_delay(2);
                    Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + 2);
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_SERVER;
                pt_evt->cb_index = idx;
                pt_evt->handle_num = idx;
                pt_evt->opcode = OPCODE_ATT_READ_BLOB_REQUEST;
                pt_evt->event = 0;
                pt_evt->length = 2;
                memcpy(pt_evt->data, (void *)&ACL_data->PDU.ATTR_OP_Read_Blob_Request.value_offset, 2);
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            }
        }
        else
        {
            err_code = ERR_MEM;
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Read_Blob_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ble_att_handle_param_t *ATT_Hdl_DB_MAPPING_Para;
    uint16_t i, size_DB_Mapping, numHDL;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];
        size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;
        ATT_Hdl_DB_MAPPING_Para = att_db_mapping[host_id].map_client_db;

        if (p_host_current->numHdl_fc_cl)
        {
            numHDL = p_host_current->numHdl_fc_cl;
            for (i = 0; i < size_DB_Mapping; i++)
            {
                if (ATT_Hdl_DB_MAPPING_Para[i].handle_num == numHDL)
                {
                    break;
                }
            }
            if (i < size_DB_Mapping)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t));

                if (p_host_current->att_MTU < ACL_data->Length_PDU)
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (p_host_current->att_MTU - 1));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (p_host_current->att_MTU - 1);
                }
                else
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 1));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (ACL_data->Length_PDU - 1);
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_CLIENT;
                pt_evt->cb_index = i;
                pt_evt->handle_num = numHDL;
                pt_evt->opcode = OPCODE_ATT_READ_BLOB_RESPONSE;
                pt_evt->event = 0;
                memcpy(pt_evt->data, (void *)&ACL_data->PDU.ATTR_OP_Read_Blob_Response.Attr_Data_part, pt_evt->length);
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                p_host_current->numHdl_fc_cl = 0;
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Read_Multiple_Request(void)
{
    int8_t err_code;

    err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_MULTIPLE_REQUEST, 0, ERR_CODE_ATT_REQUEST_NOT_SUPPORTED);

    return err_code;
}

int8_t Prcss_OPCODE_ATT_Read_Multiple_Response(void)
{
    return ERR_OK;
}

int8_t Prcss_OPCODE_ATT_Read_by_Group_Type_Request(void)
{
#if (_HOST_SERVER_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ACL_Data *ACL_dataTx;
    ble_att_param_t *ATT_SERVER_content;
    uint8_t host_id, Rsp_Error, attType, Len_AttType;
    uint16_t end_DB_Mapping, idx, DataAccu, uuid16;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        ACL_dataTx = (ACL_Data *)g_l2cap_dataTx;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];

        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        if (ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting <= ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending)
        {
            if (ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting != 0)
            {
                idx = ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting;
                if (att_db_mapping_size[host_id].size_map_server_db == 0)
                {
                    Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
                }
                else
                {
                    end_DB_Mapping = att_db_mapping_size[host_id].size_map_server_db - 1;
                    if (end_DB_Mapping > ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending)
                    {
                        end_DB_Mapping = ACL_data->PDU.ATTR_OP_General_Req.Hdl_Ending;
                    }
                    if (idx > end_DB_Mapping)
                    {
                        Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
                    }
                    else
                    {
                        Rsp_Error = ERR_CODE_ATT_NO_ERROR;
                    }
                }
            }
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BY_GROUP_TYPE_REQUEST, idx, Rsp_Error);
            break;
        }

        DataAccu = 0;
        attType = 0;
        if (ACL_data->Length_PDU == (2 + 5))
        {
            attType = ATT_TYPE_FORMAT_16UUID;
        }

        Rsp_Error = ERR_CODE_ATT_RESERVED;
        if (attType == 0)
        {
            if (memcmp((uint8_t *)ACL_data->PDU.ATTR_OP_Read_by_Group_Type_Request.typeUUID, (uint8_t *)UUID_BASE_BLUETOOTH, 6) == 0)
            {
                if (ACL_data->PDU.ATTR_OP_Read_by_Group_Type_Request.typeUUID[7] == 0)
                {
                    Rsp_Error = ERR_CODE_ATT_NO_ERROR;
                }
            }
            uuid16 = ACL_data->PDU.ATTR_OP_Read_by_Group_Type_Request.typeUUID[6];
        }
        else
        {
            uuid16 = ACL_data->PDU.ATTR_OP_Read_by_Group_Type_Request.typeUUID[0];
        }
        if ((uuid16 >= GATT_DECL_PRIMARY_SERVICE) && (uuid16 < GATT_DECL_CHARACTERISTIC))
        {
            Rsp_Error = ERR_CODE_ATT_NO_ERROR;
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BY_GROUP_TYPE_REQUEST, idx, ERR_CODE_ATT_UNSUPPORTED_GROUP_TYPE);
            break;
        }

        Rsp_Error = ERR_CODE_ATT_ATTRIBUTE_NOT_FOUND;
        while (1)
        {
            while (1)
            {
                ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_server_db[idx]);

                if ((ATT_SERVER_content->db_permission_format & ATT_TYPE_FORMAT_16UUID))
                {
                    if (uuid16 == *((uint16_t *)ATT_SERVER_content->p_uuid_type))
                    {
                        Rsp_Error = chkATT_PERMISSION_Read(host_id, idx);
                        if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
                        {
                            Len_AttType = ATT_SERVER_content->att_len;
                            memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_by_Group_Type_Response.Attr_Data_list[DataAccu], (uint8_t *)&idx, 2);
                            DataAccu += 2;
                            memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_by_Group_Type_Response.Attr_Data_list[DataAccu + 2], (uint8_t *)ATT_SERVER_content->p_uuid_value, Len_AttType);
                            break;
                        }
                    }
                }
                if (idx == end_DB_Mapping)
                {
                    break;
                }
                idx++;
            }
            if (DataAccu == 0)
            {
                break;
            }
            while (1)
            {
                if (idx == end_DB_Mapping)
                {
                    break;
                }
                if ((att_db_link[host_id].p_server_db[idx + 1]->db_permission_format) & ATT_TYPE_FORMAT_16UUID)
                {
                    if (((*((uint16_t *)att_db_link[host_id].p_server_db[idx + 1]->p_uuid_type)) >= GATT_DECL_PRIMARY_SERVICE) && ((*((uint16_t *)att_db_link[host_id].p_server_db[idx + 1]->p_uuid_type)) < GATT_DECL_CHARACTERISTIC))
                    {
                        break;
                    }
                }
                idx++;
            }
            memcpy((uint8_t *)&ACL_dataTx->PDU.ATTR_OP_Read_by_Group_Type_Response.Attr_Data_list[DataAccu], (uint8_t *)&idx, 2);
            DataAccu += (2 + Len_AttType);
            if ((DataAccu + Len_AttType + (2 + 2 + 2)) > p_host_current->att_MTU)
            {
                break;
            }
            if (idx == end_DB_Mapping)
            {
                break;
            }
            idx++;
            if (idx == end_DB_Mapping)
            {
                break;
            }
            if (Len_AttType != att_db_link[host_id].p_server_db[idx]->att_len)
            {
                break;
            }
        }
        if (DataAccu)
        {
            ACL_dataTx->PDU.ATTR_OP_Read_by_Group_Type_Response.length_list = Len_AttType + 4;
            err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BY_GROUP_TYPE_RESPONSE, &ACL_dataTx->PDU.ATTR_OP_Read_by_Group_Type_Response.length_list, (DataAccu + 1));
            break;
        }
        else
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_READ_BY_GROUP_TYPE_REQUEST, ACL_data->PDU.ATTR_OP_General_Req.Hdl_Starting, Rsp_Error);
            break;
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Read_by_Group_Type_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    LE_Host_Para *p_host_current;
    Pair_ATT_Hdl_Parsing_Param *RspData_Pair_Hdl_Param;
    ACL_Data *ACL_data;
    ble_att_param_t *ATT_SERVER_content;
    uint16_t i, size_DB_Mapping;
    uint8_t host_id, len_check, Idx_RspData_Pair;

    ACL_data = (ACL_Data *)g_l2cap_buffer.data;
    Idx_RspData_Pair = 0;

    host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];
    size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;

    while (1)
    {
        RspData_Pair_Hdl_Param = (Pair_ATT_Hdl_Parsing_Param *)&ACL_data->PDU.ATTR_OP_Read_by_Group_Type_Response.Attr_Data_list[Idx_RspData_Pair];
        for (i = 0; i < size_DB_Mapping; i++)
        {
            ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_client_db[i]);

            if (*((uint16_t *)ATT_SERVER_content->p_uuid_type) == GATT_DECL_PRIMARY_SERVICE)
            {
                len_check = ACL_data->PDU.ATTR_OP_Read_by_Group_Type_Response.length_list - 4;
                if (len_check != (ATT_SERVER_content->att_len))
                {
                    len_check = 0;
                }
                if (len_check)
                {
                    if (memcmp((uint8_t *)ATT_SERVER_content->p_uuid_value, (uint8_t *)&RspData_Pair_Hdl_Param->typeUUID, len_check) == 0)
                    {
                        att_db_mapping[host_id].map_client_db[i].handle_num = RspData_Pair_Hdl_Param->Hdl_Starting;
                        att_db_mapping[host_id].map_client_db[i].property_value = ATT_SERVER_content->property_value;

                        if (RspData_Pair_Hdl_Param->Hdl_Starting == RspData_Pair_Hdl_Param->Hdl_Ending)
                        {
                            i = size_DB_Mapping;
                        }
                        else
                        {
                            p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Starting_DB = i;
                            p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = RspData_Pair_Hdl_Param->Hdl_Starting;

                            p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = RspData_Pair_Hdl_Param->Hdl_Starting;
                            p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Ending = RspData_Pair_Hdl_Param->Hdl_Ending;

                            for (i = i + 1; i < size_DB_Mapping; i++)
                            {
                                if (*((uint16_t *)att_db_link[host_id].p_client_db[i]->p_uuid_type) == GATT_DECL_PRIMARY_SERVICE)
                                {
                                    break;
                                }
                            }
                            p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.idx_Ending_DB = i;
                            p_host_current->state_DB_parse = HS_DBP_S2;
                            if (p_host_current->state_authe == HS_ATE_S0)
                            {
                                if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_READ_BY_TYPE_REQUEST, (uint8_t *)&p_host_current->mblk_Param_L1->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting, 6) != ERR_OK)
                                {
                                    HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
                                }
                            }
                            i = 0;
                        }
                        break;
                    }
                }
            }
        }
        if (i != size_DB_Mapping)
        {
            break;
        }
        else
        {
            Idx_RspData_Pair += ACL_data->PDU.ATTR_OP_Read_by_Group_Type_Response.length_list;
        }

        if (ACL_data->Length_PDU > SIZE_DEFAULT_ATT_MTU)
        {
            ACL_data->Length_PDU = SIZE_DEFAULT_ATT_MTU;
        }
        if ((ACL_data->Length_PDU) > (Idx_RspData_Pair + 2))
        {
            p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = RspData_Pair_Hdl_Param->Hdl_Ending + 1;
        }
        else
        {
            i = size_DB_Mapping;
            break;
        }
    }

    do
    {
        if (i == size_DB_Mapping)
        {
            if (RspData_Pair_Hdl_Param->Hdl_Ending == 0xFFFF)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_param_t *p_evt_param;
                // DB parsing ok.
                p_host_current->state_DB_parse = HS_DBP_S0;

                // release resource.
                bhc_host_release_db_parsing_resource(host_id);

                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
                while (p_queue_param == NULL)
                {
                    BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                    sys_task_delay(2);
                    Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

                p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
                p_evt_param->event = BLE_ATT_GATT_EVT_DB_PARSE_COMPLETE;
                p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_db_parse_complete.host_id = host_id;
                p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_db_parse_complete.result = ERR_CODE_ATT_NO_ERROR;
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                bhc_timer_clear(host_id, TIMER_EVENT_CLIENT_DB_PARSING);
                break;
            }

            p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting = RspData_Pair_Hdl_Param->Hdl_Ending + 1;
            if (p_host_current->state_authe == HS_ATE_S0)
            {
                if (ATT_Request(p_host_current->LL_ConnID, OPCODE_ATT_READ_BY_GROUP_TYPE_REQUEST, (uint8_t *)&p_host_current->mblk_Param_L0->para.Pair_ATT_Hdl_Parsing_Param.Hdl_Starting, 6) != ERR_OK)
                {
                    HS_Msg_ATT_DB_PARSE_CARRY_ON(host_id);
                }
            }
        }
    } while (0);

    return ERR_OK;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Write_Request(void)
{
#if (_HOST_SERVER_ == 1)
    ACL_Data *ACL_data;
    ble_att_param_t *ATT_SERVER_content;
    LE_Host_Para *p_host_current;
    uint8_t host_id, Rsp_Error;
    uint16_t i, idx, end_DB_Mapping, PageIdx_old, PageIdx_new;
    uint8_t *tmp;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);

        idx = ACL_data->PDU.ATTR_OP_Write_Request.Hdl;
        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        if (idx)
        {
            if (idx < att_db_mapping_size[host_id].size_map_server_db)
            {
                end_DB_Mapping = att_db_mapping_size[host_id].size_map_server_db - 1;
                Rsp_Error = chkATT_PERMISSION_Write(host_id, idx);
            }
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_WRITE_REQUEST, idx, Rsp_Error);
            break;
        }

        ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_server_db[idx]);

        if (ACL_data->Length_PDU > LE_Host[host_id].att_MTU)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_WRITE_REQUEST, idx, ERR_CODE_ATT_INVALID_ATTRIBUTE_VALUE_LENGTH);
            break;
        }
        else if ((ATT_SERVER_content->att_len != 0) && ((ACL_data->Length_PDU - 3) > ATT_SERVER_content->att_len))
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_WRITE_REQUEST, idx, ERR_CODE_ATT_INVALID_ATTRIBUTE_VALUE_LENGTH);
            break;
        }
        else
        {
            err_code = ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_WRITE_RESPONSE, (uint8_t *)0, 0);
        }
        if (err_code == ERR_OK)
        {
            if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 3));
                while (p_queue_param == NULL)
                {
                    BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                    Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                    sys_task_delay(2);
                    Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 3));
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_SERVER;
                pt_evt->cb_index = idx;
                pt_evt->handle_num = idx;
                pt_evt->opcode = OPCODE_ATT_WRITE_REQUEST;
                pt_evt->event = 0;
                pt_evt->length = (ACL_data->Length_PDU - 3);
                memcpy(pt_evt->data, &ACL_data->PDU.ATTR_OP_Write_Request.Attr_Data, pt_evt->length);
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                p_host_current = &LE_Host[host_id];
                if ((p_host_current->BOND_Role == BLE_GAP_ROLE_CENTRAL) || (p_host_current->BOND_Role == BLE_GAP_ROLE_PERIPHERAL))
                {
                    if (p_host_current->SecurityMode >= SMP_SECURITY_MODE_MINIMUN)
                    {
                        if ((ATT_SERVER_content->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                        {
                            smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;
                            tmp = ble_flashbond_cmd(CMD_FB_GET_EXIST_PID_BY_HOST_ID, (uint8_t *)smp_Para_Bond_tmp);
                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                            {
                                *(tmp + TAB_PARA_DATA_DAT_HDL_H) = idx >> 8;
                                *(tmp + TAB_PARA_DATA_DAT_HDL_L) = idx;
                                *(tmp + TAB_PARA_DATA_GATT_ROLE) = BLE_GATT_ROLE_SERVER;
                                tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, (uint8_t *)smp_Para_Bond_tmp);
                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                {
                                    if (memcmp(&ACL_data->PDU.ATTR_OP_Write_Request.Attr_Data[0], (tmp + TAB_PARA_DATA_DAT), (ACL_data->Length_PDU - 3)) == 0)
                                    {
                                        break;
                                    }
                                }
                                tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                {
                                    *(tmp + TAB_PARA_DATA_DAT_SIZE) = (ACL_data->Length_PDU - 3);
                                    for (i = 0; i < (ACL_data->Length_PDU - 3); i++)
                                    {
                                        *(tmp + (TAB_PARA_DATA_DAT + i)) = ACL_data->PDU.ATTR_OP_Write_Request.Attr_Data[i];
                                    }
                                    tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                    if (*tmp == ERR_CODE_FLH_BND_NO_ENOUGH_REST_SPACE)
                                    {
                                        PageIdx_old = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                        tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_NXT_PID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                        {
                                            PageIdx_new = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                            tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                            {
                                                for (i = 1; i <= end_DB_Mapping; i++)
                                                {
                                                    if ((att_db_link[host_id].p_server_db[i]->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                                                    {
                                                        if (i != idx)
                                                        {
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_old >> 8;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_old;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_H] = i >> 8;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_L] = i;
                                                            tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, smp_Para_Bond_tmp);
                                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                            {
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                                tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                                {
                                                                    tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                tmp = ble_flashbond_cmd(CMD_FB_PSH_BACKUP_KEY_FLASH_PARA_BOND, (uint8_t *)smp_Para_Bond_tmp);
                                            }
                                        }
                                    }
                                }
                                else if (*tmp == ERR_CODE_FLH_BND_NO_ENOUGH_REST_SPACE)
                                {
                                    PageIdx_old = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                    tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_NXT_PID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                    {
                                        PageIdx_new = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                        *(tmp + TAB_PARA_DATA_DAT_SIZE) = (ACL_data->Length_PDU - 3);
                                        for (i = 0; i < (ACL_data->Length_PDU - 3); i++)
                                        {
                                            *(tmp + (TAB_PARA_DATA_DAT + i)) = ACL_data->PDU.ATTR_OP_Write_Request.Attr_Data[i];
                                        }
                                        tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                        {
                                            for (i = 1; i <= end_DB_Mapping; i++)
                                            {
                                                if ((att_db_link[host_id].p_server_db[i]->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                                                {
                                                    if (i != idx)
                                                    {
                                                        smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_old >> 8;
                                                        smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_old;
                                                        smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_H] = i >> 8;
                                                        smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_L] = i;
                                                        tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, smp_Para_Bond_tmp);
                                                        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                        {
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                            tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                            {
                                                                tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                            tmp = ble_flashbond_cmd(CMD_FB_PSH_BACKUP_KEY_FLASH_PARA_BOND, (uint8_t *)smp_Para_Bond_tmp);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Write_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    LE_Host_Para *p_host_current;
    ble_att_handle_param_t *ATT_Hdl_DB_MAPPING_Para;
    uint16_t i, idx, size_DB_Mapping, numHDL, PageIdx_old, PageIdx_new;
    uint8_t host_id;
    ble_att_param_t *ATT_SERVER_content;
    uint8_t *tmp;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];
        size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;
        ATT_Hdl_DB_MAPPING_Para = att_db_mapping[host_id].map_client_db;

        if (p_host_current->numHdl_fc_cl)
        {
            numHDL = p_host_current->numHdl_fc_cl;
            for (i = 0; i < size_DB_Mapping; i++)
            {
                if (ATT_Hdl_DB_MAPPING_Para[i].handle_num == numHDL)
                {
                    break;
                }
            }
            if (i < size_DB_Mapping)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t));
                if (p_queue_param == NULL)
                {
                    err_code = ERR_MEM;
                    break;
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_CLIENT;
                pt_evt->cb_index = i;
                pt_evt->handle_num = numHDL;
                pt_evt->opcode = OPCODE_ATT_WRITE_RESPONSE;
                pt_evt->event = 0;
                pt_evt->length = 0;
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
                p_host_current->numHdl_fc_cl = 0;

                // only support cccd setting.
                idx = i;
                ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_client_db[idx]);
                if ((p_host_current->BOND_Role == BLE_GAP_ROLE_CENTRAL) || (p_host_current->BOND_Role == BLE_GAP_ROLE_PERIPHERAL))
                {
                    if (p_host_current->SecurityMode >= SMP_SECURITY_MODE_MINIMUN)
                    {
                        if ((ATT_SERVER_content->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                        {
                            smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;
                            tmp = ble_flashbond_cmd(CMD_FB_GET_EXIST_PID_BY_HOST_ID, (uint8_t *)smp_Para_Bond_tmp);
                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                            {
                                *(tmp + TAB_PARA_DATA_DAT_HDL_H) = numHDL >> 8;
                                *(tmp + TAB_PARA_DATA_DAT_HDL_L) = numHDL;
                                *(tmp + TAB_PARA_DATA_GATT_ROLE) = BLE_GATT_ROLE_CLIENT;
                                tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, (uint8_t *)smp_Para_Bond_tmp);
                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                {
                                    if (memcmp(&ATT_Hdl_DB_MAPPING_Para[idx].value.cfg_client_charc, (tmp + TAB_PARA_DATA_DAT), 1) == 0)
                                    {
                                        break;
                                    }
                                }
                                tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                {
                                    *(tmp + TAB_PARA_DATA_DAT_SIZE) = 2;
                                    *(tmp + TAB_PARA_DATA_DAT) = ATT_Hdl_DB_MAPPING_Para[idx].value.cfg_client_charc;
                                    *(tmp + TAB_PARA_DATA_DAT + 1) = 0;
                                    tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                    if (*tmp == ERR_CODE_FLH_BND_NO_ENOUGH_REST_SPACE)
                                    {
                                        PageIdx_old = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                        tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_NXT_PID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                        {
                                            PageIdx_new = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                            tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                            {
                                                for (i = 1; i <= (size_DB_Mapping - 1); i++)
                                                {
                                                    if ((att_db_link[host_id].p_client_db[i]->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                                                    {
                                                        if (i != idx)
                                                        {
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_old >> 8;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_old;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_H] = ATT_Hdl_DB_MAPPING_Para[i].handle_num >> 8;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_L] = ATT_Hdl_DB_MAPPING_Para[i].handle_num;
                                                            tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, smp_Para_Bond_tmp);
                                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                            {
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                                tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                                {
                                                                    tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                tmp = ble_flashbond_cmd(CMD_FB_PSH_BACKUP_KEY_FLASH_PARA_BOND, (uint8_t *)smp_Para_Bond_tmp);
                                            }
                                        }
                                    }
                                }
                                else if (*tmp == ERR_CODE_FLH_BND_NO_ENOUGH_REST_SPACE)
                                {
                                    PageIdx_old = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                    tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_NXT_PID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                    {
                                        PageIdx_new = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                        *(tmp + TAB_PARA_DATA_DAT_SIZE) = 2;
                                        *(tmp + TAB_PARA_DATA_DAT) = ATT_Hdl_DB_MAPPING_Para[idx].value.cfg_client_charc;
                                        *(tmp + TAB_PARA_DATA_DAT + 1) = 0;
                                        tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                        {
                                            for (i = 1; i <= (size_DB_Mapping - 1); i++)
                                            {
                                                if ((att_db_link[host_id].p_client_db[i]->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                                                {
                                                    if (i != idx)
                                                    {
                                                        smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_old >> 8;
                                                        smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_old;
                                                        smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_H] = ATT_Hdl_DB_MAPPING_Para[i].handle_num >> 8;
                                                        smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_L] = ATT_Hdl_DB_MAPPING_Para[i].handle_num;
                                                        tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, smp_Para_Bond_tmp);
                                                        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                        {
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                            tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                            {
                                                                tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                            tmp = ble_flashbond_cmd(CMD_FB_PSH_BACKUP_KEY_FLASH_PARA_BOND, (uint8_t *)smp_Para_Bond_tmp);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Prepare_Write_Request(void)
{
#if (_HOST_SERVER_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    ble_att_param_t *ATT_SERVER_content;
    uint8_t host_id, Rsp_Error;
    uint16_t idx, length, value_offset;
    MBLK *mblk;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);

        idx = ACL_data->PDU.ATTR_OP_Prepare_Write_Request.Hdl;
        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        if (idx)
        {
            if (idx < att_db_mapping_size[host_id].size_map_server_db)
            {
                Rsp_Error = chkATT_PERMISSION_Write(host_id, idx);
            }
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_PREPARE_WRITE_REQUEST, idx, Rsp_Error);
            break;
        }

        ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_server_db[idx]);

        p_host_current = &LE_Host[host_id];
        value_offset = ACL_data->PDU.ATTR_OP_Prepare_Write_Request.value_offset;
        length = ACL_data->Length_PDU - 5; // 5: Opcode, Handle & Value Offset
        if (length > p_host_current->att_MTU_planed)
        {
            Rsp_Error = ERR_CODE_ATT_INVALID_ATTRIBUTE_VALUE_LENGTH;
        }
        else if ((ATT_SERVER_content->att_len != 0) && (length > ATT_SERVER_content->att_len))
        {
            Rsp_Error = ERR_CODE_ATT_INVALID_ATTRIBUTE_VALUE_LENGTH;
        }
        else if (value_offset >= p_host_current->att_MTU_planed)
        {
            Rsp_Error = ERR_CODE_ATT_INVALID_OFFSET;
        }
        else if ((value_offset + length) > p_host_current->att_MTU_planed)
        {
            Rsp_Error = ERR_CODE_ATT_PREPARE_QUEUE_FULL;
        }
        if (Rsp_Error != ERR_CODE_ATT_PREPARE_QUEUE_FULL)
        {
            if (p_host_current->mblk_Q_4_Wr == (MBLK *)0)
            {
                if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
                {
                    mblk = mem_malloc(sizeof(MHS_ATT_Queue_Data_Pkt_Para) + p_host_current->att_MTU_planed);
                    if (mblk == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                }
                else
                {
                    mblk = mem_malloc(sizeof(MHS_ATT_Queue_Data_Pkt_Para) + 1);
                    if (mblk == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                }
                p_host_current->mblk_Q_4_Wr = mblk;
                mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl = 0;
                mblk->para.MHS_ATT_Queue_Data_Pkt_Para.length = 0;
                mblk->para.MHS_ATT_Queue_Data_Pkt_Para.error_code = Rsp_Error;
            }
            mblk = p_host_current->mblk_Q_4_Wr;
            if (mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl)
            {
                if (mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl != ACL_data->PDU.ATTR_OP_Prepare_Write_Request.Hdl)
                {
                    Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
                }
            }
            else
            {
                if (p_host_current->mblk_Q_4_Wr->para.MHS_ATT_Queue_Data_Pkt_Para.error_code)
                {
                    Rsp_Error = p_host_current->mblk_Q_4_Wr->para.MHS_ATT_Queue_Data_Pkt_Para.error_code;
                }
                else
                {
                    mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl = ACL_data->PDU.ATTR_OP_Prepare_Write_Request.Hdl;
                }
            }
        }
        switch (Rsp_Error)
        {
        case ERR_CODE_ATT_INVALID_ATTRIBUTE_VALUE_LENGTH:
        case ERR_CODE_ATT_INVALID_OFFSET:
            if (mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl) // ERR_CODE_ATT_NO_ERROR:0
            {
                mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl = 0;
                mblk->para.MHS_ATT_Queue_Data_Pkt_Para.error_code = Rsp_Error;
            }
        case ERR_CODE_ATT_NO_ERROR:
            if (mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl) // ERR_CODE_ATT_NO_ERROR:0
            {
                if (value_offset == mblk->para.MHS_ATT_Queue_Data_Pkt_Para.length)
                {
                    memcpy(&mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_data[value_offset], &ACL_data->PDU.ATTR_OP_Prepare_Write_Request.Attr_Data_part[0], length);
                    mblk->para.MHS_ATT_Queue_Data_Pkt_Para.length = (value_offset + length);
                }
                else
                {
                    p_host_current->mblk_Q_4_Wr->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl = 0;
                    p_host_current->mblk_Q_4_Wr->para.MHS_ATT_Queue_Data_Pkt_Para.error_code = ERR_CODE_ATT_INVALID_OFFSET;
                }
            }
            if (ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_PREPARE_WRITE_RESPONSE, (uint8_t *)&ACL_data->PDU.ATTR_OP_Prepare_Write_Request.Hdl, (ACL_data->Length_PDU - 1)) == ERR_MEM) // SUCCESS_ or FAIL_
            {
                if (p_host_current->mblk_Q_4_Wr->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl)
                {
                    mblk->para.MHS_ATT_Queue_Data_Pkt_Para.length -= length;
                }
                err_code = ERR_MEM;
            }
            break;

        // case ERR_CODE_ATT_INVALID_HANDLE:
        // case ERR_CODE_ATT_PREPARE_QUEUE_FULL:
        default:
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_PREPARE_WRITE_REQUEST, idx, Rsp_Error);
            break;
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))}
}

int8_t Prcss_OPCODE_ATT_Prepare_Write_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    ble_att_handle_param_t *ATT_Hdl_DB_MAPPING_Para;
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    uint8_t host_id;
    uint16_t numHDL, i, size_DB_Mapping;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];
        size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;
        ATT_Hdl_DB_MAPPING_Para = att_db_mapping[host_id].map_client_db;

        numHDL = ACL_data->PDU.ATTR_OP_Prepare_Write_Response.Hdl;
        if (p_host_current->numHdl_fc_cl == numHDL)
        {
            for (i = 0; i < size_DB_Mapping; i++)
            {
                if (ATT_Hdl_DB_MAPPING_Para[i].handle_num == numHDL)
                {
                    break;
                }
            }
            if (i < size_DB_Mapping)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                if (p_host_current->att_MTU < ACL_data->Length_PDU)
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (p_host_current->att_MTU - 1));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (p_host_current->att_MTU - 1);
                }
                else
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 1));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (ACL_data->Length_PDU - 1);
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_CLIENT;
                pt_evt->cb_index = i;
                pt_evt->handle_num = numHDL;
                pt_evt->opcode = OPCODE_ATT_PREPARE_WRITE_RESPONSE;
                pt_evt->event = 0;
                memcpy(pt_evt->data, (void *)&ACL_data->PDU.ATTR_OP_Prepare_Write_Response.Hdl, pt_evt->length);
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                p_host_current->numHdl_fc_cl = 0;
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Execute_Write_Request(void)
{
#if (_HOST_SERVER_ == 1)
    LE_Host_Para *p_host_current;
    ACL_Data *ACL_data;
    uint8_t host_id, Rsp_Error;
    uint16_t idx, end_DB_Mapping;
    MBLK *mblk;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        end_DB_Mapping = att_db_mapping_size[host_id].size_map_server_db - 1;

        p_host_current = &LE_Host[host_id];

        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        if (ACL_data->PDU.ATTR_OP_Execute_Write_Request.flag == 0)
        {
            bhc_att_queue_write_resource(host_id);
        }

        mblk = p_host_current->mblk_Q_4_Wr;

        if (mblk == (MBLK *)0)
        {
            Rsp_Error = ERR_CODE_ATT_NO_ERROR;
        }
        else
        {
            idx = mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_Hdl;
            if (idx > end_DB_Mapping)
            {
                Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
            }
            else
            {
                if (idx)
                {
                    Rsp_Error = chkATT_PERMISSION_Write(host_id, idx);
                }
                else
                {
                    Rsp_Error = mblk->para.MHS_ATT_Queue_Data_Pkt_Para.error_code;
                }
            }
        }
        if (Rsp_Error != ERR_CODE_ATT_NO_ERROR)
        {
            err_code = ATT_Request_ERROR_RESPONSE(g_l2cap_buffer.conn_handle, OPCODE_ATT_EXECUTE_WRITE_REQUEST, idx, Rsp_Error);
            break;
        }
        else
        {
            if (ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_EXECUTE_WRITE_RESPONSE, (uint8_t *)0, 0) == ERR_OK)
            {
                if (mblk != (MBLK *)0)
                {
                    ble_host_to_app_evt_t *p_queue_param;
                    ble_evt_att_param_t *pt_evt;

                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + mblk->para.MHS_ATT_Queue_Data_Pkt_Para.length);
                    while (p_queue_param == NULL)
                    {
                        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                        sys_task_delay(2);
                        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + mblk->para.MHS_ATT_Queue_Data_Pkt_Para.length);
                    }
                    p_queue_param->u32_send_systick = sys_now();
                    p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->host_id = host_id;
                    pt_evt->gatt_role = BLE_GATT_ROLE_SERVER;
                    pt_evt->cb_index = idx;
                    pt_evt->handle_num = idx;
                    pt_evt->opcode = OPCODE_ATT_EXECUTE_WRITE_REQUEST;
                    pt_evt->event = 0;
                    pt_evt->length = mblk->para.MHS_ATT_Queue_Data_Pkt_Para.length;
                    memcpy(pt_evt->data, &mblk->para.MHS_ATT_Queue_Data_Pkt_Para.att_data, pt_evt->length);
                    task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
                    bhc_att_queue_write_resource(host_id);
                }
                break;
            }
            else
            {
                err_code = ERR_MEM;
                break;
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Execute_Write_Response(void)
{
#if (_HOST_CLIENT_ == 1)
    LE_Host_Para *p_host_current;
    ble_att_handle_param_t *ATT_Hdl_DB_MAPPING_Para;
    uint16_t i, numHDL, size_DB_Mapping;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
    p_host_current = &LE_Host[host_id];
    size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;
    ATT_Hdl_DB_MAPPING_Para = att_db_mapping[host_id].map_client_db;

    if (p_host_current->numHdl_fc_cl)
    {
        numHDL = p_host_current->numHdl_fc_cl;
        for (i = 0; i < size_DB_Mapping; i++)
        {
            if (ATT_Hdl_DB_MAPPING_Para[i].handle_num == numHDL)
            {
                break;
            }
        }
        if (i < size_DB_Mapping)
        {
            ble_host_to_app_evt_t *p_queue_param;
            ble_evt_att_param_t *pt_evt;

            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t));
            if (p_queue_param != NULL)
            {
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_CLIENT;
                pt_evt->cb_index = i;
                pt_evt->handle_num = numHDL;
                pt_evt->opcode = OPCODE_ATT_EXECUTE_WRITE_RESPONSE;
                pt_evt->event = 0;
                pt_evt->length = 0;
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
                p_host_current->numHdl_fc_cl = 0;
            }
            else
            {
                err_code = ERR_MEM;
            }
        }
    }

    return err_code;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Handle_Value_Notification(void)
{
#if (_HOST_CLIENT_ == 1)
    ACL_Data *ACL_data;
    ble_att_handle_param_t *ATT_Hdl_DB_MAPPING_Para;
    uint8_t host_id;
    uint16_t i, size_DB_Mapping;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;
        ATT_Hdl_DB_MAPPING_Para = att_db_mapping[host_id].map_client_db;

        for (i = 0; i < size_DB_Mapping; i++)
        {
            if (ATT_Hdl_DB_MAPPING_Para[i].handle_num == ACL_data->PDU.ATTR_OP_Handle_Value_Notification.Hdl)
            {
                break;
            }
        }
        if (i < size_DB_Mapping)
        {
            if ((ATT_Hdl_DB_MAPPING_Para[i].property_value & GATT_DECLARATIONS_PROPERTIES_NOTIFY) != 0)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                if (LE_Host[host_id].att_MTU < ACL_data->Length_PDU)
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (LE_Host[host_id].att_MTU - 3));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (LE_Host[host_id].att_MTU - 3);
                }
                else
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 3));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (ACL_data->Length_PDU - 3);
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_CLIENT;
                pt_evt->cb_index = i;
                pt_evt->handle_num = ATT_Hdl_DB_MAPPING_Para[i].handle_num;
                pt_evt->opcode = OPCODE_ATT_HANDLE_VALUE_NOTIFICATION;
                pt_evt->event = 0;
                memcpy(pt_evt->data, &ACL_data->PDU.ATTR_OP_Handle_Value_Notification.Attr_Data, pt_evt->length);
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Handle_Value_Indication(void)
{
#if (_HOST_CLIENT_ == 1)
    ACL_Data *ACL_data;
    ble_att_handle_param_t *ATT_Hdl_DB_MAPPING_Para;
    uint16_t i, size_DB_Mapping;
    uint8_t host_id;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;

        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        size_DB_Mapping = att_db_mapping_size[host_id].size_map_client_db;
        ATT_Hdl_DB_MAPPING_Para = att_db_mapping[host_id].map_client_db;

        for (i = 0; i < size_DB_Mapping; i++)
        {
            if (ATT_Hdl_DB_MAPPING_Para[i].handle_num == ACL_data->PDU.ATTR_OP_Handle_Value_Notification.Hdl)
            {
                break;
            }
        }
        if (i < size_DB_Mapping)
        {
            if ((ATT_Hdl_DB_MAPPING_Para[i].property_value & GATT_DECLARATIONS_PROPERTIES_INDICATE) != 0)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                if (LE_Host[host_id].att_MTU < ACL_data->Length_PDU)
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (LE_Host[host_id].att_MTU - 3));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (LE_Host[host_id].att_MTU - 3);
                }
                else
                {
                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 3));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->length = (ACL_data->Length_PDU - 3);
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_CLIENT;
                pt_evt->cb_index = i;
                pt_evt->handle_num = ATT_Hdl_DB_MAPPING_Para[i].handle_num;
                pt_evt->opcode = OPCODE_ATT_HANDLE_VALUE_INDICATION;
                pt_evt->event = 0;
                memcpy(pt_evt->data, &ACL_data->PDU.ATTR_OP_Handle_Value_Indication.Attr_Data, pt_evt->length);
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                ATT_Request(g_l2cap_buffer.conn_handle, OPCODE_ATT_HANDLE_VALUE_CONFIRMATION, (uint8_t *)0, 0);
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_CLIENT_ == 1))
}

int8_t Prcss_OPCODE_ATT_Handle_Value_Confirmation(void)
{
#if (_HOST_SERVER_ == 1)
    LE_Host_Para *p_host_current;
    uint8_t host_id;
    uint16_t size_DB_Mapping;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);
        p_host_current = &LE_Host[host_id];
        size_DB_Mapping = att_db_mapping_size[host_id].size_map_server_db;

        if (p_host_current->numHdl_fc_sr)
        {
            if (p_host_current->numHdl_fc_sr < size_DB_Mapping)
            {
                ble_host_to_app_evt_t *p_queue_param;
                ble_evt_att_param_t *pt_evt;

                p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t));
                if (p_queue_param == NULL)
                {
                    err_code = ERR_MEM;
                    break;
                }
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                pt_evt->host_id = host_id;
                pt_evt->gatt_role = BLE_GATT_ROLE_SERVER;
                pt_evt->cb_index = p_host_current->numHdl_fc_sr;
                pt_evt->handle_num = p_host_current->numHdl_fc_sr;
                pt_evt->opcode = OPCODE_ATT_HANDLE_VALUE_CONFIRMATION;
                pt_evt->event = 0;
                pt_evt->length = 0;
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            }
            p_host_current->numHdl_fc_sr = 0;
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Read_Multiple_Variable_Request(void)
{
    return ERR_OK;
}

int8_t Prcss_OPCODE_ATT_Read_Multiple_Variable_Response(void)
{
    return ERR_OK;
}

int8_t Prcss_OPCODE_ATT_Multiple_Handle_Value_Notification(void)
{
    return ERR_OK;
}

int8_t Prcss_OPCODE_ATT_Write_Command(void)
{
#if (_HOST_SERVER_ == 1)
    ACL_Data *ACL_data;
    ble_att_param_t *ATT_SERVER_content;
    LE_Host_Para *p_host_current;
    uint8_t host_id;
    uint16_t idx, end_DB_Mapping, i, PageIdx_old, PageIdx_new;
    uint8_t Rsp_Error;
    uint8_t *tmp;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        ACL_data = (ACL_Data *)g_l2cap_buffer.data;
        host_id = bhc_query_host_id_by_conn_id(g_l2cap_buffer.conn_handle);

        idx = ACL_data->PDU.ATTR_OP_Write_Command.Hdl;
        Rsp_Error = ERR_CODE_ATT_INVALID_HANDLE;
        if (idx)
        {
            if (idx < att_db_mapping_size[host_id].size_map_server_db)
            {
                end_DB_Mapping = att_db_mapping_size[host_id].size_map_server_db - 1;
                Rsp_Error = chkATT_PERMISSION_Write(host_id, idx);
            }
        }
        if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
        {
            ATT_SERVER_content = (ble_att_param_t *)(att_db_link[host_id].p_server_db[idx]);
            if (ACL_data->Length_PDU <= LE_Host[host_id].att_MTU)
            {
                if (ATT_SERVER_content->property_value & GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE)
                {
                    ble_host_to_app_evt_t *p_queue_param;
                    ble_evt_att_param_t *pt_evt;

                    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_att_param_t) + (ACL_data->Length_PDU - 3));
                    if (p_queue_param == NULL)
                    {
                        err_code = ERR_MEM;
                        break;
                    }
                    p_queue_param->u32_send_systick = sys_now();
                    p_queue_param->evt_type = BLE_APP_SERVICE_EVENT;

                    pt_evt = (ble_evt_att_param_t *)p_queue_param->parameter;
                    pt_evt->host_id = host_id;
                    pt_evt->gatt_role = BLE_GATT_ROLE_SERVER;
                    pt_evt->cb_index = idx;
                    pt_evt->handle_num = idx;
                    pt_evt->opcode = OPCODE_ATT_WRITE_COMMAND;
                    pt_evt->event = 0;
                    pt_evt->length = (ACL_data->Length_PDU - 3);
                    memcpy(pt_evt->data, &ACL_data->PDU.ATTR_OP_Write_Command.Attr_Data, pt_evt->length);
                    task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

                    p_host_current = &LE_Host[host_id];
                    if ((p_host_current->BOND_Role == BLE_GAP_ROLE_CENTRAL) || (p_host_current->BOND_Role == BLE_GAP_ROLE_PERIPHERAL))
                    {
                        if (p_host_current->SecurityMode >= SMP_SECURITY_MODE_MINIMUN)
                        {
                            if ((ATT_SERVER_content->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                            {
                                smp_Para_Bond_tmp[TAB_PARA_DATA_HOSTID] = host_id;
                                tmp = ble_flashbond_cmd(CMD_FB_GET_EXIST_PID_BY_HOST_ID, (uint8_t *)smp_Para_Bond_tmp);
                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                {
                                    *(tmp + TAB_PARA_DATA_DAT_HDL_H) = idx >> 8;
                                    *(tmp + TAB_PARA_DATA_DAT_HDL_L) = idx;
                                    *(tmp + TAB_PARA_DATA_GATT_ROLE) = BLE_GATT_ROLE_SERVER;
                                    tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, (uint8_t *)smp_Para_Bond_tmp);
                                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                    {
                                        if (memcmp(&ACL_data->PDU.ATTR_OP_Write_Request.Attr_Data[0], (tmp + TAB_PARA_DATA_DAT), (ACL_data->Length_PDU - 3)) == 0)
                                        {
                                            break;
                                        }
                                    }
                                    tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                    {
                                        *(tmp + TAB_PARA_DATA_DAT_SIZE) = (ACL_data->Length_PDU - 3);
                                        for (i = 0; i < (ACL_data->Length_PDU - 3); i++)
                                        {
                                            *(tmp + (TAB_PARA_DATA_DAT + i)) = ACL_data->PDU.ATTR_OP_Write_Request.Attr_Data[i];
                                        }
                                        tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                        if (*tmp == ERR_CODE_FLH_BND_NO_ENOUGH_REST_SPACE)
                                        {
                                            PageIdx_old = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                            tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_NXT_PID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                            {
                                                PageIdx_new = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                                tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                {
                                                    for (i = 1; i <= end_DB_Mapping; i++)
                                                    {
                                                        if ((att_db_link[host_id].p_server_db[i]->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                                                        {
                                                            if (i != idx)
                                                            {
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_old >> 8;
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_old;
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_H] = i >> 8;
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_L] = i;
                                                                tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, smp_Para_Bond_tmp);
                                                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                                {
                                                                    smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                                    smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                                    tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                                                    if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                                    {
                                                                        tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                    smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                    smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                    tmp = ble_flashbond_cmd(CMD_FB_PSH_BACKUP_KEY_FLASH_PARA_BOND, (uint8_t *)smp_Para_Bond_tmp);
                                                }
                                            }
                                        }
                                    }
                                    else if (*tmp == ERR_CODE_FLH_BND_NO_ENOUGH_REST_SPACE)
                                    {
                                        PageIdx_old = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                        tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_NXT_PID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                        if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                        {
                                            PageIdx_new = ((*(tmp + TAB_PARA_DATA_PID_H)) << 8) + (*(tmp + TAB_PARA_DATA_PID_L));
                                            *(tmp + TAB_PARA_DATA_DAT_SIZE) = (ACL_data->Length_PDU - 3);
                                            for (i = 0; i < (ACL_data->Length_PDU - 3); i++)
                                            {
                                                *(tmp + (TAB_PARA_DATA_DAT + i)) = ACL_data->PDU.ATTR_OP_Write_Request.Attr_Data[i];
                                            }
                                            tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                            {
                                                for (i = 1; i <= end_DB_Mapping; i++)
                                                {
                                                    if ((att_db_link[host_id].p_server_db[i]->db_permission_format & ATT_VALUE_BOND_ENABLE) == ATT_VALUE_BOND_ENABLE)
                                                    {
                                                        if (i != idx)
                                                        {
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_old >> 8;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_old;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_H] = i >> 8;
                                                            smp_Para_Bond_tmp[TAB_PARA_DATA_DAT_HDL_L] = i;
                                                            tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_VALUE_BY_HANDLE, smp_Para_Bond_tmp);
                                                            if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                            {
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                                tmp = ble_flashbond_cmd(CMD_FB_GET_DATA_FLASHBOND_EXIST_HOSTID_DBLK_START, (uint8_t *)smp_Para_Bond_tmp);
                                                                if (*tmp == FLH_BND_ERR_CODE_NO_ERR)
                                                                {
                                                                    tmp = ble_flashbond_cmd(CMD_FB_PSH_DATA_FLASHBOND_EXIST_HOSTID_DBLK, (uint8_t *)smp_Para_Bond_tmp);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_H] = PageIdx_new >> 8;
                                                smp_Para_Bond_tmp[TAB_PARA_DATA_PID_L] = PageIdx_new;
                                                tmp = ble_flashbond_cmd(CMD_FB_PSH_BACKUP_KEY_FLASH_PARA_BOND, (uint8_t *)smp_Para_Bond_tmp);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } while (0);

    return err_code;
#endif //(#if (_HOST_SERVER_ == 1))
}

int8_t Prcss_OPCODE_ATT_Signed_Write_Command(void)
{
    return ERR_OK;
}

uint8_t chkATT_PERMISSION_Read(uint8_t host_id, uint16_t ATT_Handle)
{
    uint8_t property, Rsp_Error;
    uint8_t permission;
    LE_Host_Para *p_host_current;

    property = att_db_link[host_id].p_server_db[ATT_Handle]->property_value;
    permission = att_db_link[host_id].p_server_db[ATT_Handle]->db_permission_format;
    p_host_current = &LE_Host[host_id];

    Rsp_Error = ERR_CODE_ATT_NO_ERROR;
    if ((property & GATT_DECLARATIONS_PROPERTIES_READ) == 0)
    {
        Rsp_Error = ERR_CODE_ATT_READ_NOT_PERMITTED;
    }
    else
    {
        if ((permission & ATT_PERMISSION_AUTHO_READ) != 0)
        {
            if (p_host_current->state_autho == HS_ATO_S0)
            {
                Rsp_Error = ERR_CODE_ATT_INSUFFICIENT_AUTHORIZATION;
            }
        }
        if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
        {
            if ((permission & ATT_PERMISSION_AUTHE_READ) != 0)
            {
                if (p_host_current->SecurityMode < SMP_SECURITY_MODE_MINIMUN)
                {
                    Rsp_Error = ERR_CODE_ATT_INSUFFICIENT_AUTHENTICATION;
                }
            }
        }
        if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
        {
            if ((permission & (ATT_PERMISSION_ENC_READ | ATT_PERMISSION_AUTHE_READ)) != 0)
            {
                if (p_host_current->IsEncryption == 0)
                {
                    Rsp_Error = ERR_CODE_ATT_INSUFFICIENT_ENCRYPTION;
                }
                else
                {
                    if (p_host_current->MaxEncKeySize < SIZE_MINIMUM_ENCRYPTION_KEY)
                    {
                        Rsp_Error = ERR_CODE_ATT_INSUFFICIENT_ENCRYPTION_KEY_SIZE;
                    }
                }
            }
        }
    }

    return Rsp_Error;
}

uint8_t chkATT_PERMISSION_Write(uint8_t host_id, uint16_t ATT_Handle)
{
    uint8_t property, Rsp_Error;
    uint8_t permission;
    LE_Host_Para *p_host_current;

    property = att_db_link[host_id].p_server_db[ATT_Handle]->property_value;
    permission = att_db_link[host_id].p_server_db[ATT_Handle]->db_permission_format;
    p_host_current = &LE_Host[host_id];

    Rsp_Error = ERR_CODE_ATT_NO_ERROR;
    if ((property & (GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE | GATT_DECLARATIONS_PROPERTIES_WRITE)) == 0)
    {
        Rsp_Error = ERR_CODE_ATT_WRITE_NOT_PERMITTED;
    }
    else
    {
        if ((permission & ATT_PERMISSION_AUTHO_WRITE) != 0)
        {
            if (p_host_current->state_autho == HS_ATO_S0)
            {
                Rsp_Error = ERR_CODE_ATT_INSUFFICIENT_AUTHORIZATION;
            }
        }
        if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
        {
            if ((permission & ATT_PERMISSION_AUTHE_WRITE) != 0)
            {
                if (p_host_current->SecurityMode < SMP_SECURITY_MODE_MINIMUN)
                {
                    Rsp_Error = ERR_CODE_ATT_INSUFFICIENT_AUTHENTICATION;
                }
            }
        }
        if (Rsp_Error == ERR_CODE_ATT_NO_ERROR)
        {
            if ((permission & (ATT_PERMISSION_ENC_WRITE | ATT_PERMISSION_AUTHE_WRITE)) != 0)
            {
                if (p_host_current->IsEncryption == 0)
                {
                    Rsp_Error = ERR_CODE_ATT_INSUFFICIENT_ENCRYPTION;
                }
                else
                {
                    if (p_host_current->MaxEncKeySize < SIZE_MINIMUM_ENCRYPTION_KEY)
                    {
                        Rsp_Error = ERR_CODE_ATT_INSUFFICIENT_ENCRYPTION_KEY_SIZE;
                    }
                }
            }
        }
    }
    return Rsp_Error;
}

int8_t setBLE_ConnTxData(uint16_t connID, uint8_t *ScanRspData, uint8_t Length)
{
    int8_t err_code;
    ble_hci_tx_acl_data_hdr_t *p_hci_msg;
    MBLK *mblk;

    err_code = ERR_OK;
    p_hci_msg = mem_malloc(sizeof(ble_hci_tx_acl_data_hdr_t));
    if (p_hci_msg != NULL)
    {
        p_hci_msg->transport_id = BLE_TRANSPORT_HCI_ACL_DATA;
        p_hci_msg->handle = connID;
        p_hci_msg->pb_flag = 0;
        p_hci_msg->bc_flag = 0;
        p_hci_msg->length = Length;

        mblk = get_msgblks_L1(p_hci_msg->length);
        if (mblk != NULL)
        {
            acl_data2msgblk(mblk, (uint8_t *)ScanRspData, p_hci_msg->length);
            p_hci_msg->p_data = mblk;
            task_host_queue_send(TASK_HOST_QUEUE_TX_ACL_DATA, p_hci_msg);
        }
        else
        {
            err_code = ERR_MEM;
            mem_free(p_hci_msg);
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

int8_t bhc_acl_data_send(uint16_t connID, uint8_t *ScanRspData, uint8_t Length)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_tx_acl_data_hdr_t *p_hci_msg;
    int8_t err_code;
    MBLK *mblk;

    err_code = ERR_MEM;
    vPortEnterCritical();
    do
    {
        if ((host_acl_data_queue_remaining_size() > 6) &&
            (sys_queue_remaining_size(&g_hci_common_handle) > 5))
        {
            p_hci_msg = mem_malloc(sizeof(ble_hci_tx_acl_data_hdr_t));
            if (p_hci_msg != NULL)
            {
                p_hci_msg->transport_id = BLE_TRANSPORT_HCI_ACL_DATA;
                p_hci_msg->handle = connID;
                p_hci_msg->pb_flag = 0;
                p_hci_msg->bc_flag = 0;
                p_hci_msg->length = Length;
                mblk = acl_data2msgblk_L2((uint8_t *)ScanRspData, Length);
                if (mblk != NULL)
                {
                    p_hci_msg->p_data = mblk;
                    hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_ACL_DATA;
                    hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_msg;
                    sys_queue_send(&g_hci_common_handle, (void *)&hci_comm_msg);
                    err_code = ERR_OK;
                }
                else
                {
                    mem_free(p_hci_msg);
                    break;
                }
            }
        }
    } while (0);
    vPortExitCritical();

    return err_code;
}

int8_t ATT_Request(uint16_t connID, uint8_t AttOpcode, uint8_t *paramGroup, uint16_t dataLength)
{
    ACL_Data *ACL_data;

    ACL_data = (ACL_Data *)g_l2cap_dataTx;
    ACL_data->Length_PDU = (dataLength + 1);
    ACL_data->ChannelID = L2CAP_CID_ATTRIBUTE_PROTOCOL;
    ACL_data->PDU.ATTR_OP_General.Attr_Opcode = AttOpcode;
    memcpy(ACL_data->PDU.ATTR_OP_General.Attr_Data, paramGroup, dataLength);

    return setBLE_ConnTxData(connID, g_l2cap_dataTx, (dataLength + (1 + SIZE_BASIC_L2CAP_HEADER)));
}

int8_t ATT_Request_ERROR_RESPONSE(uint16_t connID, uint8_t ReqOpcode_inError, uint16_t AttHandle_inError, uint8_t ErrorCode)
{
    ATTR_OP_Error_Response errData;

    errData.Req_Opcode = ReqOpcode_inError;
    errData.Attr_HandleInError = AttHandle_inError;
    errData.ErrorCode = ErrorCode;

    return ATT_Request(connID, OPCODE_ATT_ERROR_RESPONSE, (uint8_t *)&errData.Req_Opcode, 4);
}

static int8_t att_host_param_update(uint16_t conn_id, uint8_t att_opcode, uint16_t handle_num, uint8_t *p_data, uint16_t length)
{
    extern uint16_t queryArrayPos_By_HdlNum_Client(uint8_t host_id, uint16_t hdlNum);
    uint8_t host_id = bhc_query_host_id_by_conn_id(conn_id);

    switch (att_opcode)
    {
    case OPCODE_ATT_EXCHANGE_MTU_REQUEST:
        // update preferred mtu size
        LE_Host[host_id].att_MTU_planed = (((uint16_t)p_data[1] << 8) | p_data[0]);
        break;

    case OPCODE_ATT_READ_REQUEST:
    case OPCODE_ATT_READ_BLOB_REQUEST:
    case OPCODE_ATT_PREPARE_WRITE_REQUEST:
    case OPCODE_ATT_EXECUTE_WRITE_REQUEST:
        // update flow control data
        LE_Host[host_id].numHdl_fc_cl = handle_num;
        break;

    case OPCODE_ATT_HANDLE_VALUE_INDICATION:
        // update flow control data
        LE_Host[host_id].numHdl_fc_sr = handle_num;
        break;

    case OPCODE_ATT_WRITE_REQUEST:
    {
        ble_att_handle_param_t *p_att_handle_param;
        uint16_t idx;

        idx = queryArrayPos_By_HdlNum_Client(host_id, handle_num);
        p_att_handle_param = &att_db_mapping[host_id].map_client_db[idx];

        LE_Host[host_id].numHdl_fc_cl = handle_num;
        p_att_handle_param->value.cfg_client_charc = p_data[0];
    }
    break;

    case OPCODE_ATT_WRITE_COMMAND:
    {
        ble_att_handle_param_t *p_att_handle_param;
        uint16_t idx;

        idx = queryArrayPos_By_HdlNum_Client(host_id, handle_num);
        p_att_handle_param = &att_db_mapping[host_id].map_client_db[idx];

        p_att_handle_param->value.cfg_client_charc = p_data[0];
    }
    break;

    default:
        break;
    }

    return ERR_OK;
}

// BLE Host Command: send att request
int8_t bhc_att_req(uint16_t conn_id, uint8_t att_opcode, uint16_t handle_num, uint8_t *p_data, uint16_t length)
{
    int8_t err_code;

    err_code = ERR_OK;
    vPortEnterCritical();
    if ((host_acl_data_queue_remaining_size() > 6) &&
        (sys_queue_remaining_size(&g_hci_common_handle) > 5))
    {
        ACL_Data *ACL_data;
        ble_hci_tx_acl_data_hdr_t *p_hci_msg;
        uint16_t cmd_payload_len;
        hci_task_common_queue_t hci_comm_msg;
        MBLK *mblk;

        switch (att_opcode)
        {
        case OPCODE_ATT_ERROR_RESPONSE:
        case OPCODE_ATT_EXCHANGE_MTU_REQUEST:
        case OPCODE_ATT_READ_RESPONSE:
        case OPCODE_ATT_READ_BLOB_RESPONSE:
        case OPCODE_ATT_EXECUTE_WRITE_REQUEST:
            cmd_payload_len = length + 1 /*opcode*/ + SIZE_BASIC_L2CAP_HEADER;
            p_hci_msg = mem_malloc(sizeof(ble_hci_tx_acl_data_hdr_t));
            if (p_hci_msg != NULL)
            {
                hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_ACL_DATA;
                hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_msg;
                p_hci_msg->transport_id = BLE_TRANSPORT_HCI_ACL_DATA;
                p_hci_msg->handle = conn_id;
                p_hci_msg->pb_flag = 0;
                p_hci_msg->bc_flag = 0;
                p_hci_msg->length = cmd_payload_len;

                ACL_data = (ACL_Data *)acl_data_buff;
                ACL_data->Length_PDU = (length + 1);
                ACL_data->ChannelID = L2CAP_CID_ATTRIBUTE_PROTOCOL;
                ACL_data->PDU.ATTR_OP_General.Attr_Opcode = att_opcode;
                memcpy(ACL_data->PDU.ATTR_OP_General.Attr_Data, p_data, length);
                mblk = acl_data2msgblk_L2(acl_data_buff, cmd_payload_len);
                if (mblk != NULL)
                {
                    p_hci_msg->p_data = mblk;
                    sys_queue_send(&g_hci_common_handle, (void *)&hci_comm_msg);
                }
                else
                {
                    mem_free(p_hci_msg);
                    err_code = ERR_MEM;
                }
            }
            else
            {
                err_code = ERR_MEM;
            }
            break;

        case OPCODE_ATT_READ_BY_TYPE_RESPONSE:
        {
            typedef struct dataPrep_ReadByTypeRsp
            {
                uint8_t dummy;
                uint8_t length;
                uint16_t Hdl;
            } dataPrep_ReadByTypeRsp;

            uint16_t prepLength = 3;

            if (handle_num == 0)
            {
                err_code = ERR_MEM;
                break;
            }

            dataPrep_ReadByTypeRsp prepData;

            prepData.Hdl = handle_num;
            prepData.length = length + 2;

            cmd_payload_len = length + prepLength + (1 /*opcode*/ + SIZE_BASIC_L2CAP_HEADER);
            p_hci_msg = mem_malloc(sizeof(ble_hci_tx_acl_data_hdr_t));

            if (p_hci_msg != NULL)
            {
                hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_ACL_DATA;
                hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_msg;
                p_hci_msg->transport_id = BLE_TRANSPORT_HCI_ACL_DATA;
                p_hci_msg->handle = conn_id;
                p_hci_msg->pb_flag = 0;
                p_hci_msg->bc_flag = 0;
                p_hci_msg->length = cmd_payload_len;

                ACL_data = (ACL_Data *)acl_data_buff;
                ACL_data->Length_PDU = (length + prepLength + 1);
                ACL_data->ChannelID = L2CAP_CID_ATTRIBUTE_PROTOCOL;
                ACL_data->PDU.ATTR_OP_General.Attr_Opcode = att_opcode;
                memcpy(ACL_data->PDU.ATTR_OP_General.Attr_Data, &prepData.length, prepLength);
                memcpy(&ACL_data->PDU.ATTR_OP_General.Attr_Data[prepLength], p_data, length);
                mblk = acl_data2msgblk_L2(acl_data_buff, cmd_payload_len);
                if (mblk != NULL)
                {
                    p_hci_msg->p_data = mblk;
                    sys_queue_send(&g_hci_common_handle, (void *)&hci_comm_msg);
                }
                else
                {
                    mem_free(p_hci_msg);
                    err_code = ERR_MEM;
                }
            }
            else
            {
                err_code = ERR_MEM;
            }
        }
        break;

        default:
            if (handle_num == 0)
            {
                err_code = ERR_MEM;
                break;
            }

            cmd_payload_len = length + 3 /*opcode + handle_num*/ + SIZE_BASIC_L2CAP_HEADER;
            p_hci_msg = mem_malloc(sizeof(ble_hci_tx_acl_data_hdr_t));
            if (p_hci_msg != NULL)
            {
                hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_ACL_DATA;
                hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_msg;
                p_hci_msg->transport_id = BLE_TRANSPORT_HCI_ACL_DATA;
                p_hci_msg->handle = conn_id;
                p_hci_msg->pb_flag = 0;
                p_hci_msg->bc_flag = 0;
                p_hci_msg->length = cmd_payload_len;

                ACL_data = (ACL_Data *)acl_data_buff;
                ACL_data->Length_PDU = (length + 3);
                ACL_data->ChannelID = L2CAP_CID_ATTRIBUTE_PROTOCOL;
                ACL_data->PDU.ATTR_OP_General_Hdl.Attr_Opcode = att_opcode;
                ACL_data->PDU.ATTR_OP_General_Hdl.Hdl = handle_num;
                memcpy(ACL_data->PDU.ATTR_OP_General_Hdl.Attr_Data, p_data, length);
                mblk = acl_data2msgblk_L2(acl_data_buff, cmd_payload_len);
                if (mblk != NULL)
                {
                    p_hci_msg->p_data = mblk;
                    sys_queue_send(&g_hci_common_handle, (void *)&hci_comm_msg);
                }
                else
                {
                    mem_free(p_hci_msg);
                    err_code = ERR_MEM;
                }
            }
            else
            {
                err_code = ERR_MEM;
            }
            break;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }
    vPortExitCritical();

    if (err_code == ERR_OK)
    {
        // update host parameter
        att_host_param_update(conn_id, att_opcode, handle_num, p_data, length);
    }

    return err_code;
}

// BLE Host Command: send att error response
int8_t bhc_att_error_rsp_req(uint16_t conn_id, uint8_t att_opcode, uint16_t handle_num, uint8_t error_code)
{
    ATTR_OP_Error_Response errData;

    errData.Req_Opcode = att_opcode;
    errData.Attr_HandleInError = handle_num;
    errData.ErrorCode = error_code;

    return bhc_att_req(conn_id, OPCODE_ATT_ERROR_RESPONSE, 0, (uint8_t *)&errData.Req_Opcode, 4);
}

// BLE Host Command: Get GATT MTU Size
uint16_t bhc_gatt_att_mtu_get(uint8_t host_id)
{
    return LE_Host[host_id].att_MTU;
}

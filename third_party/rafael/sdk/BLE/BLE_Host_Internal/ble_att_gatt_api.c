/** @file ble_att_gatt.c
 *
 * @brief Provide the Definition of BLE Attributes and Generic Attributes Protocol.
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "lib_config.h"

#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
#include "ble_att_gatt_api.h"
#include "ble_event_module.h"
#include "ble_hci.h"
#include "ble_host_cmd.h"
#include "ble_memory.h"
#include "ble_printf.h"
#include "ble_profile.h"
#include "hci_cmd.h"
#include "sys_arch.h"

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

///** Set preferred MTU size and data length.
// */
// ble_err_t ble_gatt_mtu_dl_set(uint8_t host_id, uint16_t mtu, uint16_t tx_octets)
//{
//    ble_err_t status = BLE_ERR_OK;

//    // set preferred MTU size
//    status = ble_gatt_preferred_mtu_set(host_id, mtu);
//    if (status != BLE_ERR_OK)
//    {
//        return status;
//    }

// #if (!MODULE_ENABLE(HCI_SUPPORT_ZB)) //multi-protocol not support LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH now
//     // set preferred data length
//     status = ble_gatt_preferred_data_length_set(tx_octets);
//     if (status != BLE_ERR_OK)
//     {
//         return status;
//     }
// #endif
//     return BLE_ERR_OK;
// }

/** Set preferred data length.
 */
ble_err_t ble_gatt_suggested_data_length_set(ble_gatt_suggested_data_len_param_t * p_param)
{
    ble_hci_cmd_write_default_data_length_param_t p_hci_cmd_parm;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check default data length size
    if ((p_param->tx_octets > BLE_GATT_DATA_LENGTH_MAX) || (p_param->tx_octets < BLE_GATT_DATA_LENGTH_MIN))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // set HCI parameters
    p_hci_cmd_parm.tx_octets = p_param->tx_octets;
    p_hci_cmd_parm.tx_time   = ((p_param->tx_octets + 14u) << 3u);

    // issue HCI cmd
    if (hci_le_write_suggested_default_data_length_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        return BLE_ERR_SENDTO_FAIL;
    }

    return BLE_ERR_OK;
}

/** Set preferred MTU size.
 */
ble_err_t ble_gatt_preferred_mtu_set(ble_gatt_mtu_param_t * p_param)
{
    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check default mtu size
    if ((p_param->mtu > BLE_GATT_ATT_MTU_MAX) || (p_param->mtu < BLE_GATT_ATT_MTU_MIN))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    if (bhc_gatt_preferred_mtu_set(p_param->host_id, p_param->mtu) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    return BLE_ERR_OK;
}

/** ATT_MTU exchange request.
 */
ble_err_t ble_gatt_exchange_mtu_req(ble_gatt_mtu_param_t * p_param)
{
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check default mtu size
    if ((p_param->mtu > BLE_GATT_ATT_MTU_MAX) || (p_param->mtu < BLE_GATT_ATT_MTU_MIN))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check GATT role
    if (att_db_link[p_param->host_id].p_client_db == (const ble_att_param_t **) 0)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_EXCHANGE_MTU_REQUEST, 0, (uint8_t *) &p_param->mtu, 2) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** Set data length update.
 */
ble_err_t ble_gatt_data_length_update(ble_gatt_data_len_param_t * p_param)
{
    ble_hci_cmd_set_data_length_param_t p_hci_cmd_parm;
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check default data length size
    if ((p_param->tx_octets > BLE_GATT_DATA_LENGTH_MAX) || (p_param->tx_octets < BLE_GATT_DATA_LENGTH_MIN))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // set HCI parameters
    p_hci_cmd_parm.conn_handle = conn_id;
    p_hci_cmd_parm.tx_octets   = p_param->tx_octets;
    p_hci_cmd_parm.tx_time     = ((p_param->tx_octets + 14u) << 3u);

    // issue HCI cmd
    if (hci_le_set_data_length_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        return BLE_ERR_SENDTO_FAIL;
    }

    return BLE_ERR_OK;
}

/** Get BLE GATT MTU Size
 */
ble_err_t ble_gatt_mtu_get(ble_gatt_get_mtu_param_t * p_param)
{
    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check current BLE status first
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    *(p_param->p_mtu) = bhc_gatt_att_mtu_get(p_param->host_id);

    return BLE_ERR_OK;
}

/** Get BLE GATT Attribute Handles Mapping Table
 */
ble_err_t ble_gatt_att_handle_mapping_get(ble_gatt_handle_table_param_t * p_param)
{
    MBLK * p_mblk;
    ble_evt_param_t * p_evt_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // get handle number table
    if (bhc_gatt_att_handle_mapping_get(p_param->host_id, p_param->gatt_role, p_param->p_element, p_param->p_handle_num_addr) ==
        FALSE)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // send event to application
    p_mblk = get_msgblks_L1(sizeof(ble_evt_param_t));
    if (p_mblk != NULL)
    {
        p_mblk->primitive  = HOST_MSG_BY_PASS_GENERAL_EVENT;
        p_mblk->length     = sizeof(ble_evt_param_t);
        p_evt_param        = (ble_evt_param_t *) p_mblk->para.Data;
        p_evt_param->event = BLE_ATT_GATT_EVT_GET_ATT_HANDLES_TABLE_COMPLETE;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_handle_table_complete.host_id   = p_param->host_id;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_handle_table_complete.p_element = p_param->p_element;
        task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
    }
    else
    {
        return BLE_ERR_DATA_MALLOC_FAIL;
    }

    return BLE_ERR_OK;
}

/** BLE Read Response
 */
ble_err_t ble_gatt_read_rsp(ble_gatt_data_param_t * p_param)
{
    ble_err_t status;
    uint16_t conn_id, mtusize;
    ble_gatt_get_mtu_param_t mtu_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_server_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_server_property_value_is_match_check(p_param->host_id, p_param->handle_num, 0) == FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // check host state
    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // get current MTU size
    mtu_param.host_id = p_param->host_id;
    mtu_param.p_mtu   = &mtusize;
    status            = ble_gatt_mtu_get(&mtu_param);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // check data length -> mtuSize: 1 byte opcode other for attribute data
    // If the attribute value is longer than (ATT_MTU-1) then the first (ATT_MTU-1) octets shall be included in this response.
    p_param->length = (p_param->length > (mtusize - 1)) ? ((mtusize - 1)) : p_param->length;

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_READ_RESPONSE, 0, p_param->p_data, p_param->length) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE Read By Type Response
 */
ble_err_t ble_gatt_read_by_type_rsp(ble_gatt_data_param_t * p_param)
{
    ble_err_t status;
    uint16_t conn_id, mtusize;
    ble_gatt_get_mtu_param_t mtu_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_server_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_server_property_value_is_match_check(p_param->host_id, p_param->handle_num, 0) == FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // check host state
    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // get current MTU size
    mtu_param.host_id = p_param->host_id;
    mtu_param.p_mtu   = &mtusize;
    status            = ble_gatt_mtu_get(&mtu_param);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // check data length -> mtuSize: 1 byte opcode other for attribute data
    // If the attribute value is longer than (ATT_MTU-1) then the first (ATT_MTU-1) octets shall be included in this response.
    p_param->length = (p_param->length > (mtusize - 1)) ? ((mtusize - 1)) : p_param->length;

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_READ_BY_TYPE_RESPONSE, p_param->handle_num, p_param->p_data, p_param->length) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE Read Blob Response
 */
ble_err_t ble_gatt_read_blob_rsp(ble_gatt_data_param_t * p_param)
{
    ble_err_t status;
    uint16_t conn_id, mtusize;
    ble_gatt_get_mtu_param_t mtu_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_server_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_server_property_value_is_match_check(p_param->host_id, p_param->handle_num, 0) == FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // check host state
    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // get current MTU size
    mtu_param.host_id = p_param->host_id;
    mtu_param.p_mtu   = &mtusize;
    status            = ble_gatt_mtu_get(&mtu_param);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // check data length
    // mtuSize: 1 byte opcode other for attribute data
    if (p_param->length > (mtusize - 1))
    {
        // If the attribute value is longer than (ATT_MTU-1) then the first (ATT_MTU-1) octets shall be included in this response.
        p_param->length = (mtusize - 1);
    }

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_READ_BLOB_RESPONSE, 0, p_param->p_data, p_param->length) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE Error Response
 */
ble_err_t ble_gatt_error_rsp(ble_gatt_err_rsp_param_t * p_param)
{
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_server_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check host state
    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // send ATT error response
    if (bhc_att_error_rsp_req(conn_id, p_param->opcode, p_param->handle_num, p_param->err_rsp) != ERR_OK)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE Notification
 */
ble_err_t ble_gatt_notification(ble_gatt_data_param_t * p_param)
{
    ble_err_t status;
    uint16_t conn_id, mtusize;
    ble_gatt_get_mtu_param_t mtu_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_server_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_server_property_value_is_match_check(p_param->host_id, p_param->handle_num, GATT_DECLARATIONS_PROPERTIES_NOTIFY) ==
        FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // check host state
    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // get current MTU size
    mtu_param.host_id = p_param->host_id;
    mtu_param.p_mtu   = &mtusize;
    status            = ble_gatt_mtu_get(&mtu_param);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // Check data length < Current MTU size
    if (p_param->length > (mtusize - 3))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_HANDLE_VALUE_NOTIFICATION, p_param->handle_num, p_param->p_data, p_param->length) ==
        ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE Indication
 */
ble_err_t ble_gatt_indication(ble_gatt_data_param_t * p_param)
{
    ble_err_t status;
    uint16_t conn_id, mtusize;
    ble_gatt_get_mtu_param_t mtu_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_server_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_server_property_value_is_match_check(p_param->host_id, p_param->handle_num, GATT_DECLARATIONS_PROPERTIES_INDICATE) ==
        FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // check host state
    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // check GATT state
    if (bhc_host_is_wating_gatt_rsp_check(p_param->host_id) == TRUE)
    {
        return BLE_ERR_SEQUENTIAL_PROTOCOL_VIOLATION;
    }

    // get current MTU size
    mtu_param.host_id = p_param->host_id;
    mtu_param.p_mtu   = &mtusize;
    status            = ble_gatt_mtu_get(&mtu_param);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // Check data length < Current MTU size
    if (p_param->length > (mtusize - 3))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_HANDLE_VALUE_INDICATION, p_param->handle_num, p_param->p_data, p_param->length) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE Write Request
 */
ble_err_t ble_gatt_write_req(ble_gatt_data_param_t * p_param)
{
    ble_err_t status;
    uint16_t conn_id, mtusize;
    ble_gatt_get_mtu_param_t mtu_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_client_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_client_property_value_is_match_check(p_param->host_id, p_param->handle_num, GATT_DECLARATIONS_PROPERTIES_WRITE) ==
        FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // get current MTU size
    mtu_param.host_id = p_param->host_id;
    mtu_param.p_mtu   = &mtusize;
    status            = ble_gatt_mtu_get(&mtu_param);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // check host state
    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // check GATT state
    if (bhc_host_is_wating_gatt_rsp_check(p_param->host_id) == TRUE)
    {
        return BLE_ERR_SEQUENTIAL_PROTOCOL_VIOLATION;
    }

    // check data length < current MTU size
    if (p_param->length > (mtusize - 3))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_WRITE_REQUEST, p_param->handle_num, p_param->p_data, p_param->length) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE Write Command
 */
ble_err_t ble_gatt_write_cmd(ble_gatt_data_param_t * p_param)
{
    ble_err_t status;
    uint16_t conn_id, mtusize;
    ble_gatt_get_mtu_param_t mtu_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_client_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_client_property_value_is_match_check(p_param->host_id, p_param->handle_num,
                                                 GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE) == FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // check host state
    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // get current MTU size
    mtu_param.host_id = p_param->host_id;
    mtu_param.p_mtu   = &mtusize;
    status            = ble_gatt_mtu_get(&mtu_param);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // check data length < current MTU size
    if (p_param->length > (mtusize - 3))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_WRITE_COMMAND, p_param->handle_num, p_param->p_data, p_param->length) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE GATT Read Request
 */
ble_err_t ble_gatt_read_req(ble_gatt_read_req_param_t * p_param)
{
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_client_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_client_property_value_is_match_check(p_param->host_id, p_param->handle_num, GATT_DECLARATIONS_PROPERTIES_READ) == FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // check host state
    if (bhc_host_is_wating_gatt_rsp_check(p_param->host_id) == TRUE)
    {
        return BLE_ERR_SEQUENTIAL_PROTOCOL_VIOLATION;
    }

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_READ_REQUEST, p_param->handle_num, NULL, 0) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE GATT Read Long Characteristic Value
 */
ble_err_t ble_gatt_read_blob_req(ble_gatt_read_blob_req_param_t * p_param)
{
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check DB parsing is finished
    if (bhc_host_parsing_process_is_finished_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_DB_PARSING_IN_PROGRESS;
    }

    // check handle
    if ((p_param->handle_num == 0) || (p_param->handle_num >= att_db_mapping_size[p_param->host_id].size_map_client_db))
    {
        return BLE_ERR_INVALID_HANDLE;
    }

    // check GATT role
    if (bhc_client_property_value_is_match_check(p_param->host_id, p_param->handle_num, GATT_DECLARATIONS_PROPERTIES_READ) == FALSE)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    if (bhc_host_is_in_encryption_check(p_param->host_id) == TRUE)
    {
        return BLE_BUSY;
    }

    // check host state
    if (bhc_host_is_wating_gatt_rsp_check(p_param->host_id) == TRUE)
    {
        return BLE_ERR_SEQUENTIAL_PROTOCOL_VIOLATION;
    }

    // send ATT request
    if (bhc_att_req(conn_id, OPCODE_ATT_READ_BLOB_REQUEST, p_param->handle_num, (uint8_t *) &p_param->offset, 2) == ERR_MEM)
    {
        return BLE_BUSY;
    }

    return BLE_ERR_OK;
}

/** BLE ATT/GATT module event handler.
 *
 */
ble_err_t ble_evt_att_gatt_handler(void * p_param)
{
    ble_err_t status;
    ble_evt_param_t * p_evt_param = (ble_evt_param_t *) p_param;

    status = BLE_ERR_OK;
    switch (p_evt_param->event)
    {
    case BLE_ATT_GATT_EVT_DATA_LENGTH_SET: {
        ble_evt_data_length_set_t * p_cmd_param =
            (ble_evt_data_length_set_t *) &p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_data_length_set;

        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GATT] SET DATA LENGTH status = %d\n", p_cmd_param->status);
        }
    }
    break;

    case BLE_ATT_GATT_EVT_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH: {
        ble_evt_suggest_data_length_set_t * p_cmd_param =
            (ble_evt_suggest_data_length_set_t *) &p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_suggest_data_length_set;

        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GATT] WRITE DEFAULT DATA LENGTH status = %d\n", p_cmd_param->status);
        }
    }
    break;

    case BLE_ATT_GATT_EVT_DB_PARSE_COMPLETE: {
        ble_evt_att_db_parse_complete_t * p_cmd_param =
            (ble_evt_att_db_parse_complete_t *) &p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_db_parse_complete;

        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GATT] DB PARSE COMPLETED status = %d\n", p_cmd_param->result);
        }
    }
    break;

    case BLE_ATT_GATT_EVT_GET_ATT_HANDLES_TABLE_COMPLETE:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GATT] GET ATT HANDLES COMPLETED.\n");
        }
        break;

    case BLE_ATT_GATT_EVT_MTU_EXCHANGE:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GATT] MTU EXCHANGED\n");
        }
        break;

    case BLE_ATT_GATT_EVT_DATA_LENGTH_CHANGE:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GATT] DATA LENGTH EXCHANGED\n");
        }
        break;

    default:
        break;
    }

    return status;
}

#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))

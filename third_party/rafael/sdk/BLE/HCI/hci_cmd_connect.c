/** @file hci_cmd_connect.c
 *
 * @brief Includes Link Control commands (OGF = 0x01):
 *        - OCF = 0x0001 : Disconnect command
 *        - OCF = 0x001D : Read Remote Version Information command
 *
 *        Includes Status Parameter commands (OGF = 0x05):
 *        - OCF = 0x0005 : Read RSSI command
 *
 *        Includes LE Controller commands (OGF = 0x08):
 *        - OCF = 0x000D : LE Create Connection command
 *        - OCF = 0x000E : LE Create Connection Cancel command
 *        - OCF = 0x0013 : LE Connection Update command
 *        - OCF = 0x0014 : LE Set Host Channel Classification command
 *        - OCF = 0x0015 : LE Read Channel Map command
 *        - OCF = 0x0016 : LE Read Remote Features command
 *        - OCF = 0x0020 : LE Remote Connection Parameter Request Reply command
 *        - OCF = 0x0021 : LE Remote Connection Parameter Request Negative Replycommand
 *        - OCF = 0x0022 : LE Set Data Length command
 *        - OCF = 0x0023 : LE Read Suggested Default Data Length command
 *        - OCF = 0x0024 : LE Write Suggested Default Data Length command
 *        - OCF = 0x002F : LE Read Maximum Data Length command
 *        - OCF = 0x0030 : LE Read PHY command
 *        - OCF = 0x0031 : LE Set Default PHY command
 *        - OCF = 0x0032 : LE Set PHY command
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "lib_config.h"

#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
#include "ble_hci.h"
#include "sys_arch.h"
#include "task_hci.h"
#include "task_host.h"

/* ==============================
 *  Link Control: OGF = 0x01
 * ============================== */

/** Disconnect command
 *
 */
int8_t hci_disconn_cmd(ble_hci_cmd_disconnect_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_disconnect_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_disconnect_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LINK_CTRL_CMD;
        p_hci_message->ocf          = LINK_CTRL_CMD_DISCONNECT;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm              = (ble_hci_cmd_disconnect_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        p_hci_cmd_parm->reason      = p_param->reason;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** Read Remote Version Informations command
 *
 */
int8_t hci_read_remote_version_info_cmd(ble_hci_cmd_read_remote_ver_info_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * hci_message_ptr;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = 2;
    int8_t err_code;

    // set command
    err_code        = ERR_OK;
    malloc_len      = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    hci_message_ptr = mem_malloc(malloc_len);

    if (hci_message_ptr != NULL)
    {
        hci_message_ptr->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        hci_message_ptr->ogf          = HCI_CMD_LINK_CTRL_CMD;
        hci_message_ptr->ocf          = LINK_CTRL_CMD_READ_REMOTE_VERSION_INFO;
        hci_message_ptr->length       = cmd_payload_len;

        memcpy(hci_message_ptr->parameter, p_param, cmd_payload_len);
        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) hci_message_ptr;

        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(hci_message_ptr);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/* ==============================
 *  Status Parameter: OGF = 0x05
 * ============================== */

/** Read RSSI command
 *
 */
int8_t hci_read_rssi_cmd(ble_hci_cmd_read_rssi_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_read_rssi_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_read_rssi_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_STATUS_PARAM;
        p_hci_message->ocf          = STATUS_PARAM_READ_RSSI;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm              = (ble_hci_cmd_read_rssi_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/* ===================================
 *  LE Controller Command: OGF = 0x08
 * =================================== */

/** LE Create Connection command
 *
 */
int8_t hci_le_create_conn_cmd(ble_hci_cmd_create_conn_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_create_conn_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_create_conn_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_CREATE_CONNECTION;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm                     = (ble_hci_cmd_create_conn_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->scan_interval      = p_param->scan_interval;
        p_hci_cmd_parm->scan_window        = p_param->scan_window;
        p_hci_cmd_parm->init_filter_policy = p_param->init_filter_policy;
        p_hci_cmd_parm->peer_addr_type     = p_param->peer_addr_type;
        memcpy(p_hci_cmd_parm->peer_addr, p_param->peer_addr, BLE_ADDR_LEN);
        p_hci_cmd_parm->own_addr_type     = p_param->own_addr_type;
        p_hci_cmd_parm->conn_interval_min = p_param->conn_interval_min;
        p_hci_cmd_parm->conn_interval_max = p_param->conn_interval_max;
        p_hci_cmd_parm->max_latency       = p_param->max_latency;
        p_hci_cmd_parm->supv_timeout      = p_param->supv_timeout;
        p_hci_cmd_parm->min_celength      = p_param->min_celength;
        p_hci_cmd_parm->max_celength      = p_param->max_celength;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Create Connection Cancel command
 *
 */
int8_t hci_le_create_conn_cancel_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    uint16_t malloc_len;
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t);
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_CREATE_CONNECTION_CANCEL;
        p_hci_message->length       = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Connection Update command
 *
 */
int8_t hci_le_conn_update_cmd(ble_hci_cmd_conn_updated_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_conn_updated_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_conn_updated_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_CONNECTION_UPDATE;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm                    = (ble_hci_cmd_conn_updated_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle       = p_param->conn_handle;
        p_hci_cmd_parm->conn_interval_min = p_param->conn_interval_min;
        p_hci_cmd_parm->conn_interval_max = p_param->conn_interval_max;
        p_hci_cmd_parm->periph_latency    = p_param->periph_latency;
        p_hci_cmd_parm->supv_timeout      = p_param->supv_timeout;
        p_hci_cmd_parm->max_celength      = p_param->max_celength;
        p_hci_cmd_parm->min_celength      = p_param->min_celength;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Set Host Channel Classification command
 *
 */
int8_t hci_le_set_host_channel_classif_cmd(ble_hci_cmd_le_channel_classification_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_le_channel_classification_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_le_channel_classification_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_HOST_CHANNEL_CLASSIFICATION;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_le_channel_classification_t *) p_hci_message->parameter;
        memcpy(p_hci_cmd_parm, p_param, sizeof(ble_hci_cmd_le_channel_classification_t));

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Read Channel Map command
 *
 */
int8_t hci_le_read_channel_map_cmd(ble_hci_cmd_le_read_channel_map_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_le_read_channel_map_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_le_read_channel_map_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_READ_CHANNEL_MAP;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm              = (ble_hci_cmd_le_read_channel_map_t *) p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Read Remote Features command
 *
 */
int8_t hci_le_read_remote_features_cmd(ble_hci_cmd_read_remote_features_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = 2;
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_RANDOM_DEVICE_ADDRESS;
        p_hci_message->length       = cmd_payload_len;

        memcpy(p_hci_message->parameter, p_param, cmd_payload_len);

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Remote Connection Parameter Request Reply command
 *
 */
int8_t hci_le_remote_conn_param_req_reply_cmd(ble_hci_cmd_le_remote_conn_param_req_reply_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_le_remote_conn_param_req_reply_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_REMOTE_CONNECTION_PARAMETER_REQ_REPLY;
        p_hci_message->length       = cmd_payload_len;

        memcpy(p_hci_message->parameter, p_param, cmd_payload_len);

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Remote Connection Parameter Request Negative Replycommand
 *
 */
int8_t hci_le_remote_conn_param_req_neg_reply_cmd(ble_hci_cmd_le_remote_conn_param_req_neg_reply_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_le_remote_conn_param_req_neg_reply_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_REMOTE_CONNECTION_PARAMETER_REQ_NEG_REPLY;
        p_hci_message->length       = cmd_payload_len;

        memcpy(p_hci_message->parameter, p_param, cmd_payload_len);

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Set Data Length command
 *
 */
int8_t hci_le_set_data_length_cmd(ble_hci_cmd_set_data_length_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_set_data_length_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_data_length_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_DATA_LENGTH;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm              = (ble_hci_cmd_set_data_length_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        p_hci_cmd_parm->tx_octets   = p_param->tx_octets;
        p_hci_cmd_parm->tx_time     = p_param->tx_time;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Read Suggested Default Data Length command
 *
 */
int8_t hci_le_read_suggested_default_data_length_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    uint16_t malloc_len;
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t);
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_READ_SUGGESTED_DEFAULT_DATA_LENGTH;
        p_hci_message->length       = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Write Suggested Default Data Length command
 *
 */
int8_t hci_le_write_suggested_default_data_length_cmd(ble_hci_cmd_write_default_data_length_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_write_default_data_length_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_write_default_data_length_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm            = (ble_hci_cmd_write_default_data_length_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->tx_octets = p_param->tx_octets;
        p_hci_cmd_parm->tx_time   = p_param->tx_time;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Read Maximum Data Length command
 *
 */
int8_t hci_le_read_max_data_length_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    uint16_t malloc_len;
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t);
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_READ_MAXIMUM_DATA_LENGTH;
        p_hci_message->length       = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Read PHY command
 *
 */
int8_t hci_le_read_phy_cmd(ble_hci_cmd_read_phy_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_read_phy_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_read_phy_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_READ_PHY;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm              = (ble_hci_cmd_read_phy_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Set Default PHY command
 *
 */
int8_t hci_le_set_default_phy_cmd(ble_hci_cmd_set_default_phy_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_default_phy_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_READ_PHY;
        p_hci_message->length       = cmd_payload_len;
        memcpy(p_hci_message->parameter, p_param, cmd_payload_len);

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

/** LE Set PHY command
 *
 */
int8_t hci_le_set_phy_cmd(ble_hci_cmd_set_phy_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_set_phy_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_phy_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_PHY;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm              = (ble_hci_cmd_set_phy_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        p_hci_cmd_parm->all_phys    = p_param->all_phys;
        p_hci_cmd_parm->tx_phys     = p_param->tx_phys;
        p_hci_cmd_parm->rx_phys     = p_param->rx_phys;
        p_hci_cmd_parm->phy_options = p_param->phy_options;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) p_hci_message;
        if (host_send_cmd_to_hci(hci_comm_msg) == FALSE)
        {
            mem_free(p_hci_message);
            err_code = ERR_MEM;
        }
    }
    else
    {
        err_code = ERR_MEM;
    }

    return err_code;
}

#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))

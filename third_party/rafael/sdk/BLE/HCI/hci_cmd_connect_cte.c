/** @file hci_cmd_connect_cte.c
 *
 * @brief Includes LE Controller commands (OGF = 0x08):
 *        - OCF = 0x0054 : LE Set Connection CTE Receive Parameters command
 *        - OCF = 0x0055 : LE Set Connection CTE Transmit Parameters command
 *        - OCF = 0x0056 : LE Connection CTE Request Enable command
 *        - OCF = 0x0057 : LE Connection CTE Response Enable command
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "sys_arch.h"
#include "task_hci.h"
#include "task_host.h"
#include "ble_hci.h"
#include "mem_mgmt.h"

/* =======================================
 *  LE Controller Command: OGF = 0x08
 * ======================================= */

/** LE Set Connection CTE Receive Parameters command
 *
 */
int8_t hci_le_set_conn_cte_rx_param_cmd(ble_hci_cmd_set_conn_cte_rx_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_set_conn_cte_rx_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_conn_cte_rx_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len + p_param->sw_pattern_length;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_SET_CONNECTION_CTE_RECEIVE_PARAMETERS;
        p_hci_message->length = cmd_payload_len + p_param->sw_pattern_length;

        p_hci_cmd_parm = (ble_hci_cmd_set_conn_cte_rx_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        p_hci_cmd_parm->sampling_enable = p_param->sampling_enable;
        p_hci_cmd_parm->slot_durations = p_param->slot_durations;
        p_hci_cmd_parm->sw_pattern_length = p_param->sw_pattern_length;
        memcpy(p_hci_cmd_parm->antenna_ids, p_param->antenna_ids, p_param->sw_pattern_length);

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_message;
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


/** LE Set Connection CTE Transmit Parameters command
 *
 */
int8_t hci_le_set_conn_cte_tx_param_cmd(ble_hci_cmd_set_conn_cte_tx_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_set_conn_cte_tx_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_conn_cte_tx_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len + p_param->sw_pattern_length;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_SET_CONNECTION_CTE_TRANSMIT_PARAMETERS;
        p_hci_message->length = cmd_payload_len + p_param->sw_pattern_length;

        p_hci_cmd_parm = (ble_hci_cmd_set_conn_cte_tx_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        p_hci_cmd_parm->cte_types = p_param->cte_types;
        p_hci_cmd_parm->sw_pattern_length = p_param->sw_pattern_length;
        memcpy(p_hci_cmd_parm->antenna_ids, p_param->antenna_ids, p_param->sw_pattern_length);

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_message;
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


/** LE Connection CTE Request Enable command
 *
 */
int8_t hci_le_conn_cte_req_enable_cmd(ble_hci_cmd_set_conn_cte_req_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_set_conn_cte_req_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_conn_cte_req_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_CONNECTION_CTE_REQUEST_ENABLE;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_set_conn_cte_req_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        p_hci_cmd_parm->enable = p_param->enable;
        p_hci_cmd_parm->cte_req_interval = p_param->cte_req_interval;
        p_hci_cmd_parm->req_cte_length = p_param->req_cte_length;
        p_hci_cmd_parm->req_cte_type = p_param->req_cte_type;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_message;
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


/** LE Connection CTE Response Enable command
 *
 */
int8_t hci_le_conn_cte_rsp_enable_cmd(ble_hci_cmd_set_conn_cte_rsp_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_set_conn_cte_rsp_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_conn_cte_rsp_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_CONNECTION_CTE_RESPONSE_ENABLE;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_set_conn_cte_rsp_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        p_hci_cmd_parm->enable = p_param->enable;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)p_hci_message;
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




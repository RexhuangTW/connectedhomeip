/** @file hci_cmd_advertising.c
 *
 * @brief Includes LE Controller commands (OGF = 0x08):
 *        - OCF = 0x0006 : LE Set Advertising Parameters command
 *        - OCF = 0x0008 : LE Set Advertising Data command
 *        - OCF = 0x0009 : LE Set Scan Response Data command
 *        - OCF = 0x000A : LE Set Advertising Enable command
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "lib_config.h"

#if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
#include "ble_hci.h"
#include "sys_arch.h"
#include "task_hci.h"
#include "task_host.h"
#include <stdbool.h>
#include <stdint.h>

#define HCI_ADV_DATA_SIZE_MAX 31

/** LE Set Advertising Parameters command
 *
 */
int8_t hci_le_set_adv_param_cmd(ble_hci_cmd_set_adv_param_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_set_adv_param_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_adv_param_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_ADVERTISING_PARAMETERS;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm                   = (ble_hci_cmd_set_adv_param_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->adv_interval_min = p_param->adv_interval_min;
        p_hci_cmd_parm->adv_interval_max = p_param->adv_interval_max;
        p_hci_cmd_parm->adv_type         = p_param->adv_type;
        p_hci_cmd_parm->own_addr_type    = p_param->own_addr_type;
        p_hci_cmd_parm->peer_addr_type   = p_param->peer_addr_type;
        memcpy(p_hci_cmd_parm->peer_addr, p_param->peer_addr, BLE_ADDR_LEN);

        p_hci_cmd_parm->adv_channel_map   = p_param->adv_channel_map;
        p_hci_cmd_parm->adv_filter_policy = p_param->adv_filter_policy;

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

/** LE Set Advertising Data command
 *
 */
int8_t hci_le_set_adv_data_cmd(ble_hci_cmd_set_adv_data_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_set_adv_data_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = p_param->adv_data_length;
    int8_t err_code;

    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + HCI_ADV_DATA_SIZE_MAX + 1;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_ADVERTISING_DATA;
        p_hci_message->length       = (HCI_ADV_DATA_SIZE_MAX + 1);

        p_hci_cmd_parm                  = (ble_hci_cmd_set_adv_data_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->adv_data_length = cmd_payload_len;
        memset(p_hci_cmd_parm->adv_data, 0x00, HCI_ADV_DATA_SIZE_MAX);
        memcpy(p_hci_cmd_parm->adv_data, p_param->adv_data, cmd_payload_len);

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

/** LE Set Scan Response Data command
 *
 */
int8_t hci_le_set_scan_rsp_data_cmd(ble_hci_cmd_set_scan_rsp_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_set_scan_rsp_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = p_param->scan_rsp_data_length;
    int8_t err_code;

    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + HCI_ADV_DATA_SIZE_MAX + 1;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_SCAN_RSP_DATA;
        p_hci_message->length       = (HCI_ADV_DATA_SIZE_MAX + 1);

        p_hci_cmd_parm                       = (ble_hci_cmd_set_scan_rsp_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->scan_rsp_data_length = cmd_payload_len;
        memset(p_hci_cmd_parm->scan_rsp_data, 0x00, HCI_ADV_DATA_SIZE_MAX);
        memcpy(p_hci_cmd_parm->scan_rsp_data, p_param->scan_rsp_data, cmd_payload_len);

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

/** LE Set Advertising Enable command
 *
 */
int8_t hci_le_set_adv_enable_cmd(ble_hci_cmd_set_adv_enable_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_set_adv_enable_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_adv_enable_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_ADVERTISING_ENABLE;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm             = (ble_hci_cmd_set_adv_enable_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->adv_enable = p_param->adv_enable;

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

#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))

/** @file hci_cmd_scan.c
 *
 * @brief Includes LE Controller commands (OGF = 0x08):
 *        - OCF = 0x000B : LE Set Scan Parameters command
 *        - OCF = 0x000C : LE Set Scan Enable command
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "lib_config.h"

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
#include "ble_hci.h"
#include "sys_arch.h"
#include "task_hci.h"
#include "task_host.h"
#include <stdbool.h>
#include <stdint.h>

/** LE Set Scan Parameters command
 *
 */
int8_t hci_le_set_scan_param_cmd(ble_hci_cmd_set_scan_param_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_set_scan_param_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_scan_param_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_SCAN_PARAMETERS;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm                     = (ble_hci_cmd_set_scan_param_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->scan_type          = p_param->scan_type;
        p_hci_cmd_parm->scan_interval      = p_param->scan_interval;
        p_hci_cmd_parm->scan_window        = p_param->scan_window;
        p_hci_cmd_parm->own_addr_type      = p_param->own_addr_type;
        p_hci_cmd_parm->scan_filter_policy = p_param->scan_filter_policy;

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

/** LE Set Scan Enable command
 *
 */
int8_t hci_le_set_scan_enable_cmd(ble_hci_cmd_set_scan_enable_param_t * p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t * p_hci_message;
    ble_hci_cmd_set_scan_enable_param_t * p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_scan_enable_param_t);
    int8_t err_code;

    // set command
    err_code      = ERR_OK;
    malloc_len    = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf          = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf          = LE_SET_SCAN_ENABLE;
        p_hci_message->length       = cmd_payload_len;

        p_hci_cmd_parm                         = (ble_hci_cmd_set_scan_enable_param_t *) p_hci_message->parameter;
        p_hci_cmd_parm->scan_enable            = p_param->scan_enable;
        p_hci_cmd_parm->scan_filter_duplicates = p_param->scan_filter_duplicates;

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

#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))

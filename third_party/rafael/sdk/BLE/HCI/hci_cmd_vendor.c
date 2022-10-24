/** @file hci_cmd_vendor.c
 *
 * @brief Includes Vendor commands (OGF = 0x3F):
 *        - OCF = 0x0001 : Set Controller Information vendor command
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "sys_arch.h"
#include "mem_mgmt.h"
#include "task_hci.h"
#include "task_host.h"
#include "ble_hci.h"


/* ==================================================
 *  Vendor Commands: OGF = 0x3F
 * ================================================== */

/** Set Controller Information vendor command
 *
 */
int8_t hci_vendor_set_controller_info_cmd(ble_hci_vcmd_cntlr_info_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_vcmd_cntlr_info_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    int8_t err_code;
    uint16_t cmd_payload_len = sizeof(ble_hci_vcmd_cntlr_info_param_t);

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_VENDOR_CMD;
        p_hci_message->ocf = LE_SET_CONTROLLER_INFO;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_vcmd_cntlr_info_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->ble_version = p_param->ble_version;
        p_hci_cmd_parm->ble_company_id = p_param->ble_company_id;
        memcpy(p_hci_cmd_parm->ble_public_addr, p_param->ble_public_addr, BLE_ADDR_LEN);

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

/** Set BLE scan request report command
 *
 */
int8_t hci_vendor_set_scan_request_report_cmd(ble_hci_vcmd_scan_req_rpt_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_vcmd_scan_req_rpt_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    int8_t err_code;
    uint16_t cmd_payload_len = sizeof(ble_hci_vcmd_scan_req_rpt_param_t);

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_VENDOR_CMD;
        p_hci_message->ocf = LE_SCAN_REQUEST_REPORT;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_vcmd_scan_req_rpt_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->scan_report_status = p_param->scan_report_status;

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

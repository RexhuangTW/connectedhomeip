/** @file hci_cmd_security.c
 *
 * @brief Includes LE Controller commands (OGF = 0x08):
 *        - OCF = 0x0017 : LE Encrypt command
 *        - OCF = 0x0018 : LE Rand command
 *        - OCF = 0x0019 : LE Enable Encryption command
 *        - OCF = 0x001A : LE Long Term Key Request Reply command
 *        - OCF = 0x001B : LE Long Term Key Request Negative Reply command
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


/* =======================================
 *  LE Controller Command: OGF = 0x08
 * ======================================= */

/** LE Encrypt command
 *
 */
int8_t hci_le_encrypt_cmd(ble_hci_le_encrypt_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_le_encrypt_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_le_encrypt_param_t);
    int8_t err_code;

    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ocf = LE_ENCRYPT;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->length = cmd_payload_len;
        p_hci_cmd_parm = (ble_hci_le_encrypt_param_t *)p_hci_message->parameter;
        memcpy(p_hci_cmd_parm->key, p_param->key, sizeof(p_param->key));
        memcpy(p_hci_cmd_parm->plaintext_data, p_param->plaintext_data, sizeof(p_param->plaintext_data));

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


/** LE Rand command
 *
 */
int8_t hci_le_random_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    uint16_t malloc_len;
    int8_t err_code;

    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ocf = LE_RANDOM;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->length = 0;

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


/** LE Enable Encryption command
 *
 */
int8_t hci_le_enable_encryption_cmd(ble_hci_le_enable_encrypt_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_le_enable_encrypt_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_le_enable_encrypt_param_t);
    int8_t err_code;

    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ocf = LE_ENABLE_ENCRYPT;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->length = cmd_payload_len;
        p_hci_cmd_parm = (ble_hci_le_enable_encrypt_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        memcpy(p_hci_cmd_parm->random_number, p_param->random_number, sizeof(p_param->random_number));
        memcpy(p_hci_cmd_parm->encrypted_diversifier, p_param->encrypted_diversifier, sizeof(p_param->encrypted_diversifier));
        memcpy(p_hci_cmd_parm->long_term_key, p_param->long_term_key, sizeof(p_param->long_term_key));

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


/** LE Long Term Key Request Reply command
 *
 */
int8_t hci_le_long_term_key_req_reply_cmd(ble_hci_le_long_term_key_req_reply_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_le_long_term_key_req_reply_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_le_long_term_key_req_reply_param_t);
    int8_t err_code;

    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ocf = LE_LONG_TERM_KEY_REQ_REPLY;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->length = cmd_payload_len;
        p_hci_cmd_parm = (ble_hci_le_long_term_key_req_reply_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;
        memcpy(p_hci_cmd_parm->long_term_key, p_param->long_term_key, sizeof(p_param->long_term_key));

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


/** LE Long Term Key Request Negative Reply command
 *
 */
int8_t hci_le_long_term_key_req_neg_reply_cmd(ble_hci_le_long_term_key_neg_reply_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_le_long_term_key_neg_reply_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_le_long_term_key_neg_reply_param_t);
    int8_t err_code;

    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ocf = LE_LONG_TERM_KEY_REQ_NEG_REPLY;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->length = cmd_payload_len;
        p_hci_cmd_parm = (ble_hci_le_long_term_key_neg_reply_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->conn_handle = p_param->conn_handle;

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



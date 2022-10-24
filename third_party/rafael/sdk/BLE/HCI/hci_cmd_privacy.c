/** @file hci_cmd_privacy.c
 *
 * @brief Includes LE Controller commands (OGF = 0x08):
 *        - OCF = 0x0027 : LE Add Device To Resolving List command
 *        - OCF = 0x0028 : LE Remove Device From Resolving List command
 *        - OCF = 0x0029 : LE Clear Resolving List command
 *        - OCF = 0x002A : LE Read Resolving List Size command
 *        - OCF = 0x002B : LE Read Peer Resolvable Address command
 *        - OCF = 0x002C : LE Read Local Resolvable Address command
 *        - OCF = 0x002D : LE Set Address Resolution Enable command
 *        - OCF = 0x004E : LE Set Privacy Mode command
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "sys_arch.h"
#include "task_hci.h"
#include "task_host.h"
#include "ble_hci.h"
#include "mem_mgmt.h"
#include "smp.h"

/* =======================================
 *  LE Controller Command: OGF = 0x08
 * ======================================= */

/** LE Add Device To Resolving List command
 *
 */
int8_t hci_le_add_device_to_resolving_list_cmd(ble_hci_cmd_add_device_to_resolving_list_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_add_device_to_resolving_list_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_add_device_to_resolving_list_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_ADD_DEVICE_TO_RESOLVING_LIST;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_add_device_to_resolving_list_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->peer_id_addr_type = p_param->peer_id_addr_type;
        memcpy(p_hci_cmd_parm->peer_id_addr, p_param->peer_id_addr, BLE_ADDR_LEN);
        memcpy(p_hci_cmd_parm->peer_irk, p_param->peer_irk, SIZE_SMP_IRK);
        memcpy(p_hci_cmd_parm->local_irk, p_param->local_irk, SIZE_SMP_IRK);

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


/** LE Remove Device From Resolving List command
 *
 */
int8_t hci_le_remove_device_from_resolving_list_cmd(ble_hci_cmd_remove_device_from_resolving_list_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_remove_device_from_resolving_list_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_remove_device_from_resolving_list_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_REMOVE_DEVICE_FROM_RESOLVING_LIST;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_remove_device_from_resolving_list_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->peer_id_addr_type = p_param->peer_id_addr_type;
        memcpy(p_hci_cmd_parm->peer_id_addr, p_param->peer_id_addr, BLE_ADDR_LEN);

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


/** LE Clear Resolving List command
 *
 */
int8_t hci_le_clear_resolving_list_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    int8_t err_code;
    uint16_t malloc_len;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_CLEAR_RESOLVING_LIST;
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


/** LE Read Resolving List Size command
 *
 */
int8_t hci_le_read_resolving_list_size_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    int8_t err_code;
    uint16_t malloc_len;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_READ_RESOLVING_LIST_SIZE;
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


/** LE Read Peer Resolvable Address command
 *
 */
int8_t hci_le_read_peer_resolvable_addr_cmd(ble_hci_cmd_read_peer_resolvable_addr_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_read_peer_resolvable_addr_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_read_peer_resolvable_addr_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_READ_PEER_RESOLVABLE_ADDRESS;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_read_peer_resolvable_addr_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->peer_id_addr_type = p_param->peer_id_addr_type;
        memcpy(p_hci_cmd_parm->peer_id_addr, p_param->peer_id_addr, BLE_ADDR_LEN);

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


/** LE Read Local Resolvable Address command
 *
 */
int8_t hci_le_read_local_resolvable_addr_cmd(ble_hci_cmd_read_local_resolvable_addr_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_read_local_resolvable_addr_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_read_local_resolvable_addr_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_READ_LOCAL_RESOLVABLE_ADDRESS;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_read_local_resolvable_addr_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->peer_id_addr_type = p_param->peer_id_addr_type;
        memcpy(p_hci_cmd_parm->peer_id_addr, p_param->peer_id_addr, BLE_ADDR_LEN);

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


/** LE Set Address Resolution Enable command
 *
 */
int8_t hci_le_set_addr_resolution_enable_cmd(ble_hci_cmd_set_addr_resolution_enable_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_set_addr_resolution_enable_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_addr_resolution_enable_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_SET_ADDRESS_RESOLUTION_ENABLE;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_set_addr_resolution_enable_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->addr_resolution_enable = p_param->addr_resolution_enable;

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


/** LE Set Privacy Mode command
 *
 */
int8_t hci_le_set_privacy_mode_cmd(ble_hci_cmd_set_privacy_mode_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_set_privacy_mode_param_t *p_hci_cmd_parm;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_privacy_mode_param_t);
    int8_t err_code;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_SET_PRIVACY_MODE;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_set_privacy_mode_param_t *)p_hci_message->parameter;
        p_hci_cmd_parm->peer_id_addr_type = p_param->peer_id_addr_type;
        memcpy(p_hci_cmd_parm->peer_id_addr, p_param->peer_id_addr, BLE_ADDR_LEN);
        p_hci_cmd_parm->privacy_mode = p_param->privacy_mode;

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

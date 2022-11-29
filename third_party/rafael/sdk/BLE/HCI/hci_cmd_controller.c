/** @file hci_cmd_controller.c
 *
 * @brief Includes Controller & Baseband commands (OGF = 0x03):
 *        - OCF = 0x0001 : Set Event Mask command
 *        - OCF = 0x0003 : Reset command
 *
 *        Includes Information Parameters commands (OGF = 0x04):
 *        - OCF = 0x0001 : Read Local Version Information command
 *        - OCF = 0x0002 : Read Local Supported Commands command
 *        - OCF = 0x0003 : Read Local Supported Features command
 *        - OCF = 0x0005 : Read Buffer Size command
 *        - OCF = 0x0009 : Read BD_ADDR command
 *
 *        Includes LE Controller commands (OGF = 0x08):
 *        - OCF = 0x0001 : LE Set Event Mask command
 *        - OCF = 0x0002 : LE Read Buffer Size command
 *        - OCF = 0x0003 : LE Read Local Supported Features command
 *        - OCF = 0x0005 : LE Set Random Address command
 *        - OCF = 0x000F : LE Read Filter Accept List Size command
 *        - OCF = 0x0010 : LE Clear Filter Accept List command
 *        - OCF = 0x0011 : LE Add Device To Filter Accept List command
 *        - OCF = 0x0012 : LE Remove Device From Filter Accept List command
 *        - OCF = 0x001C : LE Read Supported States command
 *        - OCF = 0x0058 : LE Read Antenna Information command
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
 *  Controller and Baseband Commands: OGF = 0x03
 * ================================================== */

/** Set Event Mask command
 *
 */
int8_t hci_set_event_mask_cmd(void)
{
    // TODO
    return ERR_OK;
}


/** Reset command
 *
 */
int8_t hci_reset_cmd(void)
{
    // TODO
    return ERR_OK;
}


/* =======================================
 *  Information Parameters: OGF = 0x04
 * ======================================= */

/** Read Local Version Information command
 *
 */
int8_t hci_read_local_ver_info_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *hci_message_ptr;
    int8_t err_code;
    uint16_t malloc_len;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    hci_message_ptr = mem_malloc(malloc_len);

    if (hci_message_ptr != NULL)
    {
        hci_message_ptr->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        hci_message_ptr->ogf = HCI_CMD_INFO_PARAM;
        hci_message_ptr->ocf = INFO_PARAM_READ_LOCAL_VERSION_INFO;
        hci_message_ptr->length = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)hci_message_ptr;

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


/** Read Local Supported Commands command
 *
 */
int8_t hci_read_local_supported_cmds_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *hci_message_ptr;
    int8_t err_code;
    uint16_t malloc_len;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    hci_message_ptr = mem_malloc(malloc_len);

    if (hci_message_ptr != NULL)
    {
        hci_message_ptr->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        hci_message_ptr->ogf = HCI_CMD_INFO_PARAM;
        hci_message_ptr->ocf = INFO_PARAM_READ_LOCAL_SUPPORTED_CMDS;
        hci_message_ptr->length = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)hci_message_ptr;

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


/** Read Local Supported Features command
 *
 */
int8_t hci_read_local_supported_feature_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *hci_message_ptr;
    int8_t err_code;
    uint16_t malloc_len;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    hci_message_ptr = mem_malloc(malloc_len);
    if (hci_message_ptr != NULL)
    {
        hci_message_ptr->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        hci_message_ptr->ogf = HCI_CMD_INFO_PARAM;
        hci_message_ptr->ocf = INFO_PARAM_READ_LOCAL_SUPPORTED_FETAUTES;
        hci_message_ptr->length = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)hci_message_ptr;

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


/** Read BD_ADDR command
 *
 */
int8_t hci_read_bd_addr_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *hci_message_ptr;
    int8_t err_code;
    uint16_t malloc_len;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    hci_message_ptr = mem_malloc(malloc_len);

    if (hci_message_ptr != NULL)
    {
        hci_message_ptr->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        hci_message_ptr->ogf = HCI_CMD_INFO_PARAM;
        hci_message_ptr->ocf = INFO_PARAM_READ_BD_ADDR;
        hci_message_ptr->length = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)hci_message_ptr;

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


/* =======================================
 *  LE Controller Command: OGF = 0x08
 * ======================================= */

/** LE Set Event Mask command
 *
 */
int8_t hci_le_set_event_mask_cmd(uint8_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *hci_message_ptr;
    int8_t err_code;
    uint16_t malloc_len;
    uint8_t cmd_payload_len = 8;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    hci_message_ptr = mem_malloc(malloc_len);

    if (hci_message_ptr != NULL)
    {
        hci_message_ptr->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        hci_message_ptr->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        hci_message_ptr->ocf = LE_SET_EVENT_MASK;
        hci_message_ptr->length = cmd_payload_len;

        memcpy(hci_message_ptr->parameter, p_param, cmd_payload_len);
        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)hci_message_ptr;

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


/** LE Read Buffer Size command
 *
 */
int8_t hci_le_read_buffer_size_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *hci_message_ptr;
    int8_t err_code;
    uint16_t malloc_len;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    hci_message_ptr = mem_malloc(malloc_len);

    if (hci_message_ptr != NULL)
    {
        hci_message_ptr->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        hci_message_ptr->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        hci_message_ptr->ocf = LE_READ_BUFFER_SIZE;
        hci_message_ptr->length = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)hci_message_ptr;
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


/** LE Read Local Supported Features command
 *
 */
int8_t hci_le_read_local_supported_feature_cmd(void)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *hci_message_ptr;
    int8_t err_code;
    uint16_t malloc_len;

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t);
    hci_message_ptr = mem_malloc(malloc_len);

    if (hci_message_ptr != NULL)
    {
        hci_message_ptr->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        hci_message_ptr->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        hci_message_ptr->ocf = LE_READ_LOCAL_SUPPORTED_FEATURES;
        hci_message_ptr->length = 0;

        hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_CMD;
        hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *)hci_message_ptr;
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


/** LE Set Random Address command
 *
 */
int8_t hci_le_set_random_addr_cmd(ble_hci_cmd_set_random_addr_param_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_set_random_addr_param_t *p_hci_cmd_parm;
    int8_t err_code;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_set_random_addr_param_t);

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_SET_RANDOM_DEVICE_ADDRESS;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_set_random_addr_param_t *)p_hci_message->parameter;
        memcpy(p_hci_cmd_parm->addr, p_param->addr, cmd_payload_len);

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


/** LE Read Filter Accept List Size command
 *
 */
int8_t hci_le_read_filter_accept_list_size_cmd(void)
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
        p_hci_message->ocf = LE_READ_FILTER_ACCEPT_LIST_SIZE;
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


/** LE Clear Filter Accept List command
 *
 */
int8_t hci_le_clear_filter_accept_list_cmd(void)
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
        p_hci_message->ocf = LE_CLEAR_FILTER_ACCEPT_LIST;
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


/** LE Add Device To Filter Accept List command
 *
 */
int8_t hci_le_add_device_to_filter_accept_list_cmd(ble_hci_cmd_add_device_to_accept_list_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_add_device_to_accept_list_t *p_hci_cmd_parm;
    int8_t err_code;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_add_device_to_accept_list_t);

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_ADD_DEVICE_TO_FILTER_ACCEPT_LIST;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_add_device_to_accept_list_t *)p_hci_message->parameter;
        memcpy(&p_hci_cmd_parm->addr_type, &p_param->addr_type, cmd_payload_len);

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


/** LE Remove Device From Filter Accept List command
 *
 */
int8_t hci_le_remove_device_from_filter_accept_list_cmd(ble_hci_cmd_remove_device_from_accept_list_t *p_param)
{
    hci_task_common_queue_t hci_comm_msg;
    ble_hci_command_hdr_t *p_hci_message;
    ble_hci_cmd_remove_device_from_accept_list_t *p_hci_cmd_parm;
    int8_t err_code;
    uint16_t malloc_len;
    uint16_t cmd_payload_len = sizeof(ble_hci_cmd_remove_device_from_accept_list_t);

    // set command
    err_code = ERR_OK;
    malloc_len = sizeof(ble_hci_command_hdr_t) + cmd_payload_len;
    p_hci_message = mem_malloc(malloc_len);

    if (p_hci_message != NULL)
    {
        p_hci_message->transport_id = BLE_TRANSPORT_HCI_COMMAND;
        p_hci_message->ogf = HCI_CMD_LE_CONTROLLER_CMD;
        p_hci_message->ocf = LE_REMOVE_DEVICE_FROM_FILTER_ACCEPT_LIST;
        p_hci_message->length = cmd_payload_len;

        p_hci_cmd_parm = (ble_hci_cmd_remove_device_from_accept_list_t *)p_hci_message->parameter;
        memcpy(&p_hci_cmd_parm->addr_type, &p_param->addr_type, cmd_payload_len);

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



/** LE Read Supported States command
 *
 */
int8_t hci_le_read_supported_state_cmd(void)
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
        p_hci_message->ocf = LE_READ_SUPPORTED_STATES;
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


/** LE Read Antenna Information command
 *
 */
int8_t hci_le_read_antenna_info_cmd(void)
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
        p_hci_message->ocf = LE_READ_ANTENNA_INFORMATION;
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


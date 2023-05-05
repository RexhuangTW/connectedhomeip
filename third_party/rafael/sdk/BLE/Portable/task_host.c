/** @file task_host.c
 *
 * @brief Handle BLE HCI event and ACL data.
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "ble_printf.h"
#include "sys_arch.h"
#include <stdint.h>

#include "ble_api.h"
#include "ble_att_gatt_api.h"
#include "ble_common_api.h"
#include "ble_event_module.h"
#include "ble_host_cmd.h"
#include "ble_host_ref.h"
#include "ble_memory.h"
#include "ble_profile.h"
#include "task_ble_app.h"
#include "task_hci.h"
#include "task_host.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
// BLE Host subsystem task
#define CONFIG_HOST_SUBSYSTEM_PRIORITY TASK_PRIORITY_PROTOCOL_NORMAL /**< Task priority. */
#define CONFIG_HOST_SUBSYSTEM_STACK_SIZE 128                         /**< Task stack size. */

// BLE cmd transport task
#define CONFIG_CMD_TRANSPORT_PRIORITY TASK_PRIORITY_PROTOCOL_NORMAL /**< Task priority. */
#define CONFIG_CMD_TRANSPORT_STACK_SIZE 128                         /**< Task stack size. */

#define CONFIG_HOST_SW_TIMER_TICK_COUNT 10 /**< host sw timer tick count. */

#define TASK_DELAY_MS (4) /* task delay time (ms) */

#define CONFIG_RX_CMD_SEMAPHORE_SIZE 10
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
sys_queue_t g_host_rx_handle;       /**< The handle of BLE host RX queue. */
sys_queue_t g_host_encrypt_handle;  /**< The handle of BLE host encryption queue. */
sys_queue_t g_cmd_transport_handle; /**< The handle of command transport queue. */
sys_sem_t g_host_rx_cmd_sem;        /**< The handle of host Rx command semaphore. */
sys_sem_t g_host_rx_data_sem;       /**< The handle of host Rx data semaphore. */

TimerHandle_t g_ble_host_timer; /**< The handle of BLE host sw timer. */

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
static sys_task_t ble_host_subsystem_task;
static sys_task_t ble_cmd_transport_task;

static sys_queue_t g_host_acl_tx_handle; /**< The handle of BLE host ACL data queue. */
static sys_queue_t g_host_to_app_handle; /**< The handle of BLE host to APP event queue. */

static uint8_t acl_data[251];
/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
/**@brief check if there is any message from the host created queue. */
static void host_check_queue(void)
{
    uint32_t recv_msg_time;
    ble_host_to_app_evt_t * p_queue_param;
    ble_hci_tx_acl_data_hdr_t * pt_tlv;
    MBLK * p_mblk;

    recv_msg_time = SYS_ARCH_TIMEOUT;
    if ((sys_queue_remaining_size(&g_host_acl_tx_handle) != QUEUE_HOST_TO_HCI_ACL_DATA) ||
        (sys_queue_remaining_size(&g_host_to_app_handle) != QUEUE_HOST_TO_APP_EVENT))
    {
        if (sys_queue_remaining_size(&g_hci_common_handle) > 5) // interrupt to hci max number (1 + 4)
        {
            if (sys_arch_queue_tryrecv(&g_host_acl_tx_handle, &pt_tlv) != SYS_ARCH_TIMEOUT)
            {
                hci_task_common_queue_t hci_comm_msg;

                hci_comm_msg.hci_msg_tag = HCI_MSG_HOST_HCI_ACL_DATA;
                hci_comm_msg.hci_msg_ptr = (ble_hci_message_struct_t *) pt_tlv;
                if (sys_queue_trysend(&g_hci_common_handle, (void *) &hci_comm_msg) != ERR_OK)
                {
                    sys_queue_sendtofront(&g_host_acl_tx_handle, &pt_tlv);
                    BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] acl data to hci fail\n");
                }
            }
        }
        else
        {
            sys_task_delay(TASK_DELAY_MS);
        }

        if ((sys_queue_remaining_size(&g_host_acl_tx_handle) >= 5) && (sys_queue_remaining_size(&g_host_to_app_handle) >= 2))
        {
            recv_msg_time = sys_arch_queue_tryrecv(&g_host_rx_handle, &p_mblk);
        }

        if (get_app_queue_remaining_size() > 1)
        {
            if (sys_arch_queue_tryrecv(&g_host_to_app_handle, &p_queue_param) != SYS_ARCH_TIMEOUT)
            {
                ble_app_evt_param_t app_msg;

                app_msg.type = p_queue_param->evt_type;
                switch (p_queue_param->evt_type)
                {
                case BLE_APP_GENERAL_EVENT:
                case BLE_APP_RETURN_PARAMETER_EVENT: {
                    ble_evt_param_t * p_evt_param;

                    p_evt_param = mem_malloc(sizeof(ble_evt_param_t));
                    while (p_evt_param == NULL)
                    {
                        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                        sys_task_delay(TASK_DELAY_MS);
                        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                        p_evt_param = mem_malloc(sizeof(ble_evt_param_t));
                    }
                    memcpy(p_evt_param, p_queue_param->parameter, sizeof(ble_evt_param_t));
                    app_msg.event_param.p_ble_evt_param = p_evt_param;
                    if (task_ble_app_queue_send(app_msg) != BLE_ERR_OK)
                    {
                        mem_free(app_msg.event_param.p_ble_evt_param);
                        sys_queue_sendtofront(&g_host_to_app_handle, &p_queue_param);
                        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] event to APP fail\n");
                    }
                    else
                    {
                        mem_free(p_queue_param);
                        sys_task_delay(TASK_DELAY_MS >> 1);
                    }
                }
                break;

                case BLE_APP_SERVICE_EVENT: {
                    ble_evt_att_param_t * p_evt_att_param;
                    uint16_t malloc_len;

                    p_evt_att_param = (ble_evt_att_param_t *) p_queue_param->parameter;
                    malloc_len      = p_evt_att_param->length;
                    p_evt_att_param = mem_malloc(sizeof(ble_evt_att_param_t) + malloc_len);
                    while (p_evt_att_param == NULL)
                    {
                        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] TAA.\n"); // try to allocate again!
                        Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HOST);
                        sys_task_delay(TASK_DELAY_MS);
                        Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HOST);
                        p_evt_att_param = mem_malloc(sizeof(ble_evt_att_param_t) + malloc_len);
                    }
                    memcpy(p_evt_att_param, p_queue_param->parameter, sizeof(ble_evt_att_param_t) + malloc_len);
                    app_msg.event_param.p_ble_service_param = p_evt_att_param;
                    if (task_ble_app_queue_send(app_msg) != BLE_ERR_OK)
                    {
                        mem_free(app_msg.event_param.p_ble_service_param);
                        sys_queue_sendtofront(&g_host_to_app_handle, &p_queue_param);
                        BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] event to APP fail\n");
                    }
                    else
                    {
                        mem_free(p_queue_param);
                    }
                }
                break;

                default:
                    app_msg.event_param.p_ble_service_param = NULL;
                    mem_free(p_queue_param);
                    break;
                }
            }
        }
        else
        {
            if (recv_msg_time == SYS_ARCH_TIMEOUT)
            {
                sys_task_delay(TASK_DELAY_MS);
            }
        }
    }
    else
    {
        recv_msg_time = sys_queue_recv(&g_host_rx_handle, &p_mblk, 0);
    }

    if (recv_msg_time != SYS_ARCH_TIMEOUT)
    {
        switch (p_mblk->primitive)
        {
        case HOST_MSG_RX_HCI_EVENT: {
            host_rx_event_param_t * p_evt_param;

            p_evt_param = (host_rx_event_param_t *) p_mblk->para.Data;
            if (p_evt_param->event_code <= HCI_EVENT_CODE_SAM_STATUS_CHANGE)
            {
                if (host_prcss_hci_event[p_evt_param->event_code](p_evt_param->parameter, p_mblk->length) == ERR_OK)
                {
                    uint16_t i;

                    BLE_PRINTF(BLE_DEBUG_HCI_CMD_EVT, "[HCI_RX_E] EVT = %02x ", p_evt_param->event_code);
                    for (i = 0; i < p_mblk->length; i++)
                    {
                        BLE_PRINTF(BLE_DEBUG_HCI_CMD_EVT, "%02x ", *(p_evt_param->parameter + i));
                    }
                    BLE_PRINTF(BLE_DEBUG_HCI_CMD_EVT, "\n");
                    msgblks_free(p_mblk);
                    sys_sem_signal(&g_host_rx_cmd_sem);
                }
                else
                {
                    sys_queue_sendtofront(&g_host_rx_handle, &p_mblk);
                    sys_task_delay(TASK_DELAY_MS);
                }
            }
            else if (p_evt_param->event_code == HCI_EVENT_CODE_VENDOR_EVENT)
            {
                if (prcss_hci_vendor_event(p_evt_param->parameter, p_mblk->length) == ERR_OK)
                {
                    uint16_t i;
                    BLE_PRINTF(BLE_DEBUG_HCI_CMD_EVT, "[HCI_RX_E] EVT = %02x ", p_evt_param->event_code);
                    for (i = 0; i < p_mblk->length; i++)
                    {
                        BLE_PRINTF(BLE_DEBUG_HCI_CMD_EVT, "%02x ", *(p_evt_param->parameter + i));
                    }
                    BLE_PRINTF(BLE_DEBUG_HCI_CMD_EVT, "\n");
                    msgblks_free(p_mblk);
                    sys_sem_signal(&g_host_rx_cmd_sem);
                }
                else
                {
                    sys_queue_sendtofront(&g_host_rx_handle, &p_mblk);
                    sys_task_delay(TASK_DELAY_MS);
                }
            }
            else
            {
                BLE_PRINTF(BLE_DEBUG_HCI_CMD_EVT, "unknown event:%x\n", p_evt_param->event_code);
                msgblks_free(p_mblk);
                sys_sem_signal(&g_host_rx_cmd_sem);
            }
        }
        break;

        case HOST_MSG_RX_HCI_ACL_DATA: {
            host_rx_acl_data_param_t * p_acl_data_param;

            p_acl_data_param = (host_rx_acl_data_param_t *) p_mblk->para.Data;
            get_acl_data_from_msgblks(acl_data, p_mblk);
            if (bhc_host_prcss_l2cap_cid(p_acl_data_param->conn_handle, acl_data, p_mblk->length) == ERR_OK)
            {
                uint16_t i;

                BLE_PRINTF(BLE_DEBUG_HCI_DATA, "[HCI_RX_D] handle 0x%04x, len:0x%02x\n", p_acl_data_param->conn_handle,
                           p_mblk->length);
                BLE_PRINTF(BLE_DEBUG_HCI_DATA, "[HCI_RX_D] ACL data = ");
                for (i = 0; i < p_mblk->length; i++)
                {
                    BLE_PRINTF(BLE_DEBUG_HCI_DATA, "%02x ", acl_data[i]);
                }
                BLE_PRINTF(BLE_DEBUG_HCI_DATA, "\n");
                msgblks_free(p_mblk);
                sys_sem_signal(&g_host_rx_data_sem);
            }
            else
            {
                sys_queue_sendtofront(&g_host_rx_handle, &p_mblk);
                sys_task_delay(TASK_DELAY_MS);
            }
        }
        break;

        case HOST_MSG_DB_PARSING_EVENT:
            bhc_db_parsing_evt_handle((void *) p_mblk);
            msgblks_free(p_mblk);
            break;

        case HOST_MSG_AUTHE_EVENT:
            bhc_authentication_evt_handle((void *) p_mblk);
            msgblks_free(p_mblk);
            break;

        case HOST_MSG_AUTHO_EVENT:
            bhc_authorization_evt_handle((void *) p_mblk);
            msgblks_free(p_mblk);
            break;

        case HOST_MSG_BY_PASS_GENERAL_EVENT: {
            ble_host_to_app_evt_t * p_queue_param;
            ble_evt_param_t * p_evt_param;

            p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
            if (p_queue_param != NULL)
            {
                p_queue_param->u32_send_systick = sys_now();
                p_queue_param->evt_type         = BLE_APP_GENERAL_EVENT;

                p_evt_param = (ble_evt_param_t *) p_queue_param->parameter;
                memcpy(p_evt_param, p_mblk->para.Data, p_mblk->length);
                task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
                msgblks_free(p_mblk);
            }
            else
            {
                sys_queue_sendtofront(&g_host_rx_handle, &p_mblk);
                sys_task_delay(TASK_DELAY_MS);
            }
        }
        break;

        case HOST_MSG_SM_BOND_SPACE_INT:
            bhc_sm_prcss_bonding_space_init();
            msgblks_free(p_mblk);
            break;

        default:
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_HOST] %s recv unknow type %d (TLV %p(%d))\n", sys_current_thread_name(),
                       p_mblk->primitive, p_mblk, p_mblk->length + sizeof(MBLK));

            BLE_BDUMP(BLE_DEBUG_ERR, p_mblk, p_mblk->length + sizeof(MBLK));
            msgblks_free(p_mblk);
            break;
        }
    }
}

/**@brief BLE host task event handler. */
static void ble_host_subsystem_handle(void)
{
    for (;;)
    {
        host_check_queue();
    }
}

/**@brief check if there is any message from the host created queue. */
static void cmd_transport_check_queue(void)
{
    uint32_t recv_msg_time;
    task_queue_t recv_msg;
    task_queue_t confirm_msg;
    ble_err_t err_code;

    recv_msg_time = SYS_ARCH_TIMEOUT;
    recv_msg_time = sys_queue_recv(&g_cmd_transport_handle, &recv_msg, 0);
    if (recv_msg_time != SYS_ARCH_TIMEOUT)
    {
        // API dispatcher
        err_code = prcss_api_cmd_transport(recv_msg.pt_tlv->type, recv_msg.pt_tlv->value);

        // Confirmation info
        confirm_msg.u32_send_systick = sys_now();
        confirm_msg.pt_tlv           = mem_malloc(sizeof(sys_tlv_t) + sizeof(uint8_t));
        confirm_msg.pt_tlv->type     = recv_msg.pt_tlv->type; // cfm type
        confirm_msg.pt_tlv->length   = 1;                     // cfm len
        *confirm_msg.pt_tlv->value   = err_code;

        sys_queue_send(&g_ble_cfm_queue, (void *) &confirm_msg);

        // free
        mem_free(recv_msg.pt_tlv);
    }
}

/**@brief BLE host task event handler. */
static void ble_cmd_transport_handle(void)
{
    for (;;)
    {
        cmd_transport_check_queue();
    }
}

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/**@brief Retrun the space of free space in BLE host ACL data queue. */
uint32_t host_acl_data_queue_remaining_size(void)
{
    return sys_queue_remaining_size(&g_hci_tx_acl_handle);
}

/**@brief Retrun the space of free space in BLE host ACL data queue. */
uint32_t host_encrypt_queue_remaining_size(void)
{
    return sys_queue_remaining_size(&g_host_encrypt_handle);
}

/**@brief Post the data on the BLE host queue by @ref task_host_queue_type_t "the queue type". */
void task_host_queue_send(task_host_queue_type_t queue_type, void * p_data)
{
    switch (queue_type)
    {
    case TASK_HOST_QUEUE_TX_ACL_DATA:
        sys_queue_send(&g_host_acl_tx_handle, (void *) &p_data);
        break;

    case TASK_HOST_QUEUE_TO_APP:
        sys_queue_send(&g_host_to_app_handle, (void *) &p_data);
        break;

    case TASK_HOST_QUEUE_TO_RX_COMMON:
        sys_queue_send(&g_host_rx_handle, (void *) &p_data);
        break;

    default:
        break;
    }
}

/**@brief Post the command to HCI task on the BLE HCI common queue. */
uint8_t host_send_cmd_to_hci(hci_task_common_queue_t hci_comm_msg)
{
    uint8_t status = FALSE;

    vPortEnterCritical();
    if ((sys_queue_remaining_size(&g_hci_tx_cmd_handle) > (NUM_QUEUE_HCI_COMMAND_EVENT >> 1)) &&
        (sys_queue_remaining_size(&g_hci_common_handle) > 5))
    {
        if (sys_queue_trysend(&g_hci_common_handle, (void *) &hci_comm_msg) == ERR_OK)
        {
            status = TRUE;
        }
        else
        {
            BLE_PRINTF(BLE_DEBUG_INFO, "[BLE_DI] cmd ocf:%d ogf:%d fail\n", hci_comm_msg.hci_msg_ptr->msg_type.hci_command.ocf,
                       hci_comm_msg.hci_msg_ptr->msg_type.hci_command.ogf);
        }
    }
    vPortExitCritical();

    return status;
}

/**@brief Post the ACL data completed event to HCI task on the BLE HCI common queue. */
int8_t host_notify_acl_data_complete(uint16_t num_of_complete_acl_data)
{
    ble_hci_message_struct_t * p_hci_message;
    hci_task_common_queue_t hci_comm_msg;
    int8_t err_code;

    err_code = ERR_OK;
    do
    {
        // notify task HCI the buffer number of TX ACK packet
        p_hci_message = (ble_hci_message_struct_t *) mem_malloc(sizeof(ble_hci_acl_data_complete_event_t));
        if (p_hci_message != NULL)
        {
            p_hci_message->msg_type.hci_data.complete_num = num_of_complete_acl_data;
            hci_comm_msg.hci_msg_tag                      = HCI_MSG_HOST_NOCP_EVENT;
            hci_comm_msg.hci_msg_ptr                      = (ble_hci_message_struct_t *) p_hci_message;
            sys_queue_send(&g_hci_common_handle, (void *) &hci_comm_msg);
        }
        else
        {
            err_code = ERR_MEM;
        }
    } while (0);
    return err_code;
}

void ble_host_timer_cb(TimerHandle_t xTimer)
{
    uint8_t i, j, no_evt;
    uint32_t timer_count;
    MBLK * p_mblk;
    ble_evt_param_t * p_evt_param;

    timer_count = (uint32_t) pvTimerGetTimerID(xTimer);

    no_evt = 1;
    for (i = 0; i < MAX_CONN_NO_APP; i++) // hostId
    {
        for (j = 0; j < TOTAL_TIMER_EVENT; j++)
        {
            if ((host_timer + j)[i * TOTAL_TIMER_EVENT].event != TIMER_EVENT_NULL)
            {
                no_evt = 0;
                if (((timer_count - (host_timer + j)[i * TOTAL_TIMER_EVENT].current_time)) >
                    (host_timer + j)[i * TOTAL_TIMER_EVENT].timeout_base)
                {
                    switch ((host_timer + j)[i * TOTAL_TIMER_EVENT].event)
                    {
                    case TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP:
                    case TIMER_EVENT_CONN_UPDATE_COMPLETE: {
                        p_mblk = get_msgblks_L1(sizeof(ble_evt_param_t));
                        if (p_mblk != NULL)
                        {
                            p_mblk->primitive  = HOST_MSG_BY_PASS_GENERAL_EVENT;
                            p_mblk->length     = sizeof(ble_evt_param_t);
                            p_evt_param        = (ble_evt_param_t *) p_mblk->para.Data;
                            p_evt_param->event = BLE_GAP_EVT_CONN_PARAM_UPDATE;
                            p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.host_id        = i;
                            p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.status         = BLE_ERR_CODE_TIMEOUT;
                            p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.conn_interval  = 0;
                            p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.periph_latency = 0;
                            p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.supv_timeout   = 0;

                            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
                            (host_timer + j)[i * TOTAL_TIMER_EVENT].event = TIMER_EVENT_NULL; // close CmdTimer
                        }
                    }
                    break;

                    case TIMER_EVENT_AUTH_STATUS: {
                        p_mblk = get_msgblks_L1(sizeof(ble_evt_param_t));
                        if (p_mblk != NULL)
                        {
                            p_mblk->primitive  = HOST_MSG_BY_PASS_GENERAL_EVENT;
                            p_mblk->length     = sizeof(ble_evt_param_t);
                            p_evt_param        = (ble_evt_param_t *) p_mblk->para.Data;
                            p_evt_param->event = BLE_SM_EVT_AUTH_STATUS;
                            p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.host_id = i;
                            p_evt_param->event_param.ble_evt_sm.param.evt_auth_status.status  = BLE_ERR_CODE_TIMEOUT;

                            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
                            (host_timer + j)[i * TOTAL_TIMER_EVENT].event = TIMER_EVENT_NULL; // close CmdTimer
                        }
                    }
                    break;

                    case TIMER_EVENT_CLIENT_DB_PARSING: {
                        p_mblk = get_msgblks_L1(sizeof(ble_evt_param_t));
                        if (p_mblk != NULL)
                        {
                            p_mblk->primitive  = HOST_MSG_BY_PASS_GENERAL_EVENT;
                            p_mblk->length     = sizeof(ble_evt_param_t);
                            p_evt_param        = (ble_evt_param_t *) p_mblk->para.Data;
                            p_evt_param->event = BLE_ATT_GATT_EVT_DB_PARSE_COMPLETE;
                            p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_db_parse_complete.host_id = i;
                            p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_att_db_parse_complete.result =
                                BLE_ERR_CODE_TIMEOUT;

                            task_host_queue_send(TASK_HOST_QUEUE_TO_RX_COMMON, p_mblk);
                            (host_timer + j)[i * TOTAL_TIMER_EVENT].event = TIMER_EVENT_NULL; // close CmdTimer
                        }
                    }
                    break;

                    default:
                        (host_timer + j)[i * TOTAL_TIMER_EVENT].event = TIMER_EVENT_NULL; // close CmdTimer
                        break;
                    }
                }
            }
        }
    }

    timer_count++;
    vTimerSetTimerID(xTimer, (void *) timer_count);

    if (no_evt)
    {
        xTimerStop(xTimer, 0);
    }
}

int8_t ble_delete_host_subsystem(void)
{
    int8_t err_code = ERR_OK;

    /*-----------------------------------*/
    /* A.Input Parameter Range Check     */
    /*-----------------------------------*/

    /*-----------------------------------*/
    /* B. Main Functionality             */
    /*-----------------------------------*/
    do
    {
        sys_queue_free(&g_host_rx_handle);
        sys_queue_free(&g_host_acl_tx_handle);
        sys_queue_free(&g_host_to_app_handle);
        sys_queue_free(&g_host_encrypt_handle);
        sys_queue_free(&g_host_rx_cmd_sem);
        sys_queue_free(&g_host_rx_data_sem);

        vTaskDelete(ble_host_subsystem_task);

        sys_queue_free(&g_cmd_transport_handle);

        vTaskDelete(ble_cmd_transport_task);
    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    return err_code;
}

/**@brief BLE host subsystem initialization. */
int8_t ble_host_subsystem_init(void)
{
    int8_t err_code = ERR_OK;

    /*-----------------------------------*/
    /* A.Input Parameter Range Check     */
    /*-----------------------------------*/

    /*-----------------------------------*/
    /* B. Main Functionality             */
    /*-----------------------------------*/
    do
    {
        // check maximum BLE connection links configurations
        // "max_num_conn_host" defined in ble_profile_def.c
        // "BLE_SUPPORT_NUM_CONN_MAX" defined in profile_config.h
        if (max_num_conn_host < MAX_CONN_NO_APP)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] BLE host connection link configuration failed.\n");
            err_code = -1;
            break;
        }

        // init BLE host
        bhc_att_param_init();
        bhc_bonding_storage_init();
        ble_memory_init();

        if (sys_queue_new(&g_host_rx_handle, QUEUE_HOST_RX, sizeof(MBLK *)) != ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] g_host_rx_handle fail\n");
        }

        if (sys_queue_new(&g_host_acl_tx_handle, QUEUE_HOST_TO_HCI_ACL_DATA, sizeof(ble_hci_tx_acl_data_hdr_t *)) != ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] g_host_acl_tx_handle fail\n");
        }

        if (sys_queue_new(&g_host_to_app_handle, QUEUE_HOST_TO_APP_EVENT, sizeof(ble_host_to_app_evt_t *)) != ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] g_host_to_app_handle fail\n");
        }

        if (sys_queue_new(&g_host_encrypt_handle, QUEUE_HOST_ENCRYPT, sizeof(encrypt_queue_t)) != ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] g_host_encrypt_handle fail\n");
        }

        if (sys_sem_new(&g_host_rx_cmd_sem, CONFIG_RX_CMD_SEMAPHORE_SIZE, CONFIG_RX_CMD_SEMAPHORE_SIZE) != ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] g_host_rx_cmd_sem fail\n");
        }
        if (sys_sem_new(&g_host_rx_data_sem, (QUEUE_HOST_RX - CONFIG_RX_CMD_SEMAPHORE_SIZE - 10),
                        (QUEUE_HOST_RX - CONFIG_RX_CMD_SEMAPHORE_SIZE - 10)) != ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] g_host_rx_data_sem fail\n");
        }

        if (sys_queue_new(&g_cmd_transport_handle, 6, sizeof(task_queue_t)) != ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] g_cmd_transport_handle fail\n");
        }

        g_ble_host_timer = xTimerCreate("BLE_HOST_TIMER", CONFIG_HOST_SW_TIMER_TICK_COUNT, pdTRUE, 0, ble_host_timer_cb);
        if (g_ble_host_timer == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] host sw timer create fail\n");
        }

        ble_cmd_transport_task = sys_task_new("BLE_CMD_TRANSPORT", (TaskFunction_t) ble_cmd_transport_handle, NULL,
                                              CONFIG_CMD_TRANSPORT_STACK_SIZE, CONFIG_CMD_TRANSPORT_PRIORITY);

        if (ble_cmd_transport_task == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] ble command transport layer create failed.\n");
        }

        ble_host_subsystem_task = sys_task_new("HOST_SUBSYSTEM", (TaskFunction_t) ble_host_subsystem_handle, NULL,
                                               CONFIG_HOST_SUBSYSTEM_STACK_SIZE, CONFIG_HOST_SUBSYSTEM_PRIORITY);
        if (ble_host_subsystem_task == NULL)
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_DEBUG_ERR] host subsystem create failed.\n");
        }

    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    return err_code;
}

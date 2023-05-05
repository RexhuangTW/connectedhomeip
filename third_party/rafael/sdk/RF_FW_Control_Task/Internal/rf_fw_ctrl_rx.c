/** @file task_hci.c
 *
 * @brief Handle BLE application request, BLE event and BLE service data.
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "hci_pci_printf.h"
#include "project_config.h"
#include "rf_fw_ctrl_tx.h"
#include "rf_mcu.h"
#include "sys_arch.h"
#if (MODULE_ENABLE(SUPPORT_15P4))
#include "rfb_comm_common.h"
#include "ruci.h"
#include "ruci_head.h"
#include "task_pci.h"
#endif
#if (MODULE_ENABLE(SUPPORT_BLE))
#include "ble_hci.h"
#include "ble_memory.h"
#include "task_hci.h"
#include "task_host.h"
#endif

#include "mib_counters.h"
/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define ISR_MSG_EVENT_GET_FROM_AHB (0x00) /**< Message for isr to notify main loop to read RX event from AHB. */
#define ISR_MSG_EVENT_GET_FROM_MSG (0x01) /**< Message for isr to notify main loop to get RX event from message. */
#define ISR_MSG_DATA_GET_FROM_AHB (0x02)  /**< Message for isr to notify main loop to read RX data from AHB*/
#define ISR_MSG_TRAP (0x03)               /**< Message for isr to notify that controller fall into trap */
#define ISR_MSG_TX_DONE (0x04)            /**< Message for isr to notify TX done */
#define ISR_MSG_MAX (0x10)

#define INVALID_HANDLE (0xFFFF) /* specific number for invalid handle */
#define START_FRAME (0x02)      /* First automatically flushable packet of higher layer message */
#define CONTINUE_FRAME (0x01)   /* Continuing fragment of higher layer message*/

#define L2CAP_HEADER_SIZE (0x04)            /* header of L2CAP which contains two fields, length(2 octets) + channel ID(2 octets) */
#define L2CAP_HEADER_LENGTH_MAX_SIZE (1024) /* max size for L2CAP Header Fields*/

#define TASK_DELAY_MS (2) /* task delay time (ms) */

#define CONFIG_RX_TASK_PRIORITY TASK_PRIORITY_PROTOCOL_MEDIUM /**< Task priority. */
#define CONFIG_RX_TASK_STACK_SIZE (512)

#define NUM_HW_RX_DATA_Q (4)       /**< Mapping to RX data queue number of HW. */
#define NUM_HW_RX_EVENT_Q (1)      /**< Mapping to RX event queue number of HW. */
#define MAX_RESERVED_EVENT_NUM (3) /**< reserved for PHY update event + Disconnect event + command complete event. */

#define NUM_QUEUE_RX_COMMON (((NUM_HW_RX_DATA_Q + MAX_RESERVED_EVENT_NUM) << 1))

#define RF_ISR_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)

#if (MODULE_ENABLE(SUPPORT_BLE))
#define HCI_EVENT_FOR_HOST(event) (event == HCI_EVENT_CODE_NUMBER_OF_COMPLETE_PACKETS) ? FALSE : TRUE

#define HCI_EVENT(event) (event == BLE_TRANSPORT_HCI_EVENT) ? TRUE : FALSE
#define HCI_REASSEMBLE_BUFFER_INIT(idx)                                                                                            \
    reassemble_acl_data[idx].conn_handle       = INVALID_HANDLE;                                                                   \
    reassemble_acl_data[idx].acl_data_ptr      = NULL;                                                                             \
    reassemble_acl_data[idx].rcvd_data_length  = 0;                                                                                \
    reassemble_acl_data[idx].total_data_length = 0

#endif

#if (MODULE_ENABLE(SUPPORT_15P4))
#define PCI_EVENT(event)                                                                                                           \
    (event == RUCI_PCI_EVENT_HEADER)           ? TRUE                                                                              \
        : (event == RUCI_SF_HOST_EVENT_HEADER) ? TRUE                                                                              \
        : (event == RUCI_CMN_EVENT_HEADER)     ? TRUE                                                                              \
                                               : FALSE

#define PCI_CMD_COMPLETE_EVENT(data)                                                                                               \
    ((((ruci_para_cnf_event_t *) data)->ruci_header.u8 == RUCI_PCI_EVENT_HEADER) &&                                                \
     (((ruci_para_cnf_event_t *) data)->sub_header == RUCI_CODE_CNF_EVENT))                                                        \
        ? TRUE                                                                                                                     \
        : ((((ruci_para_cnf_event_t *) data)->ruci_header.u8 == RUCI_CMN_EVENT_HEADER) &&                                          \
           (((ruci_para_cnf_event_t *) data)->sub_header == RUCI_CODE_GET_FW_VER_EVENT))                                           \
        ? TRUE                                                                                                                     \
        : FALSE
#endif
/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
sys_queue_t g_rx_common_queue_handle;

extern sys_sem_t g_tx_cmd_sem;
extern sys_sem_t g_tx_data_sem;

sys_task_t g_phci_rx_task;
/**************************************************************************************************
 *    LOCAL TYPEDEFS
 *************************************************************************************************/
#if (MODULE_ENABLE(SUPPORT_15P4))
typedef struct
{
    uint16_t Length;
    uint8_t CrcStatus;
    uint8_t Rssi;
    uint8_t Snr;
} zb_rx_ctrl_field_t;
#endif

typedef struct
{
    uint32_t msg_tag; /**< message tag. */
#if (MODULE_ENABLE(SUPPORT_15P4))
    uint32_t param_var; /**< message variable (maximum 4 byte).*/
#endif
#if (MODULE_ENABLE(SUPPORT_BLE))
    MBLK * p_mblk; /**< message data pointer.*/
#endif

} rf_fw_rx_ctrl_msg_t;

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
uint32_t event_cnt = 0;

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
void rf_fw_isr_hander(uint8_t interrupt_status)
{
    COMM_SUBSYSTEM_INTERRUPT interrupt_state_value;
    rf_fw_rx_ctrl_msg_t isr_msg;

    interrupt_state_value.u8 = interrupt_status;

    if (interrupt_state_value.bf.RFB_TRAP) // bit 2
    {
        isr_msg.msg_tag = ISR_MSG_TRAP;
        sys_queue_send_from_isr(&g_rx_common_queue_handle, &isr_msg);
    }

    if (interrupt_state_value.bf.EVENT_DONE) // bit 0
    {
        isr_msg.msg_tag = ISR_MSG_EVENT_GET_FROM_AHB;

        sys_queue_send_from_isr(&g_rx_common_queue_handle, &isr_msg);
    }

#if (MODULE_ENABLE(SUPPORT_15P4))
    if (interrupt_state_value.bf.TX_DONE) // bit 1
    {
        uint8_t mcu_status = (uint8_t) RfMcu_McuStateRead();

        mcu_status = (mcu_status & ~RF_MCU_STATE_EVENT_DONE);
        /* Clear MCU state by sending host command */
        RfMcu_HostCmdSet((mcu_status & 0xF4));

        isr_msg.msg_tag   = ISR_MSG_TX_DONE;
        isr_msg.param_var = mcu_status;
        sys_queue_send_from_isr(&g_rx_common_queue_handle, &isr_msg);

        mib_counter_increase(RfIsrTxDoneCount);
    }
#endif

    if (interrupt_state_value.bf.RX_DONE) // //bit 5
    {
        isr_msg.msg_tag = ISR_MSG_DATA_GET_FROM_AHB;
        sys_queue_send_from_isr(&g_rx_common_queue_handle, &isr_msg);
    }
    RfMcu_InterruptClear(interrupt_status);
}

void notify_tx_task_tx_done(uint8_t tx_type)
{
    rf_fw_ctrl_msg_t rf_fw_comm_msg;

    (tx_type == TX_TYPE_DATA) ? sys_sem_signal(&g_tx_data_sem) : sys_sem_signal(&g_tx_cmd_sem);

    rf_fw_comm_msg.msg_tag = INTERNAL_MSG_TX_DONE;
    if (sys_queue_trysend(&g_rfc_common_queue_handle, (void *) &rf_fw_comm_msg) != ERR_OK)
    {
        HPCI_PRINTF(COMMON_DEBUG_INFO, "[COMMON_DEBUG_INFO] notify TX done fail\n");
    }
}

#if (MODULE_ENABLE(SUPPORT_BLE))
void hci_reassemble_acl_data_init(void)
{
    uint8_t i;

    for (i = 0; i < MAX_CONN_NO_APP; i++)
    {
        HCI_REASSEMBLE_BUFFER_INIT(i);
    }
}

void hci_reassemble_acl_data(uint8_t * acl_pkt_ptr)
{
    ble_hci_message_struct_t * hci_message_ptr = (ble_hci_message_struct_t *) acl_pkt_ptr;
    uint8_t idx, pb_flag, *data;
    uint16_t handle, data_len;

    handle   = hci_message_ptr->msg_type.hci_rx_acl_data.handle;
    data     = hci_message_ptr->msg_type.hci_rx_acl_data.data;
    data_len = hci_message_ptr->msg_type.hci_rx_acl_data.length;
    pb_flag  = hci_message_ptr->msg_type.hci_rx_acl_data.pb_flag;

    // find the index with same handle
    for (idx = 0; idx < MAX_CONN_NO_APP; idx++)
    {
        if (reassemble_acl_data[idx].conn_handle == handle)
        {
            break;
        }
    }

    // if index not found, find the index with empty handle
    if (idx == MAX_CONN_NO_APP)
    {
        for (idx = 0; idx < MAX_CONN_NO_APP; idx++)
        {
            if (reassemble_acl_data[idx].conn_handle == INVALID_HANDLE)
            {
                reassemble_acl_data[idx].conn_handle = handle;
                break;
            }
        }
    }

    // if available index not found, drop the data of index 0
    if (idx == MAX_CONN_NO_APP)
    {
        HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] assemble buffer full, curr handle: 0x%04x\n", handle);
        for (idx = 0; idx < MAX_CONN_NO_APP; idx++)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] bufferring handle: 0x%04x\n", reassemble_acl_data[idx].conn_handle);
        }

        idx = 0;
        HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] drop the data of handle: 0x%04x\n", reassemble_acl_data[idx].conn_handle);
        if (reassemble_acl_data[idx].acl_data_ptr != NULL)
        {
            msgblks_free(reassemble_acl_data[idx].acl_data_ptr);
            reassemble_acl_data[idx].acl_data_ptr      = NULL;
            reassemble_acl_data[idx].total_data_length = 0;
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] drop unfinish ACL data!!!\n");
        }
    }

    // try to assemble packet
    if (pb_flag == START_FRAME)
    {
        MBLK * mblk;
        host_rx_acl_data_param_t * p_acl_data;
        uint16_t value_len, total_len;
        get_acl_data_t p_acl_data_param;

        if (reassemble_acl_data[idx].acl_data_ptr != NULL)
        {
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] drop unfinish ACL data!!!\n");
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] handle 0x%04x, totalLen %d, rxLen %d\n",
                        reassemble_acl_data[idx].conn_handle, reassemble_acl_data[idx].total_data_length,
                        reassemble_acl_data[idx].rcvd_data_length);
            msgblks_free(reassemble_acl_data[idx].acl_data_ptr);
        }
        // get packet total length
        total_len = data[0] + (data[1] << 8);

        if ((total_len < L2CAP_HEADER_LENGTH_MAX_SIZE) &&
            ((data_len > L2CAP_HEADER_SIZE) && (total_len >= (data_len - L2CAP_HEADER_SIZE))))
        {
            //....initial reassemble_acl_data
            reassemble_acl_data[idx].total_data_length = total_len;
            value_len                                  = reassemble_acl_data[idx].total_data_length + L2CAP_HEADER_SIZE;

            while (check_msgblk_L1_size(value_len + 251) == ERR_MEM)
            {
                HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] acl rx check msg size fail\n");
                Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HCI);
                sys_task_delay(TASK_DELAY_MS);
                Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HCI);
            }
            mblk = get_msgblks_L1(value_len);
            while (mblk == NULL)
            {
                HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] acl data msg malloc fail\n");
                Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HCI);
                sys_task_delay(TASK_DELAY_MS);
                Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HCI);
                mblk = get_msgblks_L1(value_len);
            }
            reassemble_acl_data[idx].acl_data_ptr = mblk;

            mblk->primitive = HOST_MSG_RX_HCI_ACL_DATA;
            mblk->length    = value_len;

            p_acl_data                       = (host_rx_acl_data_param_t *) &mblk->para.hci_acl_data_evt;
            p_acl_data->conn_handle          = handle;
            p_acl_data_param.pDst            = mblk;
            p_acl_data_param.pSrc            = data;
            p_acl_data_param.received_length = reassemble_acl_data[idx].rcvd_data_length;
            p_acl_data_param.src_length      = data_len;
            p_acl_data_param.total_length    = reassemble_acl_data[idx].total_data_length + L2CAP_HEADER_SIZE;
            acl_data_get(&p_acl_data_param);
            reassemble_acl_data[idx].rcvd_data_length = data_len;
        }
        else
        {
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] invalid l2cap length %d %d\n", total_len, data_len);
        }
    }
    else if (pb_flag == CONTINUE_FRAME)
    {
        if (reassemble_acl_data[idx].acl_data_ptr != NULL)
        {
            if (reassemble_acl_data[idx].rcvd_data_length + data_len >
                (reassemble_acl_data[idx].total_data_length + L2CAP_HEADER_SIZE))
            {
                HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] drop invalid ACL data!!!, data_len %d totalLen %d, rxLen %d\n",
                            data_len, reassemble_acl_data[idx].total_data_length, reassemble_acl_data[idx].rcvd_data_length);

                msgblks_free(reassemble_acl_data[idx].acl_data_ptr);
                HCI_REASSEMBLE_BUFFER_INIT(idx);
            }
            else
            {
                get_acl_data_t p_acl_data_param;

                p_acl_data_param.pDst            = reassemble_acl_data[idx].acl_data_ptr;
                p_acl_data_param.pSrc            = data;
                p_acl_data_param.received_length = reassemble_acl_data[idx].rcvd_data_length;
                p_acl_data_param.src_length      = data_len;
                p_acl_data_param.total_length    = reassemble_acl_data[idx].total_data_length;
                acl_data_get(&p_acl_data_param);

                reassemble_acl_data[idx].rcvd_data_length += data_len;
            }
        }
        else
        {
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] get continue frame without start frame!!!\n");
        }
    }

    // if receive the whole L2CAP packet
    if (reassemble_acl_data[idx].rcvd_data_length == (reassemble_acl_data[idx].total_data_length + L2CAP_HEADER_SIZE))
    {
        configASSERT(&g_host_rx_handle != NULL);

        while (sys_sem_get_cnt(g_host_rx_data_sem) == 0)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] host data sem cnt0\n");
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HCI);
            sys_task_delay(TASK_DELAY_MS + 4);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HCI);
        }

        while (sys_queue_remaining_size(&g_host_rx_handle) < 6)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] host q not available\n");
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HCI);
            sys_task_delay(TASK_DELAY_MS + 4);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HCI);
        }

        if (sys_sem_wait(&g_host_rx_data_sem, TASK_DELAY_MS) == SYS_ARCH_TIMEOUT)
        {
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] host data sem get fail.\n");
        }
        while (sys_queue_send_with_timeout(&g_host_rx_handle, (void *) &reassemble_acl_data[idx].acl_data_ptr, TASK_DELAY_MS) ==
               ERR_TIMEOUT)
        {
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] HCI send data to host fail\n");
            // BREAK();
        }

        HCI_REASSEMBLE_BUFFER_INIT(idx);
    }
}

void hci_check_data_complete(uint8_t event_code, uint8_t * event_param_ptr)
{
    uint8_t complete_num = 0;

    if (event_code == HCI_EVENT_CODE_NUMBER_OF_COMPLETE_PACKETS) /*0x13: hci_number_of_complete_packets_event*/
    {
        ble_hci_evt_param_num_of_complete_packets_t * p_nocp_para = (ble_hci_evt_param_num_of_complete_packets_t *) event_param_ptr;
        complete_num                                              = p_nocp_para->num_cmplt_pkt;

        if (complete_num > 7)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] NOCP %d err\n", complete_num);
        }
    }

    while (complete_num)
    {
        notify_tx_task_tx_done(TX_TYPE_DATA);
        complete_num--;
    }

    if (sys_sem_get_cnt(g_tx_data_sem) > 7)
    {
        HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] free data q %d err\n", sys_sem_get_cnt(g_tx_data_sem));
    }
}

void hci_check_cmd_complete(uint8_t event_code, uint8_t * event_param_ptr)
{
    uint8_t complete_num = 0;

    if (event_code == HCI_EVENT_CODE_COMMAND_COMPLETE) /*0x0E: hci_command_complete_event*/
    {
        ble_hci_evt_param_cmd_complete_t * p_cmd_cmp = (ble_hci_evt_param_cmd_complete_t *) event_param_ptr;
        complete_num                                 = p_cmd_cmp->hci_num_cmd_pckts;
        if (complete_num > 1)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] complete num %d err\n", complete_num);
        }
    }
    else if (event_code == HCI_EVENT_CODE_COMMAND_STATUS) /*0x0F: hci_command_status_event*/
    {
        ble_hci_evt_param_cmd_status_t * p_cmd_status = (ble_hci_evt_param_cmd_status_t *) event_param_ptr;
        complete_num                                  = p_cmd_status->hci_num_cmd_pckts;
        if (complete_num > 1)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] status num %d err\n", complete_num);
        }
    }

    while (complete_num)
    {
        // sys_sem_signal(&g_tx_cmd_sem);
        notify_tx_task_tx_done(TX_TYPE_CMD);
        complete_num--;
    }

    if (sys_sem_get_cnt(g_tx_cmd_sem) > 1)
    {
        HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] free cmd q %d unexpected\n", sys_sem_get_cnt(g_tx_cmd_sem));
    }
}

void hci_rx_event(uint8_t * cmd_ptr, uint8_t cmd_length)
{
    ble_hci_message_struct_t * p_hci_message = (ble_hci_message_struct_t *) cmd_ptr;
    host_rx_event_param_t * p_event_param;
    MBLK * p_mblk;
    uint16_t data_len;

    event_cnt++;

    hci_check_data_complete(p_hci_message->msg_type.hci_event.event_code, p_hci_message->msg_type.hci_event.parameter);

    hci_check_cmd_complete(p_hci_message->msg_type.hci_event.event_code, p_hci_message->msg_type.hci_event.parameter);
    if (HCI_EVENT_FOR_HOST(p_hci_message->msg_type.hci_event.event_code) == TRUE)
    {
        data_len = p_hci_message->msg_type.hci_event.length + sizeof(host_rx_event_param_t);

        while (check_msgblk_L1_size(data_len + 251) == ERR_MEM)
        {
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] evt check mblk size fail %d\n", cmd_length);
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HCI);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HCI);
        }
        p_mblk = get_msgblks_L1(data_len);
        while (p_mblk == NULL)
        {
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] rx event, mblk malloc fail %d\n", cmd_length);
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HCI);
            sys_task_delay(TASK_DELAY_MS);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HCI);
            p_mblk = get_msgblks_L1(data_len);
        }

        p_mblk->primitive = HOST_MSG_RX_HCI_EVENT;
        p_mblk->length    = p_hci_message->msg_type.hci_event.length;

        p_event_param             = (host_rx_event_param_t *) p_mblk->para.Data;
        p_event_param->event_code = p_hci_message->msg_type.hci_event.event_code;
        memcpy(p_event_param->parameter, p_hci_message->msg_type.hci_event.parameter, p_hci_message->msg_type.hci_event.length);

        while (sys_sem_get_cnt(g_host_rx_cmd_sem) == 0)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] host cmd sem cnt0\n");
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HCI);
            sys_task_delay(TASK_DELAY_MS + 4);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HCI);
        }

        while (sys_queue_remaining_size(&g_host_rx_handle) < 6)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] host q not available\n");
            Lpm_Low_Power_Mask(LOW_POWER_MASK_BIT_TASK_HCI);
            sys_task_delay(TASK_DELAY_MS + 4);
            Lpm_Low_Power_Unmask(LOW_POWER_MASK_BIT_TASK_HCI);
        }

        if (sys_sem_wait(&g_host_rx_cmd_sem, TASK_DELAY_MS) == SYS_ARCH_TIMEOUT)
        {
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] host cmd sem get fail.\n");
        }
        while (sys_queue_send_with_timeout(&g_host_rx_handle, (void *) &p_mblk, TASK_DELAY_MS) == ERR_TIMEOUT)
        {
            HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] event to host fail\n");
        }
    }
}
#endif

#if (MODULE_ENABLE(SUPPORT_15P4))
void send_data_to_pci_fail(uint8_t reason)
{
    pci_task_common_queue_t msg;
    HPCI_PRINTF(PCI_DEBUG_INFO, "[PCI_DEBUG_INFO] send data to pci fail reason: 0x%02x\n", reason);

    // workaround cause mac_rt570_send_packet was not handle tx fail issue
    msg.pci_msg_tag                    = ISR_MSG_TX_DONE;
    msg.pci_msg.param_type.pci_msg_var = reason;

    if (sys_queue_trysend(&g_rx_common_queue_handle, (void *) &msg) != ERR_OK)
    {
        HPCI_PRINTF(PCI_DEBUG_INFO, "[PCI_DEBUG_INFO] send pci common q fail\n");
    }
}

void pci_rx_event(uint8_t * p_event, uint8_t event_length)
{
    rfb_event_msg_t rfb_event_msg;
    ruci_para_cnf_event_t * p_pci_cnf_event = (ruci_para_cnf_event_t *) p_event;

    if (PCI_CMD_COMPLETE_EVENT(p_pci_cnf_event))
    {
        notify_tx_task_tx_done(TX_TYPE_CMD);
    }
    else
    {
        HPCI_PRINTF(PCI_DEBUG_INFO, "[PCI_DEBUG_CMD_EVT] pci get event 0x%02x 0x%02x\n", p_pci_cnf_event->ruci_header,
                    p_pci_cnf_event->sub_header);
    }

    rfb_event_msg.length = event_length;
    rfb_event_msg.p_data = mem_malloc(rfb_event_msg.length);

    if (rfb_event_msg.p_data == NULL)
    {
        HPCI_PRINTF(PCI_DEBUG_INFO, "[PCI_DEBUG_INFO] pci event malloc fail\n");
    }
    else
    {
        memcpy(rfb_event_msg.p_data, (uint8_t *) p_pci_cnf_event, rfb_event_msg.length);
        if (sys_queue_send_with_timeout(&g_rfb_event_queue_handle, &rfb_event_msg, TASK_DELAY_MS))
        {
            HPCI_PRINTF(PCI_DEBUG_INFO, "[PCI_DEBUG_INFO] pci send event fail\n");
        }
    }
}

void pci_rx_data(uint8_t * rx_data_ptr, uint8_t * ctrl_field_ptr)
{
    extern rfb_interrupt_event_t * rfb_interrupt_event;
    zb_rx_ctrl_field_t ctrl_field;

    memcpy((uint8_t *) &ctrl_field, ctrl_field_ptr, 5 /*RUCI_LEN_RX_CONTROL_FIELD - 2*/);
    rfb_interrupt_event->rx_done(ctrl_field.Length, rx_data_ptr, ctrl_field.CrcStatus, ctrl_field.Rssi, ctrl_field.Snr);
}
#endif

void check_rx_task_queue(void)
{
    rf_fw_rx_ctrl_msg_t rf_fw_comm_msg;
    uint32_t msg_waiting_time;
    uint8_t transport_id;

    msg_waiting_time = sys_queue_recv(&g_rx_common_queue_handle, &rf_fw_comm_msg, 0);

    if (msg_waiting_time != SYS_ARCH_TIMEOUT)
    {
        switch (rf_fw_comm_msg.msg_tag)
        {

        case ISR_MSG_EVENT_GET_FROM_AHB: {
            RF_MCU_RX_CMDQ_ERROR rx_cmd;
            uint16_t cmd_length;
            uint8_t
                read_cmd_array[268]; /*HCI_ACL_DATA_MAX_LENGTH + PHY_STATUS + CRC + HCI_PKT_IND + HANDLE_PB_PC + DATA_TOTAL_LEN*/

            /* 1 rx cmd queue */
            cmd_length = RfMcu_EvtQueueRead((uint8_t *) read_cmd_array, &rx_cmd);
            if (rx_cmd == RF_MCU_RX_CMDQ_GET_SUCCESS)
            {
                transport_id = read_cmd_array[0];

#if (MODULE_ENABLE(SUPPORT_BLE))
                if (HCI_EVENT(transport_id))
                {
                    hci_rx_event(read_cmd_array, cmd_length);
                }
                else
#endif
#if (MODULE_ENABLE(SUPPORT_15P4))
                    if (PCI_EVENT(transport_id))
                {
                    pci_rx_event(read_cmd_array, cmd_length);
                }
                else
#endif
                {
                    HPCI_PRINTF(COMMON_DEBUG_ERR, "[COMMON_DEBUG_ERR] unknow event 0x%02x\n", transport_id);
                }
            }
        }
        break;

#if (MODULE_ENABLE(SUPPORT_15P4))
        case ISR_MSG_TX_DONE: {
            extern rfb_interrupt_event_t * rfb_interrupt_event;

            mib_counter_increase(RfFwCtrlTxDoneCount);

            rfb_interrupt_event->tx_done(rf_fw_comm_msg.param_var);

            if ((rf_fw_comm_msg.param_var != TX_SW_QUEUE_FULL) && (rf_fw_comm_msg.param_var != TX_MALLOC_FAIL))
            {
                // sys_sem_signal(&g_tx_data_sem);
                notify_tx_task_tx_done(TX_TYPE_DATA);
            }
        }
        break;
#endif

        case ISR_MSG_DATA_GET_FROM_AHB: {
            RF_MCU_RXQ_ERROR rx_q;
            uint8_t
                read_data_array[268]; /*HCI_ACL_DATA_MAX_LENGTH + PHY_STATUS + CRC + HCI_PKT_IND + HANDLE_PB_PC + DATA_TOTAL_LEN*/

            /* 4 rx data queue */
            /*cause rx data ISR may received twice when RT58x is sleeping*/
            while (RfMcu_RxQueueRead((uint8_t *) read_data_array, &rx_q) != 0)
            {
                transport_id = read_data_array[0];
                // SYS_PRINTF(DEBUG_INFO, "[DEBUG_INFO] transport_id %d\n", transport_id);
#if (MODULE_ENABLE(SUPPORT_BLE))
                if (transport_id == BLE_TRANSPORT_HCI_ACL_DATA)
                {
                    hci_reassemble_acl_data(read_data_array);
                }
                else
#endif
#if (MODULE_ENABLE(SUPPORT_15P4))
                    if (transport_id == RUCI_PCI_DATA_HEADER)
                {
                    pci_rx_data(read_data_array, &read_data_array[2]);
                }
                else
#endif
                {
                    HPCI_PRINTF(COMMON_DEBUG_ERR, "[COMMON_DEBUG_ERR] invalid transport_id %d\n", transport_id);
                }

                if (sys_queue_remaining_size(&g_rx_common_queue_handle) <= MAX_RESERVED_EVENT_NUM)
                {
                    break;
                }
            }
        }
        break;

        case ISR_MSG_TRAP: {
            uint32_t debug_value = 0;

            RfMcu_MemoryGet(0x4008, (uint8_t *) &debug_value, 4);
            // info("[COMMON_DEBUG_INFO] TRAP 0x%08x\n", debug_value);

            info("[Error] RFB Trap !!!\n");
            RfMcu_MemoryGet(0x4008, (uint8_t *) &debug_value, 4);
            info("PC= %X\n", debug_value);
            RfMcu_MemoryGet(0x01E0, (uint8_t *) &debug_value, 4);
            info("MAC err status= %X\n", debug_value);
            RfMcu_MemoryGet(0x0198, (uint8_t *) &debug_value, 4);
            info("MAC task status= %X\n", debug_value);
            RfMcu_MemoryGet(0x0048, (uint8_t *) &debug_value, 4);
            info("BMU err status= %X\n", debug_value);
        }
        break;

        default:
            break;
        }
    }
}

void task_rx_handle(void)
{
#if (MODULE_ENABLE(SUPPORT_BLE))
    hci_reassemble_acl_data_init();
#endif
    NVIC_SetPriority(CommSubsystem_IRQn, RF_ISR_PRIORITY);

#if (MODULE_ENABLE(SUPPORT_BLE)) && (MODULE_ENABLE(SUPPORT_15P4))
    rf_common_init_by_fw(RF_FW_LOAD_SELECT_MULTI_PROTCOL_2P4G, rf_fw_isr_hander);
#elif (MODULE_ENABLE(SUPPORT_BLE))
    rf_common_init_by_fw(RF_FW_LOAD_SELECT_BLE_CONTROLLER, rf_fw_isr_hander);
#elif (MODULE_ENABLE(SUPPORT_15P4))
    rf_common_init_by_fw(RF_FW_LOAD_SELECT_RUCI_CMD, rf_fw_isr_hander);
#endif

    for (;;)
    {
        check_rx_task_queue();
    }
}

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

void task_rx_delete(void)
{
#if (MODULE_ENABLE(SUPPORT_BLE))
    /*-----------------------------------*/
    /* A.Input Parameter Range Check     */
    /*-----------------------------------*/

    /*-----------------------------------*/
    /* B. Main Functionality             */
    /*-----------------------------------*/
    do
    {
        sys_queue_free(&g_rx_common_queue_handle);
        vTaskDelete(g_phci_rx_task);
    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
#endif
}

void task_rx_init(void)
{
#if (MODULE_ENABLE(SUPPORT_BLE)) && (MODULE_ENABLE(SUPPORT_15P4))
    static char * task_name = { "TASK_DUAL_RX" };
#elif (MODULE_ENABLE(SUPPORT_BLE))
    static char * task_name = { "HCI_RX" };
#elif (MODULE_ENABLE(SUPPORT_15P4))
    static char * task_name = { "TASK_PCI_RX" };
#endif

    HPCI_PRINTF(COMMON_INFO, "[COMMON_INFO] %s init\n", task_name);

    sys_queue_new(&g_rx_common_queue_handle, NUM_QUEUE_RX_COMMON, sizeof(rf_fw_rx_ctrl_msg_t));

    g_phci_rx_task =
        sys_task_new(task_name, (TaskFunction_t) task_rx_handle, NULL, CONFIG_RX_TASK_STACK_SIZE, CONFIG_RX_TASK_PRIORITY);
    if (g_phci_rx_task == NULL)
    {
        HPCI_PRINTF(COMMON_DEBUG_ERR, "[COMMON_DEBUG_ERR] hci create fail\n");
    }
}

/** @file task_hci.c
 *
 * @brief Handle BLE application request, BLE event and BLE service data.
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "rf_fw_ctrl_tx.h"
#include "hci_pci_printf.h"
#include "mem_mgmt.h"
#include "project_config.h"
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
/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/

#define TASK_DELAY_MS (2) /* task delay time (ms) */

#define CONFIG_TX_TASK_PRIORITY TASK_PRIORITY_PROTOCOL_HIGH /**< Task priority. */
#define CONFIG_TX_TASK_STACK_SIZE (256)

#define NUM_HW_RX_DATA_Q (4)  /**< Mapping to RX data queue number of HW. */
#define NUM_HW_RX_EVENT_Q (1) /**< Mapping to RX event queue number of HW. */

#define NUM_QUEUE_RFCI_COMMON ((NUM_HW_RX_DATA_Q + NUM_HW_RX_EVENT_Q) << 1)

#define RF_ISR_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1)

#define NUM_TX_DATA_Q (7)
#define NUM_TX_CMD_Q (1)

#define HCI_INTERNAL_MSG_TX_DONE (0x00) /**< Message for isr to notify RX event. */

/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
sys_queue_t g_rfc_common_queue_handle;
sys_queue_t g_hci_common_handle;
sys_queue_t g_pci_common_handle;
sys_queue_t g_hci_tx_acl_handle;
sys_queue_t g_hci_tx_cmd_handle;
sys_queue_t g_pci_tx_data_handle;
sys_queue_t g_pci_tx_cmd_handle;

sys_sem_t g_tx_cmd_sem;
sys_sem_t g_tx_data_sem;

sys_task_t g_phci_tx_task;
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

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/

#if (MODULE_ENABLE(SUPPORT_BLE))
uint8_t check_hci_acl_data_queue(void);
uint8_t check_hci_cmd_queue(void);
uint8_t acl_txdata_memory[251 + 6];
#endif

#if (MODULE_ENABLE(SUPPORT_15P4))
uint8_t check_pci_data_queue(void);
uint8_t check_pci_cmd_queue(void);
#endif

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

#if (MODULE_ENABLE(SUPPORT_BLE)) && (MODULE_ENABLE(SUPPORT_15P4))
uint8_t (*const check_data_queue[])(void) = {
    check_hci_acl_data_queue,
    check_pci_data_queue,
};

uint8_t (*const check_cmd_queue[])(void) = {
    check_hci_cmd_queue,
    check_pci_cmd_queue,
};
#elif (MODULE_ENABLE(SUPPORT_BLE))
uint8_t (*const check_data_queue[])(void) = {
    check_hci_acl_data_queue,
    check_hci_acl_data_queue,
};

uint8_t (*const check_cmd_queue[])(void) = {
    check_hci_cmd_queue,
    check_hci_cmd_queue,
};
#elif (MODULE_ENABLE(SUPPORT_15P4))
uint8_t (*const check_data_queue[])(void) = {
    check_pci_data_queue,
    check_pci_data_queue,
};

uint8_t (*const check_cmd_queue[])(void) = {
    check_pci_cmd_queue,
    check_pci_cmd_queue,
};
#endif

uint8_t write_tx_data(uint8_t * tx_data_ptr, uint16_t tx_data_len)
{
    extern void RfMcu_IoSet(uint8_t Q_id, const uint8_t * p_tx_data, uint16_t tx_data_length);
    uint32_t reg_val;
    uint8_t txq_id;

    /* Check empty TXQ */
    reg_val = (COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_TX_INFO & 0xFF);

    if ((reg_val & 0x7F) == 0)
    {
        return FALSE;
    }

    for (txq_id = 0; txq_id < 7; txq_id++)
    {
        if (reg_val & (1 << txq_id))
        {
            break;
        }
    }

    /* Send data to TXQ */
    RfMcu_IoSet(txq_id, tx_data_ptr, tx_data_len);

    return TRUE;
}

#if (MODULE_ENABLE(SUPPORT_BLE))

uint8_t check_hci_acl_data_queue(void)
{
    ble_hci_message_struct_t * p_hci_message;
    uint16_t hci_data_length;
    uint8_t send                  = FALSE;
    static uint8_t ble_comm_tx_sn = 0;

    if (sys_arch_queue_tryrecv(&g_hci_tx_acl_handle, &p_hci_message) != SYS_ARCH_TIMEOUT)
    {
        if (p_hci_message->msg_type.hci_tx_acl_data.transport_id == BLE_TRANSPORT_HCI_ACL_DATA)
        {
            ((ble_hci_tx_acl_data_hdr_t *) &acl_txdata_memory[0])->transport_id =
                p_hci_message->msg_type.hci_tx_acl_data.transport_id;
            ((ble_hci_tx_acl_data_hdr_t *) &acl_txdata_memory[0])->handle   = p_hci_message->msg_type.hci_tx_acl_data.handle;
            ((ble_hci_tx_acl_data_hdr_t *) &acl_txdata_memory[0])->pb_flag  = p_hci_message->msg_type.hci_tx_acl_data.pb_flag;
            ((ble_hci_tx_acl_data_hdr_t *) &acl_txdata_memory[0])->bc_flag  = p_hci_message->msg_type.hci_tx_acl_data.bc_flag;
            ((ble_hci_tx_acl_data_hdr_t *) &acl_txdata_memory[0])->length   = p_hci_message->msg_type.hci_tx_acl_data.length;
            ((ble_hci_tx_acl_data_hdr_t *) &acl_txdata_memory[0])->sequence = ble_comm_tx_sn;
            get_acl_data_from_msgblks(&acl_txdata_memory[6], (MBLK *) p_hci_message->msg_type.hci_tx_acl_data.p_data);

            hci_data_length =
                p_hci_message->msg_type.hci_tx_acl_data.length + 1 /*transport*/ + 1 /* sequence */ + 2 /*handle*/ + 2 /*length*/;

            if (write_tx_data((uint8_t *) &acl_txdata_memory[0], hci_data_length) != TRUE)
            {
                HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] tx data fail\n");
            }
            else
            {
                // ACL data TX
                if ((HPCI_FORCE_PRINT & HCI_DEBUG_DATA) != 0)
                {
                    uint16_t i;

                    HPCI_PRINTF(HCI_DEBUG_DATA, "[HCI_TX_D] handle 0x%04x, len:0x%02x\n",
                                p_hci_message->msg_type.hci_tx_acl_data.handle, p_hci_message->msg_type.hci_tx_acl_data.length);
                    HPCI_PRINTF(HCI_DEBUG_DATA, "[HCI_TX_D] ACL data TX =");
                    for (i = 0; i < p_hci_message->msg_type.hci_tx_acl_data.length; i++)
                    {
                        HPCI_PRINTF(HCI_DEBUG_DATA, " %02x", acl_txdata_memory[6 + i]);
                    }
                    HPCI_PRINTF(HCI_DEBUG_DATA, "\n");
                }
                ble_comm_tx_sn++;
                send = TRUE;
            }
        }
        else
        {
            // incorrect type
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] tx acl data q rcvd incorrect transport_id %d\n",
                        p_hci_message->msg_type.hci_tx_acl_data.transport_id);
        }
        msgblks_free((MBLK *) p_hci_message->msg_type.hci_tx_acl_data.p_data);
        mem_free(p_hci_message);
    }

    return send;
}

uint8_t check_hci_cmd_queue(void)
{
    ble_hci_message_struct_t * p_hci_message;
    uint8_t transport_id, hci_command_length;
    uint8_t cmd_send = FALSE;

    if (sys_arch_queue_tryrecv(&g_hci_tx_cmd_handle, &p_hci_message) != SYS_ARCH_TIMEOUT)
    {
        transport_id = p_hci_message->msg_type.ble_hci_array[0];

        if (transport_id == BLE_TRANSPORT_HCI_COMMAND)
        {
            hci_command_length = p_hci_message->msg_type.hci_command.length + 1 /*transport*/ + 2 /*opcode*/ + 1 /*length*/;

            HPCI_PRINTF(HCI_TASK, "[HCI_TASK] hci cmd ocf %d ogf %d\n", p_hci_message->msg_type.hci_command.ocf,
                        p_hci_message->msg_type.hci_command.ogf);
            if (RfMcu_CmdQueueSend((uint8_t *) p_hci_message, hci_command_length) != RF_MCU_TX_CMDQ_SET_SUCCESS)
            {
                HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] tx cmd fail\n");
            }
            else
            {
                cmd_send = TRUE;
            }
        }
        else
        {
            // incorrect type
            HPCI_PRINTF(HCI_DEBUG_ERR, "[HCI_DEBUG_ERR] tx cmd q rcvd incorrect transport_id %d\n", transport_id);
        }
        mem_free(p_hci_message);
    }
    return cmd_send;
}

void hci_clear_tx_data_queue(uint16_t target_handle)
{
    ble_hci_message_struct_t * hci_msg_ptr;
    uint32_t data_num;

    data_num = (QUEUE_HCI_ACL_DATA_TX - sys_queue_remaining_size(&g_hci_tx_acl_handle));

    HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] num of occupy queue %d\n", data_num);
    while (data_num)
    {
        if (sys_arch_queue_tryrecv(&g_hci_tx_acl_handle, &hci_msg_ptr) != SYS_ARCH_TIMEOUT)
        {
            if (hci_msg_ptr->msg_type.hci_tx_acl_data.handle == target_handle)
            {
                HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] clear tx data handle 0x%04x\n",
                            hci_msg_ptr->msg_type.hci_tx_acl_data.handle);
                msgblks_free((MBLK *) hci_msg_ptr->msg_type.hci_tx_acl_data.p_data);
                mem_free(hci_msg_ptr);
            }
            else
            {
                sys_queue_send(&g_hci_tx_acl_handle, &hci_msg_ptr);
            }
        }
        data_num--;
    }
}
#endif

#if (MODULE_ENABLE(SUPPORT_15P4))

uint8_t check_pci_data_queue(void)
{
    pci_message_struct_t * p_pci_message;
    uint16_t hci_data_length;
    uint8_t send = FALSE;

    if (sys_arch_queue_tryrecv(&g_pci_tx_data_handle, &p_pci_message) != SYS_ARCH_TIMEOUT)
    {

        if (p_pci_message->msg_type.pci_tx_data_hdr.ruci_header == RUCI_PCI_DATA_HEADER)
        {
            hci_data_length = p_pci_message->msg_type.pci_tx_data_hdr.length + sizeof(pci_15p4_data_hdr_t);

            if (write_tx_data((uint8_t *) p_pci_message, hci_data_length) != TRUE)
            {
                HPCI_PRINTF(PCI_DEBUG_ERR, "[PCI_DEBUG_ERR] tx data fail\n");
            }
            else
            {
                send = TRUE;
            }
        }
        else
        {
            // incorrect type
            HPCI_PRINTF(PCI_DEBUG_ERR, "[PCI_DEBUG_ERR] tx acl data q rcvd incorrect transport_id %d\n",
                        p_pci_message->msg_type.pci_tx_data_hdr.ruci_header);
        }

        mem_free(p_pci_message);
    }

    return send;
}

uint8_t check_pci_cmd_queue(void)
{
    pci_message_struct_t * p_pci_message;
    uint8_t pci_command_length;
    uint8_t cmd_send = FALSE;

    if (sys_arch_queue_tryrecv(&g_pci_tx_cmd_handle, &p_pci_message) != SYS_ARCH_TIMEOUT)
    {
#if 0
        if ((p_pci_message->msg_type.pci_tx_hdr.ruci_header == RUCI_PCI15P4_MAC_CMD_HEADER) ||
                (p_pci_message->msg_type.pci_tx_hdr.ruci_header == RUCI_PCI_COMMON_CMD_HEADER) ||
                (p_pci_message->msg_type.pci_tx_hdr.ruci_header == RUCI_CMN_SYS_CMD_HEADER))
#endif
        {
            pci_command_length = p_pci_message->msg_type.pci_tx_hdr.length + sizeof(pci_15p4_cmd_hdr_t);

            RfMcu_HostCmdSet(RF_MCU_STATE_TX_FAIL); // test code for rfb cmd send fail
            if (RfMcu_CmdQueueSend(p_pci_message->msg_type.pci_array, pci_command_length) != RF_MCU_TX_CMDQ_SET_SUCCESS)
            {
                rfb_event_msg_t rfb_event_msg;
                info("[PCI_DEBUG_INFO] tx cmd fail\n");

                rfb_event_msg.length = 0;
                if (sys_queue_send_with_timeout(&g_rfb_event_queue_handle, &rfb_event_msg, TASK_DELAY_MS))
                {
                    HPCI_PRINTF(PCI_DEBUG_INFO, "[PCI_DEBUG_INFO] pci send cmd fail\n");
                }
            }
            else
            {
                cmd_send = TRUE;
            }
        }

        mem_free(p_pci_message);
    }
    return cmd_send;
}

#endif

void check_task_common_queue(void)
{
    rf_fw_ctrl_msg_t rf_fw_comm_msg;
    uint32_t msg_waiting_time;

    msg_waiting_time = sys_queue_recv(&g_rfc_common_queue_handle, &rf_fw_comm_msg, 0);

    if (msg_waiting_time != SYS_ARCH_TIMEOUT)
    {
        switch (rf_fw_comm_msg.msg_tag)
        {
        case INTERNAL_MSG_TX_DONE:
            break;
#if (MODULE_ENABLE(SUPPORT_BLE))
        case HCI_MSG_HOST_HCI_ACL_DATA_CLEAR: {
            hci_clear_tx_data_queue(rf_fw_comm_msg.param_type.hci_msg_ptr->msg_type.hci_data_clear.conn_handle);
            mem_free(rf_fw_comm_msg.param_type.hci_msg_ptr);
        }
        break;

        case HCI_MSG_HOST_HCI_CMD: {
            /*forward cmd to cmd queue*/
            if (sys_queue_send_with_timeout(&g_hci_tx_cmd_handle, &rf_fw_comm_msg.param_type.hci_msg_ptr, TASK_DELAY_MS))
            {
                HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] hci send cmd fail\n");
                mem_free(rf_fw_comm_msg.param_type.hci_msg_ptr);
            }
        }
        break;

        case HCI_MSG_HOST_HCI_ACL_DATA: {
            /*forward data to data queue*/
            if (sys_queue_send_with_timeout(&g_hci_tx_acl_handle, &rf_fw_comm_msg.param_type.hci_msg_ptr, TASK_DELAY_MS))
            {
                HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] hci send data fail\n");
                mem_free(rf_fw_comm_msg.param_type.hci_msg_ptr);
            }
        }
        break;

        case HCI_MSG_HOST_NOCP_EVENT: {
            uint8_t complete_num;

            complete_num = rf_fw_comm_msg.param_type.hci_msg_ptr->msg_type.hci_data.complete_num;
            if (complete_num > 7)
            {
                HPCI_PRINTF(HCI_DEBUG_INFO, "[HCI_DEBUG_INFO] invalid complete_num %d\n", complete_num);
            }

            mem_free(rf_fw_comm_msg.param_type.hci_msg_ptr);
        }
        break;
#endif

#if (MODULE_ENABLE(SUPPORT_15P4))
        case PCI_MSG_TX_PCI_CMD: {
            /*forward cmd to cmd queue*/
            if (sys_queue_send_with_timeout(&g_pci_tx_cmd_handle, &rf_fw_comm_msg.param_type.pci_msg.param_type.p_pci_msg,
                                            TASK_DELAY_MS))
            {
                HPCI_PRINTF(PCI_DEBUG_INFO, "[PCI_DEBUG_INFO] pci send cmd fail\n");
                mem_free(rf_fw_comm_msg.param_type.pci_msg.param_type.p_pci_msg);
            }
        }
        break;

        case PCI_MSG_TX_PCI_DATA: {
            /*forward data to data queue*/
            if (sys_queue_send_with_timeout(&g_pci_tx_data_handle, &rf_fw_comm_msg.param_type.pci_msg.param_type.p_pci_msg,
                                            TASK_DELAY_MS))
            {
                HPCI_PRINTF(PCI_DEBUG_INFO, "[PCI_DEBUG_INFO] pci send data fail\n");
                mem_free(rf_fw_comm_msg.param_type.pci_msg.param_type.p_pci_msg);
            }
        }
        break;
#endif

        default:
            break;
        }
    }
}

void check_task_cmd_queue(void)
{
    while (sys_sem_get_cnt(g_tx_cmd_sem) > 0)
    {
        static uint8_t idx = 0;

        idx ^= 1;

        if (check_cmd_queue[idx]() == TRUE)
        {
            if (sys_sem_wait(&g_tx_cmd_sem, 1) == SYS_ARCH_TIMEOUT)
            {
                HPCI_PRINTF(COMMON_DEBUG_INFO, "[COMMON_DEBUG_INFO] cmd sem get fail, cnt %d\n", sys_sem_get_cnt(g_tx_cmd_sem));
            }
        }
#if (MODULE_ENABLE(SUPPORT_BLE)) && (MODULE_ENABLE(SUPPORT_15P4))
        else if (check_cmd_queue[idx ^ 1]() == TRUE)
        {
            if (sys_sem_wait(&g_tx_cmd_sem, 1) == SYS_ARCH_TIMEOUT)
            {
                HPCI_PRINTF(COMMON_DEBUG_INFO, "[COMMON_DEBUG_INFO] cmd sem get fail, cnt %d\n", sys_sem_get_cnt(g_tx_cmd_sem));
            }
        }
#endif
        else
        {
            // no message found in queue
            break;
        }
    }
}

void check_task_data_queue(void)
{
    uint8_t sem_cnt = sys_sem_get_cnt(g_tx_data_sem);

    while (sem_cnt > 0)
    {
        static uint8_t idx = 0;

        idx ^= 1;
        if (check_data_queue[idx]() == TRUE)
        {
            if (sys_sem_wait(&g_tx_data_sem, 1) == SYS_ARCH_TIMEOUT)
            {
                HPCI_PRINTF(COMMON_DEBUG_INFO, "[COMMON_DEBUG_INFO] data sem get fail, cnt %d\n", sys_sem_get_cnt(g_tx_data_sem));
            }
        }
#if (MODULE_ENABLE(SUPPORT_BLE)) && (MODULE_ENABLE(SUPPORT_15P4))
        else if (check_data_queue[idx ^ 1]() == TRUE)
        {
            if (sys_sem_wait(&g_tx_data_sem, 1) == SYS_ARCH_TIMEOUT)
            {
                HPCI_PRINTF(COMMON_DEBUG_INFO, "[COMMON_DEBUG_INFO] data sem get fail, cnt %d\n", sys_sem_get_cnt(g_tx_data_sem));
            }
        }
#endif
        else
        {
            // no message found in queue
            break;
        }
        sem_cnt--;
    }
}

void check_tx_task_queue(void)
{
    check_task_common_queue();
    check_task_data_queue();
    check_task_cmd_queue();
}

void task_tx_handle(void)
{
    for (;;)
    {
        check_tx_task_queue();
    }
}

void task_tx_delete(void)
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
        sys_queue_free(&g_rfc_common_queue_handle);
        sys_queue_free(&g_hci_tx_acl_handle);
        sys_queue_free(&g_hci_tx_cmd_handle);

        sys_queue_free(&g_tx_cmd_sem);
        sys_queue_free(&g_tx_data_sem);

        vTaskDelete(g_phci_tx_task);
    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
#endif
}

void task_tx_init(void)
{
#if (MODULE_ENABLE(SUPPORT_BLE)) && (MODULE_ENABLE(SUPPORT_15P4))
    static char * task_name = { "TASK_DUAL_TX" };
#elif (MODULE_ENABLE(SUPPORT_BLE))
    static char * task_name = { "HCI_TX" };
#elif (MODULE_ENABLE(SUPPORT_15P4))
    static char * task_name = { "TASK_PCI_TX" };
#endif

    HPCI_PRINTF(COMMON_INFO, "[COMMON_INFO] %s init\n", task_name);

    sys_queue_new(&g_rfc_common_queue_handle, NUM_QUEUE_RFCI_COMMON, sizeof(rf_fw_ctrl_msg_t));

#if (MODULE_ENABLE(SUPPORT_BLE))
    g_hci_common_handle = g_rfc_common_queue_handle;
    sys_queue_new(&g_hci_tx_acl_handle, QUEUE_HCI_ACL_DATA_TX, sizeof(ble_hci_message_struct_t *));
    sys_queue_new(&g_hci_tx_cmd_handle, QUEUE_HCI_COMMAND_EVENT, sizeof(ble_hci_message_struct_t *));
#endif

#if (MODULE_ENABLE(SUPPORT_15P4))
    g_pci_common_handle = g_rfc_common_queue_handle;
    sys_queue_new(&g_pci_tx_data_handle, NUM_QUEUE_PCI_DATA, sizeof(pci_message_struct_t *));
    sys_queue_new(&g_pci_tx_cmd_handle, NUM_QUEUE_PCI_CMD, sizeof(pci_message_struct_t *));
#endif
    sys_sem_new(&g_tx_cmd_sem, NUM_TX_CMD_Q, NUM_TX_CMD_Q);
    sys_sem_new(&g_tx_data_sem, NUM_TX_DATA_Q, NUM_TX_DATA_Q);

    g_phci_tx_task =
        sys_task_new(task_name, (TaskFunction_t) task_tx_handle, NULL, CONFIG_TX_TASK_STACK_SIZE, CONFIG_TX_TASK_PRIORITY);
    if (g_phci_tx_task == NULL)
    {
        HPCI_PRINTF(COMMON_DEBUG_ERR, "[COMMON_DEBUG_ERR] hci create fail\n");
    }
}

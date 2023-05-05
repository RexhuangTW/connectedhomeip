/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "ble_api.h"
#include "ble_printf.h"
#include "sys_arch.h"
#include "task_ble_app.h"
#include "task_hci.h"
#include "task_host.h"
#include <stdio.h>

/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/
#define APP_ALIGN_4_BYTES(a) (((uint32_t) (a) + 0x3) & ~0x3)

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
static sys_tlv_t * g_app_ptlv = NULL;
static sys_sem_t g_app_sem;
static pf_evt_indication * pf_app_indication;

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

int ble_app_notify(sys_tlv_t * pt_tlv)
{
    int sys_rtn = BLE_ERR_OK;

    if (pt_tlv != NULL)
    {
        uint32_t u32_total_len;
        u32_total_len = pt_tlv->length + sizeof(ble_tlv_t);

        while (1)
        {
            uint32_t u32_timeout = 0;
            g_app_ptlv           = pt_tlv;

            if (!pf_app_indication)
            {
                BLE_PRINTF(BLE_DEBUG_ERR, "%s APP notification callback not installed\n", sys_current_thread_name());
                sys_rtn = BLE_ERR_NOT_INIT;
                break;
            }
            pf_app_indication(u32_total_len);

            // wait APP receive tlv packet
            u32_timeout = sys_sem_wait(&g_app_sem, 5);
            if (u32_timeout == SYS_ARCH_TIMEOUT)
            {
                BLE_PRINTF(BLE_DEBUG_INFO, "%s wait app receive pkt timeout\n", sys_current_thread_name());
                sys_rtn = BLE_BUSY;
                break;
            }
            else
            {
                sys_rtn = BLE_ERR_OK;
                break;
            }
        }
    }
    else
    {
        sys_rtn = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return sys_rtn;
}

int ble_event_msg_sendto(ble_tlv_t * pt_tlv)
{
    sys_tlv_t * pt_new_tlv = NULL;
    sys_tlv_t * pt_cfm_tlv = NULL;
    uint32_t u32_new_size;
    int sys_rtn = BLE_ERR_OK;

    do
    {
        /*-----------------------------------*/
        /* A.Input Parameter Range Check     */
        /*-----------------------------------*/
        if (pt_tlv == NULL)
        {
            sys_rtn = BLE_ERR_SENDTO_POINTER_NULL;
            break;
        }

        /*-----------------------------------*/
        /* B. Main Functionality             */
        /*-----------------------------------*/
        u32_new_size = APP_ALIGN_4_BYTES(pt_tlv->length) + sizeof(sys_tlv_t);

        pt_new_tlv = (sys_tlv_t *) mem_malloc(u32_new_size);
        if (pt_new_tlv == NULL)
        {
            sys_rtn = BLE_ERR_DATA_MALLOC_FAIL;
            break;
        }
        memcpy(pt_new_tlv, pt_tlv, u32_new_size);

        sys_rtn = ble_queue_sendto(pt_new_tlv);

        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "Sento status %d...", sys_rtn);
        if (sys_rtn == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "Wait cfm");
            if (ble_wait_cfm(&pt_cfm_tlv, 0) != SYS_ARCH_TIMEOUT)
            {
                sys_rtn = (int) *pt_cfm_tlv->value;
                BLE_PRINTF(BLE_DEBUG_CMD_INFO, "\nRecv cfm status %d\n", (ble_err_t) sys_rtn);
                mem_free(pt_cfm_tlv);
            }
            else
            {
                BLE_PRINTF(BLE_DEBUG_ERR, "Timeout!\n");
                sys_rtn = BLE_BUSY;
            }
        }

    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    return sys_rtn;
}

int ble_event_msg_recvfrom(uint8_t * pu8_buf, uint32_t * pu32_buf_len)
{
    uint32_t u32_tlv_length;
    do
    {
        /*-----------------------------------*/
        /* A.Input Parameter Range Check     */
        /*-----------------------------------*/
        if (pu8_buf == NULL)
        {
            return BLE_ERR_SENDTO_POINTER_NULL;
        }
        if (g_app_ptlv == NULL)
        {
            return BLE_ERR_RECVFROM_NO_DATA;
        }
        u32_tlv_length = g_app_ptlv->length + sizeof(ble_tlv_t);
        if (*pu32_buf_len < u32_tlv_length)
        {
            return BLE_ERR_RECVFROM_LEN_NOT_ENOUGH;
        }

        /*-----------------------------------*/
        /* B. Main Functionality             */
        /*-----------------------------------*/
        *pu32_buf_len = u32_tlv_length;
        memcpy(pu8_buf, g_app_ptlv, u32_tlv_length);
        g_app_ptlv = NULL;
        sys_sem_signal(&g_app_sem);
    } while (0);
    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    return 0;
}

int ble_host_stack_deinit(void)
{
    int i32_ret = 0;

    do
    {
        /*-----------------------------------*/
        /* A.Input Parameter Range Check     */
        /*-----------------------------------*/

        /*-----------------------------------*/
        /* B. Main Functionality             */
        /*-----------------------------------*/
        sys_sem_free(&g_app_sem);

        /* Initial protocol stack tasks here */
        ble_delete_host_subsystem();
        task_delete_ble_app();

    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    return i32_ret;
}

int ble_host_stack_init(ble_cfg_t * pt_cfg)
{
    int i32_ret = 0;

    do
    {
        /*-----------------------------------*/
        /* A.Input Parameter Range Check     */
        /*-----------------------------------*/
        if (pt_cfg == NULL)
        {
            i32_ret = BLE_ERR_SENDTO_POINTER_NULL;
            break;
        }

        /*-----------------------------------*/
        /* B. Main Functionality             */
        /*-----------------------------------*/
        pf_app_indication = pt_cfg->pf_evt_indication;
        sys_binary_sem_new(&g_app_sem);

        /* Initial protocol stack tasks here */
        ble_host_subsystem_init();
        task_ble_app_init();

    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    return i32_ret;
}

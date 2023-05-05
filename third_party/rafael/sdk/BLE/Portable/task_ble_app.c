/** @file task_ble_app.c
 *
 * @brief Handle BLE application request, BLE event and BLE service data.
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "task_ble_app.h"
#include "ble_att_gatt.h"
#include "ble_event_app.h"
#include "lib_config.h"
#include "task_host.h"
#include <stdint.h>

#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
#include "ble_profile.h"
#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define CONFIG_APP_QUEUE_SIZE 10
#define CONFIG_QUEUE_SEND_TIMEOUT_MS 50 /**< Timeout in millisecond for queue sending. */

#define NOTIFY_TASK_SIZE 128
#define NOTIFY_TASK_PRIORITY TASK_PRIORITY_PROTOCOL_LOW
#define NOTIFY_TASK_QUEUE_SIZE 20

#define BLE_CFM_QUEUE_SIZE 2
/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
/* Protocol Task */
static sys_queue_t g_ble_app_queue; /**< The handle of BLE application request, BLE event and BLE data queue. */
sys_queue_t g_ble_cfm_queue;

/* APP Notify Task */
static sys_task_t g_app_notify_task;
static sys_queue_t g_app_notify_queue;

extern int ble_app_notify(sys_tlv_t * pt_tlv);

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/

static void notify_task_handle(void * arg)
{
    task_queue_t notify_queue   = { 0 };
    ble_app_evt_param_t t_queue = { 0 };
    uint32_t u32_time;
    sys_tlv_t * pt_tlv;

    configASSERT(&g_app_notify_queue != NULL);
    configASSERT(&g_ble_app_queue != NULL);

    for (;;)
    {
        u32_time = SYS_ARCH_TIMEOUT;
        if (sys_queue_remaining_size(&g_app_notify_queue) != NOTIFY_TASK_QUEUE_SIZE)
        {
            if (sys_arch_queue_tryrecv(&g_app_notify_queue, &notify_queue) != SYS_ARCH_TIMEOUT)
            {
                pt_tlv = notify_queue.pt_tlv;
                if (ble_app_notify(pt_tlv) != BLE_ERR_OK) // callback to application
                {
                    sys_queue_sendtofront(&g_app_notify_queue, &notify_queue);
                }
                else
                {
                    mem_free(pt_tlv);
                }
            }
            if (sys_queue_remaining_size(&g_app_notify_queue) != 0)
            {
                u32_time = sys_arch_queue_tryrecv(&g_ble_app_queue, &t_queue);
            }
        }
        else
        {
            // wait for the event
            u32_time = sys_queue_recv(&g_ble_app_queue, (void *) &t_queue, 0);
        }

        if (u32_time != SYS_ARCH_TIMEOUT)
        {
            ble_app_evt_param_t * p_param = (ble_app_evt_param_t *) &t_queue;

            switch (p_param->type)
            {
            case BLE_APP_GENERAL_EVENT:
                ble_cmd_event_post_to_module(p_param->event_param.p_ble_evt_param);
                mem_free(t_queue.event_param.p_ble_evt_param);
                break;

            case BLE_APP_RETURN_PARAMETER_EVENT:
                if (ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, (void *) p_param->event_param.p_ble_evt_param) == BLE_ERR_OK)
                {
                    mem_free(t_queue.event_param.p_ble_evt_param);
                }
                else
                {
                    sys_queue_sendtofront(&g_ble_app_queue, &t_queue);
                    sys_task_delay(2);
                }
                break;

#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
            case BLE_APP_SERVICE_EVENT:
                if (ble_event_post_to_notify(BLE_APP_SERVICE_EVENT, (void *) p_param->event_param.p_ble_service_param) ==
                    BLE_ERR_OK)
                {
                    mem_free(t_queue.event_param.p_ble_service_param);
                }
                else
                {
                    sys_queue_sendtofront(&g_ble_app_queue, &t_queue);
                    sys_task_delay(2);
                }
                break;
#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))

            default:
                break;
            }
        }
    }
}

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/
uint32_t ble_wait_cfm(sys_tlv_t ** pt_cfm_tlv, uint32_t u32_timeout)
{
    uint32_t u32_time;
    task_queue_t t_queue = { 0 };
    u32_time             = sys_queue_recv(&g_ble_cfm_queue, (void *) &t_queue, u32_timeout);
    if (u32_time != SYS_ARCH_TIMEOUT)
    {
        *pt_cfm_tlv = t_queue.pt_tlv;
    }
    return u32_time;
}

/**@brief Retrun the space of free space in BLE application queue. */
uint8_t get_app_queue_remaining_size(void)
{
    return sys_queue_remaining_size(&g_ble_app_queue);
}

ble_err_t notify_evt_queue_send(uint8_t type, uint16_t param_len, void * param)
{
    ble_err_t status = BLE_ERR_OK;
    task_queue_t p_queue;

    p_queue.pt_tlv = mem_malloc(sizeof(sys_tlv_t) + param_len);

    if (p_queue.pt_tlv != NULL)
    {
        p_queue.u32_send_systick = sys_now();
        p_queue.pt_tlv->type     = type; // BLE_APP_GENERAL_EVENT, BLE_APP_SERVICE_EVENT
        p_queue.pt_tlv->length   = param_len;
        memcpy(p_queue.pt_tlv->value, param, param_len);

        sys_queue_send(&g_app_notify_queue, (void *) &p_queue);
    }
    else
    {
        status = BLE_ERR_ALLOC_MEMORY_FAIL;
    }

    return status;
}

/**@brief Post the application event parameters on the BLE application queue. */
ble_err_t task_ble_app_queue_send(ble_app_evt_param_t p_param)
{
    ble_err_t status = BLE_ERR_OK;

    if ((p_param.type == BLE_APP_RETURN_PARAMETER_EVENT) || (p_param.type == BLE_APP_GENERAL_EVENT) ||
        (p_param.type == BLE_APP_SERVICE_EVENT))
    {
        // send
        if (sys_queue_send_with_timeout(&g_ble_app_queue, (void *) &p_param, CONFIG_QUEUE_SEND_TIMEOUT_MS) == ERR_TIMEOUT)
        {
            status = BLE_ERR_SENDTO_FAIL;
        }
    }
    else
    {
        status = BLE_ERR_WRONG_CONFIG;
    }

    return status;
}

ble_err_t ble_queue_sendto(sys_tlv_t * pt_tlv)
{
    ble_err_t t_return = BLE_ERR_OK;

    /*-----------------------------------*/
    /* A.Input Parameter Range Check     */
    /*-----------------------------------*/
    if (pt_tlv == NULL)
    {
        return BLE_ERR_SENDTO_POINTER_NULL;
    }

    /*-----------------------------------*/
    /* B. Main Functionality             */
    /*-----------------------------------*/
    do
    {
        task_queue_t t_queue;

        t_queue.u32_send_systick = sys_now();
        t_queue.pt_tlv           = pt_tlv;

        if (sys_queue_send_with_timeout(&g_cmd_transport_handle, (void *) &t_queue, CONFIG_QUEUE_SEND_TIMEOUT_MS) == ERR_TIMEOUT)
        {
            t_return = BLE_ERR_SENDTO_FAIL;
            break;
        }
    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    return t_return;
}

int8_t task_delete_ble_app(void)
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
        sys_queue_free(&g_ble_app_queue);
        sys_queue_free(&g_app_notify_queue);
        sys_queue_free(&g_ble_cfm_queue);

        vTaskDelete(g_app_notify_task);
    } while (0);

    /*-----------------------------------*/
    /* C. Result & Return                */
    /*-----------------------------------*/
    return err_code;
}

/**@brief BLE application task initialization. */
int task_ble_app_init(void)
{
    int i32_ret = 0;

    do
    {
        if (sys_queue_new(&g_ble_app_queue, CONFIG_APP_QUEUE_SIZE, sizeof(ble_app_evt_param_t)) != ERR_OK)
        {
            i32_ret = -1;
            break;
        }

        if (sys_queue_new(&g_app_notify_queue, NOTIFY_TASK_QUEUE_SIZE, sizeof(task_queue_t)) != ERR_OK)
        {
            i32_ret = -1;
            break;
        }

        if (sys_queue_new(&g_ble_cfm_queue, BLE_CFM_QUEUE_SIZE, sizeof(task_queue_t)) != ERR_OK)
        {
            i32_ret = -1;
            break;
        }

        g_app_notify_task = sys_task_new("notify_t", notify_task_handle, NULL, NOTIFY_TASK_SIZE, NOTIFY_TASK_PRIORITY);

        if (g_app_notify_task == NULL)
        {
            i32_ret = -1;
            break;
        }
    } while (0);

    return i32_ret;
}

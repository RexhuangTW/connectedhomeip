/** @file ble_scan.c
 *
 * @brief Define BLE scan related definition, structure and functions.
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "lib_config.h"

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
#include "ble_scan_api.h"
#include "ble_gap_api.h"
#include "ble_advertising_api.h"
#include "sys_arch.h"
#include "ble_hci.h"
#include "hci_cmd.h"
#include "ble_event_module.h"
#include "ble_printf.h"
#include "ble_privacy_api.h"
#include "smp.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define SCAN_OWN_ADDR_TYPE_PUBLIC               0UL
#define SCAN_OWN_ADDR_TYPE_RANDOM               1UL
#define SCAN_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR  2UL
#define SCAN_OWN_ADDR_TYPE_RESOLVABLE_ADDR      3UL

/**
 * @brief  Define scan state.
 */
typedef uint8_t ble_scan_state_t;
#define SCAN_ENABLE_PROCESSING           0x01    /**< Scan enable command is processing. */
#define SCAN_ENABLED                     0x02    /**< Scan is enabled. */
#define SCAN_DISABLE_PROCESSING          0x03    /**< Scan disable command is processing. */
#define SCAN_DISABLED                    0x04    /**< Scan is disabled. */
/** @} */


/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
static ble_scan_state_t g_scan_enable = SCAN_DISABLED;
static ble_scan_param_t g_scan_param =
{
    .scan_type = SCAN_TYPE_ACTIVE,
    .own_addr_type = SCAN_OWN_ADDR_TYPE_RANDOM,
    .scan_window = 80,
    .scan_interval = 160,
    .scan_filter_policy = SCAN_FILTER_POLICY_BASIC_UNFILTERED,
};

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
static ble_err_t scan_status_set(ble_scan_state_t state)
{
    vPortEnterCritical();
    g_scan_enable = state;
    vPortExitCritical();

    return BLE_ERR_OK;
}

static ble_err_t scan_param_set(ble_scan_param_t *p_param)
{
    vPortEnterCritical();
    g_scan_param.scan_type = p_param->scan_type;
    g_scan_param.own_addr_type = p_param->own_addr_type;
    g_scan_param.scan_window = p_param->scan_window;
    g_scan_param.scan_interval = p_param->scan_interval;
    g_scan_param.scan_filter_policy = p_param->scan_filter_policy;
    vPortExitCritical();

    return BLE_ERR_OK;
}

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/** Check if the BLE scan is disabled or not.
 *
 */
bool ble_scan_idle_state_check(void)
{
    if (g_scan_enable == SCAN_DISABLED)
    {
        return TRUE;
    }

    return FALSE;
}


/** Set BLE scan parameters.
 *
 */
ble_err_t ble_scan_param_set(ble_scan_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_scan_param_param_t p_hci_param;
    ble_gap_addr_t ble_device_addr;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check scan filter policy
    if ((p_param->scan_filter_policy == SCAN_FILTER_POLICY_EXTENED_UNFILTERED) || (p_param->scan_filter_policy == SCAN_FILTER_POLICY_EXTENED_FILTERED))
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // check status
    if ((g_scan_enable == SCAN_ENABLE_PROCESSING) || (g_scan_enable == SCAN_ENABLED))
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check parameters
    if ( (p_param->scan_type != SCAN_TYPE_PASSIVE && p_param->scan_type != SCAN_TYPE_ACTIVE) ||
            (p_param->scan_interval < SCAN_INTERVAL_MIN || p_param->scan_interval > SCAN_INTERVAL_MAX) ||
            (p_param->scan_window < SCAN_WINDOW_MIN || p_param->scan_window > SCAN_WINDOW_MAX) ||
            (p_param->scan_window > p_param->scan_interval))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // get device address
    ble_gap_device_address_get(&ble_device_addr);

    // set HCI parameters
    p_hci_param.scan_type = p_param->scan_type;
    p_hci_param.scan_interval = p_param->scan_interval;
    p_hci_param.scan_window = p_param->scan_window;
    p_hci_param.scan_filter_policy = p_param->scan_filter_policy;

    if (ble_device_addr.addr_type == PUBLIC_ADDR)
    {
        p_hci_param.own_addr_type = SCAN_OWN_ADDR_TYPE_PUBLIC;
    }
    else if ((ble_device_addr.addr_type == RANDOM_STATIC_ADDR) ||
             (ble_device_addr.addr_type == RANDOM_NON_RESOLVABLE_ADDR) ||
             (ble_device_addr.addr_type == RANDOM_RESOLVABLE_ADDR))
    {
        p_hci_param.own_addr_type = SCAN_OWN_ADDR_TYPE_RANDOM;
    }
    else
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // issue HCI cmd
    if (hci_le_set_scan_param_cmd(&p_hci_param) == ERR_MEM)
    {
        return BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        // set to local parameter
        scan_param_set(p_param);
        status = BLE_ERR_OK;
    }

    return status;
}


/** Enable BLE scan.
 *
 */
ble_err_t ble_scan_enable(void)
{
    uint8_t host_id;
    ble_gap_addr_t addr_param;
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_scan_enable_param_t p_hci_cmd_parm;

    // check status
    if (ble_scan_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    if ((g_scan_param.own_addr_type == SCAN_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR) ||
            (g_scan_param.own_addr_type == SCAN_OWN_ADDR_TYPE_RESOLVABLE_ADDR))
    {
        if (ble_privacy_parameter_enable_check() == FALSE)
        {
            return BLE_ERR_INVALID_STATE;
        }
        if ((ble_adv_idle_state_check() == FALSE) || (ble_init_idle_state_check() == FALSE))
        {
            return BLE_ERR_INVALID_STATE;
        }
        host_id = ble_privacy_host_id_get();
        addr_param.addr_type = g_scan_param.own_addr_type;
        if (g_scan_param.own_addr_type == SCAN_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR)
        {
            param_rpa[host_id].rpa_addr[5] &= 0x3F;
        }
        memcpy(addr_param.addr, param_rpa[host_id].rpa_addr, BLE_ADDR_LEN);
        ble_gap_device_address_set(&addr_param);
    }
    else
    {
        if (ble_gap_device_address_compare() != BLE_ERR_OK)
        {
            if ((ble_adv_idle_state_check() == FALSE) || (ble_init_idle_state_check() == FALSE))
            {
                return BLE_ERR_INVALID_STATE;
            }
            ble_gap_device_identity_address_get(&addr_param);
            ble_gap_device_address_set(&addr_param);
        }
    }

    // set HCI parameters
    p_hci_cmd_parm.scan_enable = 0x01;
    p_hci_cmd_parm.scan_filter_duplicates = 0x00;

    // set status
    scan_status_set(SCAN_ENABLE_PROCESSING);

    // issue HCI cmd
    if (hci_le_set_scan_enable_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** Disable BLE scan.
 *
 */
ble_err_t ble_scan_disable(void)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_scan_enable_param_t p_hci_cmd_parm;

    // check status
    if ((g_scan_enable == SCAN_DISABLE_PROCESSING) || (g_scan_enable == SCAN_DISABLED))
    {
        return BLE_ERR_INVALID_STATE;
    }

    // set status
    scan_status_set(SCAN_DISABLE_PROCESSING);

    // set HCI parameters
    p_hci_cmd_parm.scan_enable = 0x00;
    p_hci_cmd_parm.scan_filter_duplicates = 0x00;

    // issue HCI cmd
    if (hci_le_set_scan_enable_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** BLE scan module event handler.
 *
 */
ble_err_t ble_evt_scan_handler(void *p_param)
{
    ble_err_t status;
    ble_evt_param_t *p_evt_param = (ble_evt_param_t *)p_param;

    status = BLE_ERR_OK;
    switch (p_evt_param->event)
    {
    case BLE_SCAN_EVT_SET_PARAM:
    {
        ble_evt_scan_set_scan_param_t *p_scan_param = (ble_evt_scan_set_scan_param_t *)&p_evt_param->event_param.ble_evt_scan.param.evt_set_scan_param;

        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[SCAN] SET PARAM status = %d\n", p_scan_param->status);
        }
    }
    break;

    case BLE_SCAN_EVT_SET_ENABLE:
    {
        ble_evt_scan_set_scan_enable_t *p_scan_enable = (ble_evt_scan_set_scan_enable_t *)&p_evt_param->event_param.ble_evt_scan.param.evt_set_scan_enable;

        // post to user
        if (p_scan_enable->status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            if (g_scan_enable == SCAN_ENABLE_PROCESSING)
            {
                p_scan_enable->scan_enabled = TRUE;
            }
            else if (g_scan_enable == SCAN_DISABLE_PROCESSING)
            {
                p_scan_enable->scan_enabled = FALSE;
            }
        }
        else
        {
            // recover to last state
            if (g_scan_enable == SCAN_ENABLE_PROCESSING)
            {
                p_scan_enable->scan_enabled = FALSE;
            }
            else if (g_scan_enable == SCAN_DISABLE_PROCESSING)
            {
                p_scan_enable->scan_enabled = TRUE;
            }
        }
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            if (p_scan_enable->status == BLE_HCI_ERR_CODE_SUCCESS)
            {
                if (g_scan_enable == SCAN_ENABLE_PROCESSING)
                {
                    scan_status_set(SCAN_ENABLED);
                }
                else if (g_scan_enable == SCAN_DISABLE_PROCESSING)
                {
                    scan_status_set(SCAN_DISABLED);
                }
            }
            else
            {
                // recover to last state
                if (g_scan_enable == SCAN_ENABLE_PROCESSING)
                {
                    scan_status_set(SCAN_DISABLED);
                }
                else if (g_scan_enable == SCAN_DISABLE_PROCESSING)
                {
                    scan_status_set(SCAN_ENABLED);
                }
            }

            BLE_PRINTF(BLE_DEBUG_LOG, "[SCAN] SET ENABLE status = %d\n", p_scan_enable->status);
        }
    }
    break;

    case BLE_SCAN_EVT_ADV_REPORT:
    {
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
    }
    break;

    default:
        break;
    }

    return status;
}



#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))

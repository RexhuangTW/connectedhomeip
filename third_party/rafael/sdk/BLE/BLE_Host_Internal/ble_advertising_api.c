/** @file ble_advertising.c
 *
 * @brief
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "lib_config.h"

#if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
#include "ble_advertising_api.h"
#include "ble_gap_api.h"
#include "ble_scan_api.h"
#include "ble_hci.h"
#include "hci_cmd.h"
#include "ble_host_cmd.h"
#include "ble_event_module.h"
#include "ble_profile.h"
#include "ble_printf.h"
#include "smp.h"
#include "ble_privacy_api.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/

#define ADV_OWN_ADDR_TYPE_PUBLIC                0UL
#define ADV_OWN_ADDR_TYPE_RANDOM                1UL
#define ADV_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR   2UL
#define ADV_OWN_ADDR_TYPE_RESOLVABLE_ADDR       3UL

/**
 * @brief  Define advertising state.
 */
typedef uint8_t ble_adv_state_t;
#define ADV_ENABLE_PROCESSING           0x01    /**< Advertising enable command is processing. */
#define ADV_ENABLED                     0x02    /**< Advertising is enabled. */
#define ADV_DISABLE_PROCESSING          0x03    /**< Advertising disable command is processing. */
#define ADV_DISABLED                    0x04    /**< Advertising is disabled. */
/** @} */


/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
static ble_adv_state_t g_adv_enable = ADV_DISABLED;
static ble_adv_param_t g_adv_param =
{
    .own_addr_type = ADV_OWN_ADDR_TYPE_RANDOM,
    .adv_type = ADV_TYPE_ADV_IND,
    .adv_interval_min = 160,    // 100ms
    .adv_interval_max = 160,    // 100ms
    .adv_peer_addr_param = 0,
    .adv_channel_map = ADV_CHANNEL_ALL,
    .adv_filter_policy = ADV_FILTER_POLICY_ACCEPT_ALL,
};


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
static ble_err_t adv_status_set(ble_adv_state_t state)
{
    vPortEnterCritical();
    g_adv_enable = state;
    vPortExitCritical();

    return BLE_ERR_OK;
}

static ble_err_t adv_param_set(ble_adv_param_t *p_param)
{
    vPortEnterCritical();
    g_adv_param.own_addr_type = p_param->own_addr_type;
    g_adv_param.adv_channel_map = p_param->adv_channel_map;
    g_adv_param.adv_filter_policy = p_param->adv_filter_policy;
    g_adv_param.adv_interval_max = p_param->adv_interval_max;
    g_adv_param.adv_interval_min = p_param->adv_interval_min;
    g_adv_param.adv_peer_addr_param.addr_type = p_param->adv_peer_addr_param.addr_type;
    memcpy(g_adv_param.adv_peer_addr_param.addr, p_param->adv_peer_addr_param.addr, BLE_ADDR_LEN);
    g_adv_param.adv_type = p_param->adv_type;
    vPortExitCritical();

    return BLE_ERR_OK;
}

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/** Check if the BLE advertising is disabled or not.
 *
 */
bool ble_adv_idle_state_check(void)
{
    if (g_adv_enable == ADV_DISABLED)
    {
        return TRUE;
    }

    return FALSE;
}


/** Set BLE advertising parameters.
 *
 */
ble_err_t ble_adv_param_set(ble_adv_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_adv_param_param_t p_hci_param;
    ble_gap_addr_t ble_device_addr;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check status
    if (ble_adv_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check parameters
    if ( (p_param->adv_type > ADV_TYPE_ADV_NONCONN_IND) ||
            (p_param->adv_channel_map == 0x00) ||
            (p_param->adv_interval_min < ADV_INTERVAL_MIN || p_param->adv_interval_min > ADV_INTERVAL_MAX) ||
            (p_param->adv_interval_max < ADV_INTERVAL_MIN || p_param->adv_interval_max > ADV_INTERVAL_MAX) ||
            (p_param->adv_interval_min > p_param->adv_interval_max) ||
            (p_param->adv_channel_map > ADV_CHANNEL_ALL) ||
            (p_param->adv_filter_policy > ADV_FILTER_POLICY_ACCEPT_SCAN_CONN_REQ_FROM_FAL))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // get BLE device address
    ble_gap_device_address_get(&ble_device_addr);

    // set HCI parameters
    p_hci_param.adv_channel_map = p_param->adv_channel_map;
    p_hci_param.adv_filter_policy = p_param->adv_filter_policy;
    p_hci_param.adv_interval_max = p_param->adv_interval_max;
    p_hci_param.adv_interval_min = p_param->adv_interval_min;
    p_hci_param.peer_addr_type = p_param->adv_peer_addr_param.addr_type;
    memcpy(p_hci_param.peer_addr, p_param->adv_peer_addr_param.addr, BLE_ADDR_LEN);
    p_hci_param.adv_type = p_param->adv_type;

    if (ble_device_addr.addr_type == PUBLIC_ADDR)
    {
        p_hci_param.own_addr_type = ADV_OWN_ADDR_TYPE_PUBLIC;
    }
    else if ((ble_device_addr.addr_type == RANDOM_STATIC_ADDR) ||
             (ble_device_addr.addr_type == RANDOM_NON_RESOLVABLE_ADDR) ||
             (ble_device_addr.addr_type == RANDOM_RESOLVABLE_ADDR))
    {
        p_hci_param.own_addr_type = ADV_OWN_ADDR_TYPE_RANDOM;
    }
    else
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // issue HCI cmd
    if (hci_le_set_adv_param_cmd(&p_hci_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        // set to local parameter
        adv_param_set(p_param);

        status = BLE_ERR_OK;
    }

    return status;
}


/** Set BLE advertising data.
 *
 */
ble_err_t ble_adv_data_set(ble_adv_data_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_adv_data_param_t p_hci_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check data length
    if (p_param->length > BLE_ADV_DATA_SIZE_MAX)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // set HCI parameters
    p_hci_param.adv_data_length = p_param->length;
    memcpy(p_hci_param.adv_data, p_param->data, p_param->length);

    // issue HCI cmd
    if (hci_le_set_adv_data_cmd(&p_hci_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** Set BLE advertising scan response data.
 *
 */
ble_err_t ble_adv_scan_rsp_set(ble_adv_data_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_scan_rsp_param_t p_hci_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check data length
    if (p_param->length > BLE_ADV_DATA_SIZE_MAX)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // set HCI parameters
    p_hci_param.scan_rsp_data_length = p_param->length;
    memcpy(p_hci_param.scan_rsp_data, p_param->data, p_param->length);

    // issue HCI cmd
    if (hci_le_set_scan_rsp_data_cmd(&p_hci_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** Enable BLE advertising.
 *
 */
ble_err_t ble_adv_enable(ble_adv_enable_param_t *p_param)
{
    ble_gap_addr_t addr_param;
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_adv_enable_param_t p_hci_param;
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check status
    if (ble_adv_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == TRUE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check adv type
    if (((p_param->host_id == BLE_HOSTID_RESERVED) || (bhc_host_connected_link_number_get() == MAX_CONN_NO_APP)) &&
            ((g_adv_param.adv_type != ADV_TYPE_ADV_NONCONN_IND) && (g_adv_param.adv_type != ADV_TYPE_ADV_NONCONN_IND)))

    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    if (ble_privacy_parameter_enable_check() == TRUE)
    {
        if ((ble_scan_idle_state_check() == FALSE) || (ble_init_idle_state_check() == FALSE))
        {
            return BLE_ERR_INVALID_STATE;
        }
    }

    if ((g_adv_param.own_addr_type == ADV_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR) ||
            (g_adv_param.own_addr_type == ADV_OWN_ADDR_TYPE_RESOLVABLE_ADDR))
    {
        if ((ble_scan_idle_state_check() == FALSE) || (ble_init_idle_state_check() == FALSE))
        {
            return BLE_ERR_INVALID_STATE;
        }
        addr_param.addr_type = g_adv_param.own_addr_type;
        if (g_adv_param.own_addr_type == ADV_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR)
        {
            param_rpa[p_param->host_id].rpa_addr[5] &= 0x3F;
        }
        memcpy(addr_param.addr, param_rpa[p_param->host_id].rpa_addr, BLE_ADDR_LEN);
        ble_gap_device_address_set(&addr_param);
    }
    else
    {
        if (ble_gap_device_address_compare() != BLE_ERR_OK)
        {
            if ((ble_scan_idle_state_check() == FALSE) || (ble_init_idle_state_check() == FALSE))
            {
                return BLE_ERR_INVALID_STATE;
            }
            ble_gap_device_identity_address_get(&addr_param);
            ble_gap_device_address_set(&addr_param);
        }
    }

    // set status
    adv_status_set(ADV_ENABLE_PROCESSING);

    // set HCI parameters
    p_hci_param.adv_enable = 0x01; // enable

    // issue HCI cmd

    // set host id to active mode
    bhc_host_id_state_active_set(p_param->host_id, BLE_GAP_ROLE_PERIPHERAL);
    if (hci_le_set_adv_enable_cmd(&p_hci_param) == ERR_MEM)
    {
        bhc_host_id_state_active_release(BLE_GAP_ROLE_PERIPHERAL);
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** Disable BLE advertising.
 *
 */
ble_err_t ble_adv_disable(void)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_adv_enable_param_t p_hci_param;

    // check status
    if ((g_adv_enable == ADV_DISABLE_PROCESSING) || (g_adv_enable == ADV_DISABLED))
    {
        return BLE_ERR_INVALID_STATE;
    }

    // set status
    adv_status_set(ADV_DISABLE_PROCESSING);

    // set HCI parameters
    p_hci_param.adv_enable = 0x00; // disable

    // issue HCI cmd
    if (hci_le_set_adv_enable_cmd(&p_hci_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** BLE advertising module event handler.
 *
 */
ble_err_t ble_evt_adv_handler(void *p_param)
{
    ble_err_t status;
    ble_evt_param_t *p_evt_param = (ble_evt_param_t *)p_param;

    status = BLE_ERR_OK;
    switch (p_evt_param->event)
    {
    case BLE_ADV_EVT_SET_PARAM:
    {
        ble_evt_adv_set_adv_param_t *p_adv_param = (ble_evt_adv_set_adv_param_t *)&p_evt_param->event_param.ble_evt_adv.param.evt_set_adv_param;

        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[ADV] SET PARAM status = %d\n", p_adv_param->status);
        }
    }
    break;

    case BLE_ADV_EVT_SET_DATA:
    {
        ble_evt_adv_set_adv_data_t *p_adv_data = (ble_evt_adv_set_adv_data_t *)&p_evt_param->event_param.ble_evt_adv.param.evt_set_adv_data;

        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[ADV] SET DATA status = %d\n", p_adv_data->status);
        }
    }
    break;

    case BLE_ADV_EVT_SET_SCAN_RSP:
    {
        ble_evt_adv_set_scan_rsp_t *p_scan_rsp = (ble_evt_adv_set_scan_rsp_t *)&p_evt_param->event_param.ble_evt_adv.param.evt_set_scan_rsp;

        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[ADV] SET SCAN RSP status = %d\n", p_scan_rsp->status);
        }
    }
    break;

    case BLE_ADV_EVT_SET_ENABLE:
    {
        ble_evt_adv_set_adv_enable_t *p_adv_enable = &p_evt_param->event_param.ble_evt_adv.param.evt_set_adv_enable;

        // post to user
        if (p_adv_enable->status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            if (g_adv_enable == ADV_ENABLE_PROCESSING)
            {
                p_adv_enable->adv_enabled = TRUE;
            }
            else if (g_adv_enable == ADV_DISABLE_PROCESSING)
            {
                p_adv_enable->adv_enabled = FALSE;
            }
        }
        else
        {
            // recover to last state
            if (g_adv_enable == ADV_ENABLE_PROCESSING)
            {
                p_adv_enable->adv_enabled = FALSE;
            }
            else if (g_adv_enable == ADV_DISABLE_PROCESSING)
            {
                p_adv_enable->adv_enabled = TRUE;
            }
        }

        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            if (p_adv_enable->status == BLE_HCI_ERR_CODE_SUCCESS)
            {
                if (g_adv_enable == ADV_ENABLE_PROCESSING)
                {
                    adv_status_set(ADV_ENABLED);
                }
                else if (g_adv_enable == ADV_DISABLE_PROCESSING)
                {
                    // update host id state
                    bhc_host_id_state_active_release(BLE_GAP_ROLE_PERIPHERAL);
                    adv_status_set(ADV_DISABLED);
                }
            }
            else
            {
                // recover to last state
                if (g_adv_enable == ADV_ENABLE_PROCESSING)
                {
                    adv_status_set(ADV_DISABLED);
                }
                else if (g_adv_enable == ADV_DISABLE_PROCESSING)
                {
                    adv_status_set(ADV_ENABLED);
                }
            }
            BLE_PRINTF(BLE_DEBUG_LOG, "[ADV] SET ENABLE status = %d\n", p_adv_enable->status);
        }
    }
    break;

    case BLE_GAP_EVT_CONN_COMPLETE:
    {
        ble_evt_gap_conn_complete_t *p_conn_param = (ble_evt_gap_conn_complete_t *)&p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete;

        if ((p_conn_param->role == BLE_GAP_ROLE_PERIPHERAL) || (p_conn_param->status == BLE_HCI_ERR_CODE_DIRECTED_ADVERTISING_TIMEOUT))
        {
            adv_status_set(ADV_DISABLED);
        }
    }
    break;

    default:
        break;
    }

    return status;
}
#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))

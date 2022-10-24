/** @file ble_gap.c
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
#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
#include "ble_scan_api.h"
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))

#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
#include "ble_profile.h"
#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))

#include "ble_common_api.h"
#include "ble_gap_api.h"
#include "sys_arch.h"
#include "mem_mgmt.h"
#include "ble_hci.h"
#include "hci_cmd.h"
#include "ble_host_cmd.h"
#include "ble_event_module.h"
#include "ble_printf.h"
#include "ble_api.h"
#include "ble_privacy_api.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define MSK_HCI_CONNID_16_CENTRAL                   0x0200
#define MSK_HCI_CONNID_16_PERIPHERAL                0x0300

#define CRCON_OWN_ADDR_TYPE_PUBLIC                  0UL
#define CRCON_OWN_ADDR_TYPE_RANDOM                  1UL
#define CRCON_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR     2UL
#define CRCON_OWN_ADDR_TYPE_RESOLVABLE_ADDR         3UL

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_) && BLE_MODULE_ENABLE(_CONN_SUPPORT_))

/**
* @brief  Define initiator state.
*/
typedef uint8_t ble_cmd_state_t;
#define STATE_IDLE                    0x00    /**< Idle mode. */
#define STATE_CMD_PROCESSING          0x01    /**< Command is in processing mode. */
/** @} */

#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_) && BLE_MODULE_ENABLE(_CONN_SUPPORT_))



/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/


/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
static ble_gap_addr_t  g_ble_device_addr =
{
    .addr_type = RANDOM_STATIC_ADDR,
    .addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0xC1}
};

static ble_gap_addr_t  g_ble_identity_addr =
{
    .addr_type = RANDOM_STATIC_ADDR,
    .addr = {0x11, 0x12, 0x13, 0x14, 0x15, 0xC1}
};


#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_) && BLE_MODULE_ENABLE(_CONN_SUPPORT_))

ble_cmd_state_t  g_init_enable    = STATE_IDLE;

#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_) && BLE_MODULE_ENABLE(_CONN_SUPPORT_))


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_) && BLE_MODULE_ENABLE(_CONN_SUPPORT_))
static ble_err_t ble_init_status_set(ble_cmd_state_t state)
{
    vPortEnterCritical();
    g_init_enable = state;
    vPortExitCritical();

    return BLE_ERR_OK;
}
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_) && BLE_MODULE_ENABLE(_CONN_SUPPORT_))


static ble_err_t ble_global_addr_set(ble_gap_addr_t *p_addr)
{
    vPortEnterCritical();

    if ((p_addr->addr_type  == PUBLIC_ADDR) ||
            (p_addr->addr_type  == RANDOM_STATIC_ADDR))
    {
        memcpy(&g_ble_identity_addr, p_addr, sizeof(ble_gap_addr_t));
    }
    else
    {
        p_addr->addr_type = RANDOM_STATIC_ADDR;
    }
    memcpy(&g_ble_device_addr, p_addr, sizeof(ble_gap_addr_t));
    vPortExitCritical();

    return BLE_ERR_OK;
}


static ble_err_t random_addr_set(ble_gap_addr_t *p_addr)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_random_addr_param_t p_hci_cmd_parm;

    // set HCI parameters
    memcpy(p_hci_cmd_parm.addr, p_addr->addr, BLE_ADDR_LEN);

    // issue HCI cmd
    if (hci_le_set_random_addr_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        ble_global_addr_set(p_addr);
        status = BLE_ERR_OK;
    }

    return status;
}


static ble_err_t public_addr_set(ble_gap_addr_t *p_addr)
{
    ble_err_t status = BLE_ERR_OK;

    status = ble_common_controller_info_set(BLE_STACK_VERSION, BLE_COMPANY_ID, p_addr->addr);
    if (status != BLE_ERR_OK)
    {
        BLE_PRINTF(BLE_DEBUG_ERR, "[COMMON] ble_common_controller_info_set() returns fail %d\n", status);
    }
    else
    {
        ble_global_addr_set(p_addr);
    }

    return status;
}


/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/
/** Get BLE device address type and address.
 *
 */
#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_) && BLE_MODULE_ENABLE(_CONN_SUPPORT_))
bool ble_init_idle_state_check(void)
{
    if (g_init_enable == STATE_IDLE)
    {
        return TRUE;
    }
    return FALSE;
}
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_) && BLE_MODULE_ENABLE(_CONN_SUPPORT_))


/** Get BLE device address type and address.
 *
 */
ble_err_t ble_gap_device_address_get(ble_gap_addr_t *p_addr)
{
    memcpy(p_addr, &g_ble_device_addr, sizeof(ble_gap_addr_t));

    return BLE_ERR_OK;
}


/** Get BLE device address type and address.
 *
 */
ble_err_t ble_gap_device_identity_address_get(ble_gap_addr_t *p_addr)
{
    memcpy(p_addr, &g_ble_identity_addr, sizeof(ble_gap_addr_t));

    return BLE_ERR_OK;
}


/** Get BLE device address type and address.
 *
 */
ble_err_t ble_gap_device_address_compare(void)
{
    ble_err_t status;

    status = BLE_ERR_OK;
    if (memcmp(&g_ble_device_addr.addr_type, &g_ble_identity_addr.addr_type, sizeof(ble_gap_addr_t)) != 0)
    {
        status = BLE_ERR_INVALID_PARAMETER;
    }

    return status;
}


/** Set BLE device address type and address.
 *
 */
ble_err_t ble_gap_device_address_set(ble_gap_addr_t *p_addr)
{
    ble_err_t status = BLE_ERR_OK;

#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
    int i = 0;

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
    // check init state
    if (ble_init_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif  // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))

    if (p_addr == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check connection
    for (i = 0; i < max_num_conn_host; i++)
    {
        uint16_t conn_id;

        if (bhc_host_id_is_connected_check(i, &conn_id) == TRUE)
        {
            return BLE_ERR_INVALID_STATE;
        }
    }
#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_)) 

#if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
    // check if in advertising state
    if (ble_adv_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
    // check if in scan state
    if (ble_scan_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))

    // check BLE address type
    status = BLE_ERR_INVALID_PARAMETER;

    switch (p_addr->addr_type)
    {
    case PUBLIC_ADDR:
        status = public_addr_set(p_addr);
        break;

    case RANDOM_STATIC_ADDR:
        // check ADDDR MSB[7:6] = 11
        if ((p_addr->addr[BLE_ADDR_LEN - 1] & 0xC0) == 0xC0 )
        {
            status = random_addr_set(p_addr);
        }
        break;
    case RANDOM_NON_RESOLVABLE_ADDR:
        // check ADDDR MSB[7:6] = 00
        if ((p_addr->addr[BLE_ADDR_LEN - 1] & 0xC0) == 0x00 )
        {
            status = random_addr_set(p_addr);
        }
        break;
    case RANDOM_RESOLVABLE_ADDR:
        // check ADDDR MSB[7:6] = 01
        if ((p_addr->addr[BLE_ADDR_LEN - 1] & 0xC0) == 0x40 )
        {
            status = random_addr_set(p_addr);
        }
        break;

    default:
        break;
    }

    return status;
}


#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))


#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))

/** BLE create connection.
 *
 */
ble_err_t ble_gap_connection_create(ble_gap_create_conn_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_create_conn_param_t p_hci_cmd_parm;
    uint16_t conn_id;
    ble_gap_addr_t ble_device_addr;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check maximum connection link number
    if (MAX_CONN_NO_APP == 0)
    {
        return BLE_ERR_CMD_NOT_SUPPORTED;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check state
    if ((bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == TRUE) ||
            (ble_init_idle_state_check() == FALSE) ||
            (ble_scan_idle_state_check() == FALSE) )
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check parameters
    if ((p_param->scan_interval < SCAN_INTERVAL_MIN || p_param->scan_interval > SCAN_INTERVAL_MAX) ||
            (p_param->scan_window < SCAN_WINDOW_MIN || p_param->scan_window > SCAN_WINDOW_MAX) ||
            (p_param->scan_window > p_param->scan_interval))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    if ( (p_param->conn_param.min_conn_interval < BLE_CONN_INTERVAL_MIN || p_param->conn_param.min_conn_interval > BLE_CONN_INTERVAL_MAX) ||
            (p_param->conn_param.max_conn_interval < BLE_CONN_INTERVAL_MIN || p_param->conn_param.max_conn_interval > BLE_CONN_INTERVAL_MAX) ||
            (p_param->conn_param.min_conn_interval > p_param->conn_param.max_conn_interval) ||
            (p_param->conn_param.periph_latency > BLE_CONN_LATENCY_MAX) ||
            (p_param->conn_param.supv_timeout < BLE_CONN_SUPV_TIMEOUT_MIN || p_param->conn_param.supv_timeout > BLE_CONN_SUPV_TIMEOUT_MAX) ||
            ((p_param->conn_param.supv_timeout * 4) < ((1 + p_param->conn_param.periph_latency) * p_param->conn_param.max_conn_interval )))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // get device address
    ble_gap_device_address_get(&ble_device_addr);

    // set HCI parameters
    p_hci_cmd_parm.scan_interval = p_param->scan_interval;
    p_hci_cmd_parm.scan_window = p_param->scan_window;
    p_hci_cmd_parm.init_filter_policy = p_param->init_filter_policy;
    p_hci_cmd_parm.peer_addr_type = p_param->peer_addr.addr_type;
    memcpy(p_hci_cmd_parm.peer_addr, p_param->peer_addr.addr, BLE_ADDR_LEN);
    p_hci_cmd_parm.conn_interval_min = p_param->conn_param.min_conn_interval;
    p_hci_cmd_parm.conn_interval_max = p_param->conn_param.max_conn_interval;
    p_hci_cmd_parm.max_latency = p_param->conn_param.periph_latency;
    p_hci_cmd_parm.supv_timeout = p_param->conn_param.supv_timeout;
    p_hci_cmd_parm.min_celength = 12; // TODO: 12?
    p_hci_cmd_parm.max_celength = 12; // TODO: 12?

    if (ble_device_addr.addr_type == PUBLIC_ADDR)
    {
        p_hci_cmd_parm.own_addr_type = CRCON_OWN_ADDR_TYPE_PUBLIC;
    }
    else if ((ble_device_addr.addr_type == RANDOM_STATIC_ADDR) ||
             (ble_device_addr.addr_type == RANDOM_NON_RESOLVABLE_ADDR) ||
             (ble_device_addr.addr_type == RANDOM_RESOLVABLE_ADDR))
    {
        p_hci_cmd_parm.own_addr_type = CRCON_OWN_ADDR_TYPE_RANDOM;
    }
    else
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    if (ble_privacy_parameter_enable_check() == TRUE)
    {
        if ((ble_adv_idle_state_check() == FALSE) || (ble_scan_idle_state_check() == FALSE))
        {
            return BLE_ERR_INVALID_STATE;
        }
    }

    if ((p_param->own_addr_type == CRCON_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR) ||
            (p_param->own_addr_type == CRCON_OWN_ADDR_TYPE_RESOLVABLE_ADDR))
    {
        if ((ble_adv_idle_state_check() == FALSE) || (ble_scan_idle_state_check() == FALSE))
        {
            return BLE_ERR_INVALID_STATE;
        }
        ble_device_addr.addr_type = p_param->own_addr_type;
        if (p_param->own_addr_type == CRCON_OWN_ADDR_TYPE_NON_RESOLVABLE_ADDR)
        {
            param_rpa[p_param->host_id].rpa_addr[5] &= 0x3F;
        }
        memcpy(ble_device_addr.addr, param_rpa[p_param->host_id].rpa_addr, BLE_ADDR_LEN);
        ble_gap_device_address_set(&ble_device_addr);
    }
    else
    {
        if (ble_gap_device_address_compare() != BLE_ERR_OK)
        {
            if ((ble_adv_idle_state_check() == FALSE) || (ble_scan_idle_state_check() == FALSE))
            {
                return BLE_ERR_INVALID_STATE;
            }
            ble_gap_device_identity_address_get(&ble_device_addr);
            ble_gap_device_address_set(&ble_device_addr);
        }
    }

    // set state
    ble_init_status_set(STATE_CMD_PROCESSING);

    // issue HCI cmd
    bhc_host_id_state_active_set(p_param->host_id, BLE_GAP_ROLE_CENTRAL);
    if (hci_le_create_conn_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        bhc_host_id_state_active_release(BLE_GAP_ROLE_CENTRAL);
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }
    return status;
}

/** BLE cancel create connection.
 *
 */
ble_err_t ble_gap_connection_cancel(void)
{
    ble_err_t status = BLE_ERR_OK;

    // check status
    if (ble_init_idle_state_check() == TRUE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // update state
    ble_init_status_set(STATE_IDLE);

    // issue HCI cmd
    if (hci_le_create_conn_cancel_cmd() == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))



/** BLE connection parameter update.
 *
 */
ble_err_t ble_gap_connection_update(ble_gap_conn_param_update_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_conn_updated_param_t p_hci_cmd_parm;
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check parameters
    if ( (p_param->ble_conn_param.min_conn_interval < BLE_CONN_INTERVAL_MIN || p_param->ble_conn_param.min_conn_interval > BLE_CONN_INTERVAL_MAX) ||
            (p_param->ble_conn_param.max_conn_interval < BLE_CONN_INTERVAL_MIN || p_param->ble_conn_param.max_conn_interval > BLE_CONN_INTERVAL_MAX) ||
            (p_param->ble_conn_param.min_conn_interval > p_param->ble_conn_param.max_conn_interval) ||
            (p_param->ble_conn_param.periph_latency > BLE_CONN_LATENCY_MAX) ||
            (p_param->ble_conn_param.supv_timeout < BLE_CONN_SUPV_TIMEOUT_MIN || p_param->ble_conn_param.supv_timeout > BLE_CONN_SUPV_TIMEOUT_MAX) ||
            ((p_param->ble_conn_param.supv_timeout * 4) <= ((1 + p_param->ble_conn_param.periph_latency) * p_param->ble_conn_param.max_conn_interval )))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // set HCI parameters
    if ((conn_id & 0xFF00) == MSK_HCI_CONNID_16_CENTRAL)
    {
        p_hci_cmd_parm.conn_handle = conn_id;
        p_hci_cmd_parm.conn_interval_min = p_param->ble_conn_param.min_conn_interval;
        p_hci_cmd_parm.conn_interval_max = p_param->ble_conn_param.max_conn_interval;
        p_hci_cmd_parm.periph_latency = p_param->ble_conn_param.periph_latency;
        p_hci_cmd_parm.supv_timeout = p_param->ble_conn_param.supv_timeout;
        p_hci_cmd_parm.max_celength = 12;
        p_hci_cmd_parm.min_celength = 12;

        // issue HCI cmd
        if (hci_le_conn_update_cmd(&p_hci_cmd_parm) == ERR_MEM)
        {
            status = BLE_ERR_SENDTO_FAIL;
        }
        else
        {
            status = BLE_ERR_OK;
        }
    }
    else
    {
        if ((bhc_timer_evt_get(p_param->host_id, TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP) == TIMER_EVENT_NULL) &&
                (bhc_timer_evt_get(p_param->host_id, TIMER_EVENT_CONN_UPDATE_COMPLETE) == TIMER_EVENT_NULL))
        {
            status = bhc_gap_connection_update(conn_id, &p_param->ble_conn_param);
        }
        else
        {
            status = BLE_BUSY;
        }
    }

    return status;
}


/** Terminate the BLE connection link.
 *
 */
ble_err_t ble_gap_conn_terminate(ble_gap_conn_terminate_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_disconnect_param_t p_hci_cmd_parm;
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // set HCI parameters
    p_hci_cmd_parm.conn_handle = conn_id;
    p_hci_cmd_parm.reason = BLE_HCI_ERR_CODE_REMOTE_USER_TERMINATED_CONNECTION;

    // issue HCI cmd
    if (hci_disconn_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** BLE PHY update.
 *
 */
ble_err_t ble_gap_phy_update(ble_gap_phy_update_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_set_phy_param_t p_hci_cmd_parm;
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check phy parameters
    if (((p_param->tx_phy != BLE_PHY_1M) && (p_param->tx_phy != BLE_PHY_2M) && (p_param->tx_phy != BLE_PHY_CODED)) ||
            ((p_param->rx_phy != BLE_PHY_1M) && (p_param->rx_phy != BLE_PHY_2M) && (p_param->rx_phy != BLE_PHY_CODED)) ||
            ((p_param->coded_phy_option != BLE_CODED_PHY_NO_PREFERRED) && (p_param->coded_phy_option != BLE_CODED_PHY_S2) && (p_param->coded_phy_option != BLE_CODED_PHY_S8)))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // set HCI parameters
    p_hci_cmd_parm.conn_handle = conn_id;
    p_hci_cmd_parm.all_phys = 0x00; // The Host has no preference among the transmitter PHYs supported by the Controller
    p_hci_cmd_parm.tx_phys = p_param->tx_phy;
    p_hci_cmd_parm.rx_phys = p_param->rx_phy;
    p_hci_cmd_parm.phy_options = p_param->coded_phy_option;

    // issue HCI cmd
    if (hci_le_set_phy_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** BLE read PHY.
 *
 */
ble_err_t ble_gap_phy_read(ble_gap_phy_read_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_read_phy_param_t p_hci_cmd_parm;
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // set HCI parameters
    p_hci_cmd_parm.conn_handle = conn_id;

    // issue HCI cmd
    if (hci_le_read_phy_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}



/** BLE read RSSI.
 *
 */
ble_err_t ble_gap_rssi_read(ble_gap_rssi_read_param_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_read_rssi_param_t p_hci_cmd_parm;
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // set HCI parameters
    p_hci_cmd_parm.conn_handle = conn_id;

    // issue HCI cmd
    if (hci_read_rssi_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** BLE set le host channel classification.
 *
 */
ble_err_t ble_gap_host_channel_classification_set(ble_gap_host_ch_classif_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;

    // issue HCI cmd
    if (hci_le_set_host_channel_classif_cmd((ble_hci_cmd_le_channel_classification_t *)p_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}


/** BLE read channel map.
 *
 */
ble_err_t ble_gap_channel_map_read(ble_gap_channel_map_read_t *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_cmd_le_read_channel_map_t p_hci_cmd_parm;
    uint16_t conn_id;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // set HCI parameters
    p_hci_cmd_parm.conn_handle = conn_id;

    // issue HCI cmd
    if (hci_le_read_channel_map_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}
#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))

ble_err_t ble_resolvable_address_init(void)
{
    uint8_t i, cmp[16];
    ble_err_t status;
    encrypt_queue_t encrypt_msg;
    int8_t err_code;

    status = BLE_ERR_OK;
    for (i = 0; i < MAX_CONN_NO_APP; i++)
    {
        do
        {
            memset(&cmp[0], 0xFF, SIZE_SMP_IRK);
            if (memcmp(LE_Host[i].OwnIRK, &cmp[0], SIZE_SMP_IRK) != 0)
            {
                memset(&cmp[0], 0x00, SIZE_SMP_IRK);
                if (memcmp(LE_Host[i].OwnIRK, &cmp[0], SIZE_SMP_IRK) != 0)
                {
                    memcpy(param_rpa[i].irk, LE_Host[i].OwnIRK, SIZE_SMP_IRK);
                    err_code = bhc_host_gen_resolvable_address(i);
                    if (err_code == ERR_MEM)
                    {
                        status = BLE_ERR_SENDTO_FAIL;
                    }
                    break;
                }
            }
            vPortEnterCritical();
            if (hci_le_random_cmd() == ERR_OK)
            {
                //send encrypt queue
                encrypt_msg.host_id = i;
                encrypt_msg.encrypt_state = STATE_GEN_LOCAL_IRK;
                sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
            }
            else
            {
                status = BLE_ERR_SENDTO_FAIL;
            }
            vPortExitCritical();
        } while (0);
        vTaskDelay(20);
    }

    return status;
}

ble_err_t ble_regenerate_resolvable_address(ble_gap_regen_resol_addr_t *p_param)
{
    uint8_t cmp[16];
    ble_err_t status;
    encrypt_queue_t encrypt_msg;
    int8_t err_code;

    if ((ble_adv_idle_state_check() == FALSE) || (ble_scan_idle_state_check() == FALSE) || (ble_init_idle_state_check() == FALSE))
    {
        return BLE_ERR_INVALID_STATE;
    }

    status = BLE_ERR_OK;
    do
    {
        if (p_param->gen_new_irk == DISABLE)
        {
            memset(&cmp[0], 0xFF, SIZE_SMP_IRK);
            if (memcmp(LE_Host[p_param->host_id].OwnIRK, &cmp[0], SIZE_SMP_IRK) != 0)
            {
                memset(&cmp[0], 0x00, SIZE_SMP_IRK);
                if (memcmp(LE_Host[p_param->host_id].OwnIRK, &cmp[0], SIZE_SMP_IRK) != 0)
                {
                    memcpy(param_rpa[p_param->host_id].irk, LE_Host[p_param->host_id].OwnIRK, SIZE_SMP_IRK);
                    err_code = bhc_host_gen_resolvable_address(p_param->host_id);
                    if (err_code == ERR_MEM)
                    {
                        status = BLE_ERR_SENDTO_FAIL;
                    }
                    break;
                }
            }
        }
        vPortEnterCritical();
        if (hci_le_random_cmd() == ERR_OK)
        {
            //send encrypt queue
            encrypt_msg.host_id = p_param->host_id;
            encrypt_msg.encrypt_state = STATE_GEN_LOCAL_IRK;
            sys_queue_send(&g_host_encrypt_handle, &encrypt_msg);
        }
        else
        {
            status = BLE_ERR_SENDTO_FAIL;
        }
        vPortExitCritical();
    } while (0);
    vTaskDelay(20);

    return status;
}

ble_err_t ble_connection_cte_rx_param_set(ble_connection_cte_rx_param_t *p_param)
{
    ble_err_t status;
    ble_hci_cmd_set_conn_cte_rx_param_t *p_cte_rx_param;
    uint16_t conn_id;
    uint8_t i;

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    p_cte_rx_param = mem_malloc(sizeof(ble_hci_cmd_set_conn_cte_rx_param_t) + p_param->sw_pattern_length);
    p_cte_rx_param->conn_handle = conn_id;
    p_cte_rx_param->sampling_enable = p_param->sampling_enable;
    p_cte_rx_param->slot_durations = p_param->slot_durations;
    p_cte_rx_param->sw_pattern_length = p_param->sw_pattern_length;
    for (i = 0; i < p_param->sw_pattern_length; i++)
    {
        *(p_cte_rx_param->antenna_ids + i) = *(p_param->antenna_ids + i);
    }

    if (hci_le_set_conn_cte_rx_param_cmd(p_cte_rx_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }
    mem_free(p_cte_rx_param);

    return status;
}

ble_err_t ble_connection_cte_tx_param_set(ble_connection_cte_tx_param_t *p_param)
{
    ble_err_t status;
    ble_hci_cmd_set_conn_cte_tx_param_t *p_cte_tx_param;
    uint16_t conn_id;
    uint8_t i;

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    p_cte_tx_param = mem_malloc(sizeof(ble_hci_cmd_set_conn_cte_tx_param_t) + p_param->sw_pattern_length);
    p_cte_tx_param->conn_handle = conn_id;
    p_cte_tx_param->cte_types = p_param->cte_types;
    p_cte_tx_param->sw_pattern_length = p_param->sw_pattern_length;
    for (i = 0; i < p_param->sw_pattern_length; i++)
    {
        *(p_cte_tx_param->antenna_ids + i) = *(p_param->antenna_ids + i);
    }

    if (hci_le_set_conn_cte_tx_param_cmd(p_cte_tx_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }
    mem_free(p_cte_tx_param);

    return status;
}

ble_err_t ble_connection_cte_req_set(ble_connection_cte_req_enable_t *p_param)
{
    ble_err_t status;
    ble_hci_cmd_set_conn_cte_req_param_t p_cte_req_param;
    uint16_t conn_id;

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    p_cte_req_param.conn_handle = conn_id;
    p_cte_req_param.enable = p_param->enable;
    p_cte_req_param.cte_req_interval = p_param->cte_req_interval;
    p_cte_req_param.req_cte_length = p_param->req_cte_length;
    p_cte_req_param.req_cte_type = p_param->req_cte_type;

    if (hci_le_conn_cte_req_enable_cmd(&p_cte_req_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}

ble_err_t ble_connection_cte_rsp_set(ble_connection_cte_rsp_enable_t *p_param)
{
    ble_err_t status;
    ble_hci_cmd_set_conn_cte_rsp_param_t p_cte_rsp_param;
    uint16_t conn_id;

    // check host id is valid or not
    if (bhc_host_id_is_valid_check(p_param->host_id) == FALSE)
    {
        return BLE_ERR_INVALID_HOST_ID;
    }

    // check connection exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    p_cte_rsp_param.conn_handle = conn_id;
    p_cte_rsp_param.enable = p_param->enable;

    if (hci_le_conn_cte_rsp_enable_cmd(&p_cte_rsp_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }
    else
    {
        status = BLE_ERR_OK;
    }

    return status;
}

/** BLE gap module event handler.
 *
 */
ble_err_t ble_evt_gap_handler(void *p_param)
{
    ble_err_t status;
    ble_evt_param_t *p_evt_param = (ble_evt_param_t *)p_param;

    status = BLE_ERR_OK;
    switch (p_evt_param->event)
    {
    case BLE_GAP_EVT_SET_RANDOM_ADDR:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] SET ADDR status = %d\n", p_evt_param->event_param.ble_evt_gap.param.evt_set_addr.status);
        }
        break;

#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
    case BLE_GAP_EVT_CONN_COMPLETE:
    {
        ble_evt_gap_conn_complete_t *p_conn_param = (ble_evt_gap_conn_complete_t *)&p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete;

        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
            // update state
            if ((p_conn_param->role == BLE_GAP_ROLE_CENTRAL) && (ble_init_idle_state_check() == FALSE))
            {
                ble_init_status_set(STATE_IDLE);
            }
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] CONN COMPLETE status = %d\n", p_conn_param->status);
        }
    }
    break;

    case BLE_GAP_EVT_CONN_CANCEL:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            // update host id state
            bhc_host_id_state_active_release(BLE_GAP_ROLE_CENTRAL);
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] CANCEL CREATE CONN status = %d\n", p_evt_param->event_param.ble_evt_gap.param.evt_create_conn.status);
        }
        break;

    case BLE_GAP_EVT_CONN_PARAM_UPDATE:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] CONN UPDATE status = %d\n", p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.status);
        }
        break;

    case BLE_GAP_EVT_DISCONN_COMPLETE:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] DISCONN COMPLETE status = %d\n", p_evt_param->event_param.ble_evt_gap.param.evt_disconn_complete.status);
        }
        break;

    case BLE_GAP_EVT_PHY_READ:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] PHY READ status = %d\n", p_evt_param->event_param.ble_evt_gap.param.evt_phy.status);
        }
        break;

    case BLE_GAP_EVT_PHY_UPDATE:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] PHY COMPLETE status = %d\n", p_evt_param->event_param.ble_evt_gap.param.evt_phy.status);
        }
        break;

    case BLE_GAP_EVT_RSSI_READ:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] READ RSSI status = %d\n", p_evt_param->event_param.ble_evt_gap.param.evt_rssi.status);
        }
        break;

    case BLE_GAP_EVT_SET_LE_HOST_CH_CLASSIFICATION:
        break;

    case BLE_GAP_EVT_READ_CHANNEL_MAP:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] READ channel map status = %d\n", p_evt_param->event_param.ble_evt_gap.param.evt_channel_map.status);
        }
        break;

    case BLE_CTE_EVT_SET_CONN_CTE_RX_PARAM:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] Set conn cte rx param status = %d\n", p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_rx_param.status);
        }
        break;

    case BLE_CTE_EVT_SET_CONN_CTE_TX_PARAM:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] Set conn cte tx param status = %d\n", p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_tx_param.status);
        }
        break;

    case BLE_CTE_EVT_SET_CONN_CTE_REQ:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] Set conn cte req status = %d\n", p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_req.status);
        }
        break;

    case BLE_CTE_EVT_SET_CONN_CTE_RSP:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] Set conn cte rsp status = %d\n", p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_rsp.status);
        }
        break;

    case BLE_CTE_EVT_IQ_REPORT:
        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[GAP] IQ Report received\n");
        }
        break;

#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))

    default:
        break;
    }

    return status;
}


/** @file ble_privacy.c
 *
 * @brief Define BLE privacy command definition, structure and functions.
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <string.h>
#include "FreeRTOS.h"
#include "ble_api.h"
#include "ble_privacy_api.h"
#include "ble_att_gatt.h"
#include "ble_printf.h"

#include "ble_scan_api.h"
#include "ble_advertising_api.h"
#include "ble_gap_api.h"

#include "sys_arch.h"
#include "ble_hci.h"
#include "hci_cmd.h"
#include "ble_event_module.h"
#include "host_management.h"

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
static ble_privacy_param_t g_privacy_param = BLE_PRIVACY_PARAM_DISABLE;
static uint8_t g_privacy_host_id = BLE_HOSTID_RESERVED;
/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/
ble_err_t ble_privacy_enable(ble_set_privacy_cfg_t *p_param)
{
    uint8_t cmp[16];
    // check status
    if ((ble_scan_idle_state_check() == FALSE) || (ble_adv_idle_state_check() == FALSE) || (ble_init_idle_state_check() == FALSE))
    {
        return BLE_ERR_INVALID_STATE;
    }

    memset(cmp, 0xFF, 16);
    if (memcmp(LE_Host[p_param->host_id].PeerIRK_M_S_RAND, cmp, 16) == 0)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }
    g_privacy_param |= BLE_PRIVACY_PARAM_ENABLE;
    if (p_param->privacy_mode == NETWORK_PRIVACY_MODE)
    {
        g_privacy_param &= (~BLE_PRIVACY_PARAM_FLD_MODE);
    }
    else
    {
        g_privacy_param |= BLE_PRIVACY_PARAM_MODE_DEVICE;
    }
    g_privacy_host_id = p_param->host_id;

    return BLE_ERR_OK;
}

ble_err_t ble_privacy_disable(void)
{
    // check status
    if ((ble_scan_idle_state_check() == FALSE) || (ble_adv_idle_state_check() == FALSE) || (ble_init_idle_state_check() == FALSE))
    {
        return BLE_ERR_INVALID_STATE;
    }
    g_privacy_param &= (~BLE_PRIVACY_PARAM_FLD_ENABLE);
    g_privacy_host_id = BLE_HOSTID_RESERVED;
    return BLE_ERR_OK;
}


/**
 * TRUE: enable
 * FALSE: disable
 */
bool ble_privacy_parameter_enable_check(void)
{
    if (g_privacy_param & BLE_PRIVACY_PARAM_FLD_ENABLE)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * TRUE: Device Privacy Mode
 * FALSE: Network Privacy Mode
 */
bool ble_privacy_parameter_device_mode_check(void)
{
    if (g_privacy_param & BLE_PRIVACY_PARAM_FLD_MODE)
    {
        return TRUE;
    }

    return FALSE;
}

/**
 * TRUE: LL_privacy enable
 * FALSE: LL_privacy disable
 */
bool ble_privacy_parameter_LL_privacy_on_check(void)
{
    if (g_privacy_param & BLE_PRIVACY_PARAM_FLD_LL_PRIVACY)
    {
        return TRUE;
    }

    return FALSE;
}

uint8_t ble_privacy_host_id_get(void)
{
    return g_privacy_host_id;
}

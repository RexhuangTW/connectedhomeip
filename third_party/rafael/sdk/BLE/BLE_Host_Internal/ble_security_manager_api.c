/** @file ble_security_manager.c
 *
 * @brief
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "lib_config.h"

#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
#include "ble_security_manager_api.h"
#include "sys_arch.h"
#include "ble_hci.h"
#include "task_hci.h"
#include "task_host.h"
#include "ble_event_module.h"
#include "ble_host_cmd.h"
#include "ble_advertising_api.h"
#include "ble_scan_api.h"
#include "ble_gap_api.h"
#include "ble_printf.h"

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/** BLE send security request.
 *
 */
ble_err_t ble_sm_security_request_set(ble_sm_security_request_param_t *p_param)
{
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

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    return bhc_sm_security_request(p_param->host_id);
}


/** Set BLE Pairing PassKey Value
 */
ble_err_t ble_sm_passkey_set(ble_sm_passkey_param_t *p_param)
{
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

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    if (bhc_sm_passkey_set(p_param->passkey, conn_id) == ERR_OK)
    {
        return BLE_ERR_OK;
    }
    else
    {
        return BLE_BUSY;
    }
}


/** Set BLE IO Capabilities
 */
ble_err_t ble_sm_io_capability_set(ble_evt_sm_io_cap_t *p_param)
{
    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check BLE state, initial state and check connection number
#if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
    if ((ble_adv_idle_state_check() != TRUE)    ||
            (bhc_host_connected_link_number_get()  != 0))
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
    if ( (ble_scan_idle_state_check() != TRUE)   ||
            (ble_init_idle_state_check() != TRUE)   ||
            (bhc_host_connected_link_number_get()  != 0))
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))      

    if ((p_param->io_caps_param != DISPLAY_ONLY) &&
            (p_param->io_caps_param != DISPLAY_YESNO) &&
            (p_param->io_caps_param != KEYBOARD_ONLY) &&
            (p_param->io_caps_param != NOINPUT_NOOUTPUT) &&
            (p_param->io_caps_param != KEYBOARD_DISPLAY))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    bhc_sm_io_caps_set(p_param);
    return BLE_ERR_OK;
}


/** Set BLE Bonding Flags
 */
ble_err_t ble_sm_bonding_flag_set(ble_evt_sm_bonding_flag_t *p_param)
{
    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check BLE state, initial state and check connection number
#if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
    if ((ble_adv_idle_state_check() != TRUE)    ||
            (bhc_host_connected_link_number_get()  != 0))
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
    if ( (ble_scan_idle_state_check() != TRUE)   ||
            (ble_init_idle_state_check() != TRUE)   ||
            (bhc_host_connected_link_number_get()  != 0))
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))

    if ((p_param->bonding_flag != NO_BONDING) && (p_param->bonding_flag != BONDING))
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    bhc_sm_bonding_flag_set(p_param);
    return BLE_ERR_OK;
}


/** BLE retoste cccd command
 */
ble_err_t ble_sm_cccd_restore(ble_sm_restore_cccd_param_t *p_param)
{
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

    // check connection is exist or not
    if (bhc_host_id_is_connected_check(p_param->host_id, &conn_id) == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    return bhc_sm_restore_cccd_from_bond(p_param->host_id);
}

/** BLE bond space init command
 */
ble_err_t ble_sm_bonding_space_init(void)
{
    return bhc_sm_bonding_space_init();
}

/** BLE set identity resolving key command
 */
ble_err_t ble_sm_irk_set(ble_sm_irk_param_t *p_param)
{
#if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
    if ((ble_adv_idle_state_check() != TRUE)    ||
            (bhc_host_connected_link_number_get()  != 0))
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
    if ( (ble_scan_idle_state_check() != TRUE)   ||
            (ble_init_idle_state_check() != TRUE)   ||
            (bhc_host_connected_link_number_get()  != 0))
    {
        return BLE_ERR_INVALID_STATE;
    }
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))

    return bhc_sm_identity_resolving_key_set(p_param);
}


/** BLE security module event handler.
 *
 */
ble_err_t ble_evt_sm_handler(void *p_param)
{
    ble_err_t status;
    ble_evt_param_t *p_evt_param = (ble_evt_param_t *)p_param;

    status = BLE_ERR_OK;
    switch (p_evt_param->event)
    {
    case BLE_SM_EVT_STK_GENERATION_METHOD:
    {
        ble_evt_sm_stk_gen_method_t *p_gen_method = (ble_evt_sm_stk_gen_method_t *)&p_evt_param->event_param.ble_evt_sm.param.evt_stk_gen_method;

        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[SM] STK GENERATION METHOD = 0x%02x\n", p_gen_method->key_gen_method);
        }
    }
    break;

    case BLE_SM_EVT_PASSKEY_CONFIRM:
    {
        ble_evt_sm_passkey_confirm_param_t *p_passkey = (ble_evt_sm_passkey_confirm_param_t *)&p_evt_param->event_param.ble_evt_sm.param.evt_passkey_confirm_param;

        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[SM] PASSKEY CONFIRMED host id = %d\n", p_passkey->host_id);
        }
    }
    break;

    case BLE_SM_EVT_AUTH_STATUS:
    {
        ble_evt_sm_auth_status_t *p_auth = (ble_evt_sm_auth_status_t *)&p_evt_param->event_param.ble_evt_sm.param.evt_auth_status;

        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[SM] AUTH CONFIRM status = %d\n", p_auth->status);
        }
    }
    break;

    case BLE_SM_EVT_IRK_RESOLVING_FAIL:
    {
        ble_evt_sm_irk_resolving_fail_t *p_auth = (ble_evt_sm_irk_resolving_fail_t *)&p_evt_param->event_param.ble_evt_sm.param.evt_irk_resolving_fail;

        // post to user
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[SM] Resolving address fail by host id = %d\n", p_auth->host_id);
        }
    }
    break;

    default:
        break;
    }

    return status;
}

#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))


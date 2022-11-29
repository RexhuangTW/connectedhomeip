/** @file ble_event_module.c
 *
 * @brief Handle BLE events from BLE stack.
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "lib_config.h"
#include "ble_event_module.h"
#include "ble_common_api.h"
#include "ble_gap_api.h"
#include "ble_advertising_api.h"
#include "ble_scan_api.h"
#include "ble_att_gatt_api.h"
#include "ble_security_manager_api.h"

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/** Post @ref ble_cmd_evt_t with parameters to the related module handle.
 */
ble_err_t ble_cmd_event_post_to_module(ble_evt_param_t *p_param)
{
    ble_err_t status;
    uint8_t event_type = (p_param->event & 0xF0);

    status = BLE_ERR_OK;
    switch (event_type)
    {
    case BLE_COMMON_EVT_BASE:
        status = ble_evt_common_handler(p_param);
        break;

    case BLE_GAP_EVT_BASE:
        status = ble_evt_gap_handler(p_param);
        if (status == BLE_ERR_OK)
        {
#if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
            if (p_param->event == BLE_GAP_EVT_CONN_COMPLETE)
            {
                // pass connection complete to adv file
                status = ble_evt_adv_handler(p_param);
            }
#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
        }
        break;

    case BLE_CTE_EVT_BASE:
        status = ble_evt_gap_handler(p_param);
        break;

#if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))
    case BLE_ADV_EVT_BASE:
        status = ble_evt_adv_handler(p_param);
        break;
#endif // #if (BLE_MODULE_ENABLE(_ADV_SUPPORT_))

#if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))
    case BLE_SCAN_EVT_BASE:
        status = ble_evt_scan_handler(p_param);
        break;
#endif // #if (BLE_MODULE_ENABLE(_SCAN_SUPPORT_))


#if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))
    case BLE_ATT_GATT_EVT_BASE:
        status = ble_evt_att_gatt_handler(p_param);
        break;
#endif // #if (BLE_MODULE_ENABLE(_CONN_SUPPORT_))     

    case BLE_SM_EVT_BASE:
        status = ble_evt_sm_handler(p_param);
        break;

    case BLE_VENDOR_EVT_BASE:
        status = ble_evt_vendor_handler(p_param);
        break;

    default:
        // do nothing.
        break;
    }

    return status;
}

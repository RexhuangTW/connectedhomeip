/** @file ble_event_app.c
 *
 * @brief Handle BLE events from BLE host.
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "ble_event_app.h"
#include "task_ble_app.h"
#include "ble_api.h"

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/

/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/
ble_err_t ble_event_post_to_notify(uint8_t type, void *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    uint16_t param_len = 0;

    switch (type)
    {
    case BLE_APP_GENERAL_EVENT:
    case BLE_APP_RETURN_PARAMETER_EVENT:
        param_len = sizeof(ble_evt_param_t);
        status = notify_evt_queue_send(type, param_len, p_param);
        break;

    case BLE_APP_SERVICE_EVENT:
        param_len = sizeof(ble_evt_att_param_t) + ((ble_evt_att_param_t *)p_param)->length;
        status = notify_evt_queue_send(BLE_APP_SERVICE_EVENT, param_len, p_param);
        break;

    default:
        break;
    }

    return status;
}

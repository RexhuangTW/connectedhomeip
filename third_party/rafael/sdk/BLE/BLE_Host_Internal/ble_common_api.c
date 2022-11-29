/** @file ble_common.c
 *
 * @brief Define BLE common definition, structure and functions.
 *
 */

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "ble_common_api.h"
#include "ble_gap_api.h"
#include "ble_advertising_api.h"
#include "ble_scan_api.h"
#include "ble_hci.h"
#include "hci_cmd.h"
#include "ble_event_app.h"
#include "ble_event_module.h"
#include "ble_printf.h"
#include "task_hci.h"


/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/


/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/

/** BLE set controller information function.
*/
ble_err_t ble_common_controller_info_set(uint8_t version, uint16_t company_id, uint8_t *p_addr)
{
    ble_err_t status = BLE_ERR_OK;
    ble_hci_vcmd_cntlr_info_param_t p_hci_cmd_parm;

    // set HCI parameters
    p_hci_cmd_parm.ble_version = version;
    p_hci_cmd_parm.ble_company_id = company_id;
    memcpy(p_hci_cmd_parm.ble_public_addr, p_addr, BLE_ADDR_LEN);

    // issue HCI cmd
    if (hci_vendor_set_controller_info_cmd(&p_hci_cmd_parm) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }

    return status;
}


/** BLE common controller initialization.
 *
 */
ble_err_t ble_common_controller_init(void *p_param)
{
    ble_err_t status = BLE_ERR_OK;
    ble_common_controller_info_t *p_info_param = (ble_common_controller_info_t *)p_param;

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // set controller information
    status = ble_common_controller_info_set(p_info_param->version, p_info_param->company_id, (uint8_t *)p_info_param->public_addr);
    if (status != BLE_ERR_OK)
    {
        BLE_PRINTF(DEBUG_ERR, "[COMMON] ble_common_controller_info_set() returns fail %d\n", status);
        return status;
    }

    // set BLE event mask
    if (hci_le_set_event_mask_cmd((uint8_t *)p_info_param->le_event_mask) == ERR_MEM)
    {
        BLE_PRINTF(DEBUG_ERR, "[COMMON] hci_le_set_event_mask_cmd() returns fail.\n");
        return BLE_ERR_SENDTO_FAIL;
    }

    // get local version
    if (hci_read_local_ver_info_cmd() == ERR_MEM)
    {
        BLE_PRINTF(DEBUG_ERR, "[COMMON] ble_read_local_ver_info() returns fail.\n");
        return BLE_ERR_SENDTO_FAIL;
    }

    // get buffer size
    if (hci_le_read_buffer_size_cmd() == ERR_MEM)
    {
        BLE_PRINTF(DEBUG_ERR, "[COMMON] ble_read_buffer_size() returns fail.\n");
        return BLE_ERR_SENDTO_FAIL;
    }

    return BLE_ERR_OK;
}


/** BLE common Read filter accept list size.
 */
ble_err_t ble_common_read_filter_accept_list_size(void)
{
    ble_err_t status = BLE_ERR_OK;

    if (hci_le_read_filter_accept_list_size_cmd() == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }

    return status;
}


/** BLE common Read filter accept list size.
 */
ble_err_t ble_common_clear_filter_accept_list(void)
{
    ble_err_t status;

    status = BLE_ERR_OK;

    if (hci_le_clear_filter_accept_list_cmd() == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }

    return status;
}


/** BLE add device to filter accept list function.
*/
ble_err_t ble_common_add_device_to_filter_accept_list(ble_filter_accept_list_t *p_param)
{
    ble_err_t status;
    ble_filter_accept_list_t *p_list_param = (ble_filter_accept_list_t *)p_param;

    // check init state
    if (ble_init_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check if in advertising state
    if (ble_adv_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check if in scan state
    if (ble_scan_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    status = BLE_ERR_OK;
    if (hci_le_add_device_to_filter_accept_list_cmd((ble_hci_cmd_add_device_to_accept_list_t *)p_list_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }

    return status;
}


ble_err_t ble_common_remove_device_from_filter_accept_list(ble_filter_accept_list_t *p_param)
{
    ble_err_t status;
    ble_filter_accept_list_t *p_list_param = (ble_filter_accept_list_t *)p_param;

    // check init state
    if (ble_init_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    if (p_param == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // check if in advertising state
    if (ble_adv_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    // check if in scan state
    if (ble_scan_idle_state_check() == FALSE)
    {
        return BLE_ERR_INVALID_STATE;
    }

    status = BLE_ERR_OK;
    if (hci_le_remove_device_from_filter_accept_list_cmd((ble_hci_cmd_remove_device_from_accept_list_t *)p_list_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }

    return status;
}

ble_err_t ble_common_read_antenna_info(void)
{
    ble_err_t status;

    status = BLE_ERR_OK;
    if (hci_le_read_antenna_info_cmd() == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }

    return status;
}


/** BLE set scan report function.
*/
ble_err_t ble_vendor_scan_req_report_set(ble_vendor_scan_req_rpt_t *p_param)
{
    ble_vendor_scan_req_rpt_t *p_rpt_param = (ble_vendor_scan_req_rpt_t *)p_param;
    ble_err_t status = BLE_ERR_OK;

    // issue HCI cmd
    if (hci_vendor_set_scan_request_report_cmd((ble_hci_vcmd_scan_req_rpt_param_t *)p_rpt_param) == ERR_MEM)
    {
        status = BLE_ERR_SENDTO_FAIL;
    }

    return status;
}


/** BLE common module event handler.
 *
 */
ble_err_t ble_evt_common_handler(void *p_param)
{
    ble_err_t status;
    ble_evt_param_t *p_evt_param = (ble_evt_param_t *)p_param;

    status = BLE_ERR_OK;
    switch (p_evt_param->event)
    {
    case BLE_COMMON_EVT_SET_CONTROLLER_INFO:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] SET CONTLR INFO status = %d\n", p_evt_param->event_param.ble_evt_common.param.evt_set_cntlr_info.status);
        }
        break;

    case BLE_COMMON_EVT_SET_EVENT_MASK:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] SET EVENT MASK status = %d\n", p_evt_param->event_param.ble_evt_common.param.evt_set_event_mask.status);
        }
        break;

    case BLE_COMMON_EVT_READ_LOCAL_VER:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] READ VER status = %d, hci_revision = %d\n", p_evt_param->event_param.ble_evt_common.param.evt_read_local_ver.status, p_evt_param->event_param.ble_evt_common.param.evt_read_local_ver.hci_revision);
        }
        break;

    case BLE_COMMON_EVT_READ_BUFFER_SIZE:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] READ BUFFER status = %d size = %d \n", p_evt_param->event_param.ble_evt_common.param.evt_read_buffer_size.status, p_evt_param->event_param.ble_evt_common.param.evt_read_buffer_size.total_num_data_packet);
        }
        break;

    case BLE_COMMON_EVT_READ_FILTER_ACCEPT_LIST_SIZE:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] READ Filter accept list size status = %d size = %d \n", p_evt_param->event_param.ble_evt_common.param.evt_read_accept_list_size.status, p_evt_param->event_param.ble_evt_common.param.evt_read_accept_list_size.filter_accept_list_size);
        }
        break;

    case BLE_COMMON_EVT_CLEAR_FILTER_ACCEPT_LIST:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] Clear Filter accept list status = %d\n", p_evt_param->event_param.ble_evt_common.param.evt_clear_accept_list.status);
        }
        break;

    case BLE_COMMON_EVT_ADD_FILTER_ACCEPT_LIST:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] Add Filter accept list status = %d\n", p_evt_param->event_param.ble_evt_common.param.evt_add_accept_list.status);
        }
        break;

    case BLE_COMMON_EVT_REMOVE_FILTER_ACCEPT_LIST:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] Remove Filter accept list status = %d\n", p_evt_param->event_param.ble_evt_common.param.evt_remove_accept_list.status);
        }
        break;

    case BLE_COMMON_EVT_READ_ANTENNA_INFO:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        if (status == BLE_ERR_OK)
        {
            BLE_PRINTF(BLE_DEBUG_LOG, "[COMMON] Read antenna info status = %d\n", p_evt_param->event_param.ble_evt_common.param.evt_read_antenna_info.status);
        }
        break;

    default:
        break;
    }

    return status;
}

/** BLE common module event handler.
 *
 */
ble_err_t ble_evt_vendor_handler(void *p_param)
{
    ble_err_t status;
    ble_evt_param_t *p_evt_param = (ble_evt_param_t *)p_param;

    status = BLE_ERR_OK;
    switch (p_evt_param->event)
    {
    case BLE_VENDOR_EVT_SCAN_REQ_REPORT:
        status = ble_event_post_to_notify(BLE_APP_GENERAL_EVENT, p_evt_param);
        break;

    default:
        break;
    }

    return status;
}

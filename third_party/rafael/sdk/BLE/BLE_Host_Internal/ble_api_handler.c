
#include <stdio.h>
#include "ble_api.h"
#include "ble_common_api.h"
#include "ble_gap_api.h"
#include "ble_advertising_api.h"
#include "ble_scan_api.h"
#include "ble_att_gatt_api.h"
#include "ble_security_manager_api.h"
#include "ble_privacy_api.h"

ble_err_t prcss_api_cmd_transport(uint16_t type, void *p_param)
{
    ble_err_t status = BLE_ERR_OK;

    switch (type)
    {
    //====================================================================
    //     BLE COMMON API
    //====================================================================
    case TYPE_BLE_COMMON_CONTROLLER_INIT:
        status = ble_common_controller_init(p_param);
        break;

    case TYPE_BLE_COMMON_READ_FILTER_ACCEPT_LIST_SIZE:
        status = ble_common_read_filter_accept_list_size();
        break;

    case TYPE_BLE_COMMON_CLEAR_FILTER_ACCEPT_LIST:
        status = ble_common_clear_filter_accept_list();
        break;

    case TYPE_BLE_COMMON_ADD_DEVICE_TO_FILTER_ACCEPT_LIST:
        status = ble_common_add_device_to_filter_accept_list((ble_filter_accept_list_t *)p_param);
        break;

    case TYPE_BLE_COMMON_REMOVE_DEVICE_FROM_FILTER_ACCEPT_LIST:
        status = ble_common_remove_device_from_filter_accept_list((ble_filter_accept_list_t *)p_param);
        break;

    case TYPE_BLE_COMMON_ANTENNA_INFO_READ:
        status = ble_common_read_antenna_info();
        break;

    //====================================================================
    //     BLE GAP API
    //====================================================================
    case TYPE_BLE_GAP_DEVICE_ADDR_GET:
    {
        ble_gap_get_addr_t *p_target_addr = (ble_gap_get_addr_t *)p_param;

        status = ble_gap_device_address_get((ble_gap_addr_t *)p_target_addr->p_addr_param);
    }
    break;

    case TYPE_BLE_GAP_DEVICE_ADDR_SET:
        status = ble_gap_device_address_set((ble_gap_addr_t *)p_param);
        break;

    case TYPE_BLE_GAP_CONNECTION_UPDATE:
        status = ble_gap_connection_update((ble_gap_conn_param_update_param_t *)p_param);
        break;

    case TYPE_BLE_GAP_CONNECTION_CREATE:
        status = ble_gap_connection_create((ble_gap_create_conn_param_t *)p_param);
        break;

    case TYPE_BLE_GAP_CONNECTION_CANCEL:
        status = ble_gap_connection_cancel();
        break;

    case TYPE_BLE_GAP_CONNECTION_TERMINATE:
        status = ble_gap_conn_terminate((ble_gap_conn_terminate_param_t *)p_param);
        break;

    case TYPE_BLE_GAP_PHY_UPDATE:
        status = ble_gap_phy_update((ble_gap_phy_update_param_t *)p_param);
        break;

    case TYPE_BLE_GAP_PHY_READ:
        status = ble_gap_phy_read((ble_gap_phy_read_param_t *)p_param);
        break;

    case TYPE_BLE_GAP_RSSI_READ:
        status = ble_gap_rssi_read((ble_gap_rssi_read_param_t *)p_param);
        break;

    case TYPE_BLE_GAP_HOST_CHANNEL_CLASSIFICATION_SET:
        status = ble_gap_host_channel_classification_set((ble_gap_host_ch_classif_t *)p_param);
        break;

    case TYPE_BLE_CHANNEL_MAP_READ:
        status = ble_gap_channel_map_read((ble_gap_channel_map_read_t *)p_param);
        break;

    case TYPE_BLE_RESOLVABLE_ADDR_INIT:
        status = ble_resolvable_address_init();
        break;

    case TYPE_BLE_GAP_REGEN_RESOLVABLE_ADDRESS:
        status = ble_regenerate_resolvable_address((ble_gap_regen_resol_addr_t *)p_param);
        break;

    //====================================================================
    //     BLE ADV API
    //====================================================================
    case TYPE_BLE_ADV_PARAMETER_SET:
        status = ble_adv_param_set((ble_adv_param_t *)p_param);
        break;

    case TYPE_BLE_ADV_DATA_SET:
        status =  ble_adv_data_set((ble_adv_data_param_t *)p_param);
        break;

    case TYPE_BLE_ADV_SCAN_RSP_SET:
        status =  ble_adv_scan_rsp_set((ble_adv_data_param_t *)p_param);
        break;

    case TYPE_BLE_ADV_ENABLE:
        status = ble_adv_enable((ble_adv_enable_param_t *)p_param);
        break;

    case TYPE_BLE_ADV_DISABLE:
        status = ble_adv_disable();
        break;

    //====================================================================
    //     BLE SCAN API
    //====================================================================
    case TYPE_BLE_SCAN_PARAMETER_SET:
        status = ble_scan_param_set((ble_scan_param_t *)p_param);
        break;

    case TYPE_BLE_SCAN_ENABLE:
        status = ble_scan_enable();
        break;

    case TYPE_BLE_SCAN_DISABLE:
        status = ble_scan_disable();
        break;

    //====================================================================
    //     BLE GATT API
    //====================================================================
    case TYPE_BLE_GATT_PREFERRED_DATA_LENGTH_SET:
        status = ble_gatt_suggested_data_length_set((ble_gatt_suggested_data_len_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_PREFERRED_MTU_SET:
        status = ble_gatt_preferred_mtu_set((ble_gatt_mtu_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_EXCHANGE_MTU_REQ:
        status = ble_gatt_exchange_mtu_req((ble_gatt_mtu_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_DATA_LENGTH_UPDATE:
        status = ble_gatt_data_length_update((ble_gatt_data_len_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_MTU_GET:
        status = ble_gatt_mtu_get((ble_gatt_get_mtu_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_ATT_HANDLE_MAPPING_GET:
        status = ble_gatt_att_handle_mapping_get((ble_gatt_handle_table_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_READ_RSP:
        status = ble_gatt_read_rsp((ble_gatt_data_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_READ_BY_TYPE_RSP:
        status = ble_gatt_read_by_type_rsp((ble_gatt_data_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_READ_BLOB_RSP:
        status = ble_gatt_read_blob_rsp((ble_gatt_data_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_ERROR_RSP:
        status = ble_gatt_error_rsp((ble_gatt_err_rsp_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_NOTIFICATION:
        status = ble_gatt_notification((ble_gatt_data_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_INDICATION:
        status = ble_gatt_indication((ble_gatt_data_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_WRITE_REQ:
        status = ble_gatt_write_req((ble_gatt_data_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_WRITE_CMD:
        status = ble_gatt_write_cmd((ble_gatt_data_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_READ_REQ:
        status = ble_gatt_read_req((ble_gatt_read_req_param_t *)p_param);
        break;

    case TYPE_BLE_GATT_READ_BLOB_REQ:
        status = ble_gatt_read_blob_req((ble_gatt_read_blob_req_param_t *)p_param);
        break;

    //====================================================================
    //     BLE SM API
    //====================================================================
    case TYPE_BLE_SM_BONDING_FLAG_SET:
        status = ble_sm_bonding_flag_set((ble_evt_sm_bonding_flag_t *)p_param);
        break;

    case TYPE_BLE_SM_CCCD_RESTORE:
        status = ble_sm_cccd_restore((ble_sm_restore_cccd_param_t *)p_param);
        break;

    case TYPE_BLE_SM_IO_CAPABILITY_SET:
        status = ble_sm_io_capability_set((ble_evt_sm_io_cap_t *)p_param);
        break;

    case TYPE_BLE_SM_PASSKEY_SET:
        status = ble_sm_passkey_set((ble_sm_passkey_param_t *)p_param);
        break;

    case TYPE_BLE_SM_SECURITY_REQ_SET:
        status = ble_sm_security_request_set((ble_sm_security_request_param_t *)p_param);
        break;

    case TYPE_BLE_SM_BOND_SPACE_INIT:
        status = ble_sm_bonding_space_init();
        break;

    case TYPE_BLE_SM_IDENTITY_RESOLVING_KEY_SET:
        status = ble_sm_irk_set((ble_sm_irk_param_t *)p_param);
        break;

    //====================================================================
    //     Vendor API
    //====================================================================
    case TYPE_BLE_VENDOR_SCAN_REQUEST_REPORT:
        status = ble_vendor_scan_req_report_set((ble_vendor_scan_req_rpt_t *)p_param);
        break;

    //====================================================================
    //     Privacy API
    //====================================================================
    case TYPE_BLE_PRIVACY_ENABLE:
        status = ble_privacy_enable((ble_set_privacy_cfg_t *)p_param);
        break;

    case TYPE_BLE_PRIVACY_DISABLE:
        status = ble_privacy_disable();
        break;

    //====================================================================
    //     CONN CTE API
    //====================================================================
    case TYPE_BLE_CTE_CONNECTION_CTE_RX_PARAMETERS_SET:
        status = ble_connection_cte_rx_param_set((ble_connection_cte_rx_param_t *)p_param);
        break;

    case TYPE_BLE_CTE_CONNECTION_CTE_TX_PARAMETERS_SET:
        status = ble_connection_cte_tx_param_set((ble_connection_cte_tx_param_t *)p_param);
        break;

    case TYPE_BLE_CTE_CONNECTION_CTE_REQ_SET:
        status = ble_connection_cte_req_set((ble_connection_cte_req_enable_t *)p_param);
        break;

    case TYPE_BLE_CTE_CONNECTION_CTE_RSP_SET:
        status = ble_connection_cte_rsp_set((ble_connection_cte_rsp_enable_t *)p_param);
        break;

    //====================================================================
    //     UNKNOWN API
    //====================================================================
    default:
        status = BLE_ERR_CMD_NOT_SUPPORTED;
        break;
    }

    return status;
};

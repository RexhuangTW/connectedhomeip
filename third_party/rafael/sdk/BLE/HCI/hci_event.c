/** @file hci_event.c
 *
 * @brief
 *
 */
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdint.h>
#include "sys_arch.h"
#include "mem_mgmt.h"
#include "ble_printf.h"
#include "task_host.h"
#include "task_ble_app.h"
#include "ble_hci.h"
#include "ble_event_module.h"
#include "ble_host_cmd.h"
#include "ble_bonding.h"
#include "ble_api.h"
#include "host_management.h"
#include "ble_privacy_api.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define GET_OPCODE(ogf, ocf) ((ogf<<10)|ocf)


/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
static int8_t hci_le_null_event(uint8_t *p_data)
{
    BLE_PRINTF(BLE_DEBUG_LOG, "[BLE_DEBUG_LOG] unsupport event rcvd\n");
    return ERR_OK;
}

static int8_t hci_le_connection_complete_evnet(uint8_t *p_data)
{
    ble_hci_le_meta_evt_param_conn_complete_t *p_conn_complete;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;
    uint8_t host_id;

    p_conn_complete = (ble_hci_le_meta_evt_param_conn_complete_t *)p_data;
    // init event data
    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
    if (p_queue_param != NULL)
    {
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        memset(&p_evt_param->event_param, 0, sizeof(ble_evt_gap_conn_complete_t));
        p_evt_param->event = BLE_GAP_EVT_CONN_COMPLETE;
        p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.status = p_conn_complete->status;

        switch (p_conn_complete->status)
        {
        case BLE_HCI_ERR_CODE_SUCCESS:
            /* handle connection completed. */
            bhc_connection_complete_handle(p_conn_complete);
            host_id = bhc_query_host_id_by_conn_id(p_conn_complete->conn_handle);

            // event data
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.host_id = host_id;
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.role = p_conn_complete->role;
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.peer_addr.addr_type = p_conn_complete->peer_addr_type;
            memcpy(p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.peer_addr.addr, p_conn_complete->peer_addr, BLE_ADDR_LEN);
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.conn_interval = p_conn_complete->conn_interval;
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.periph_latency = p_conn_complete->periph_latency;
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.supv_timeout = p_conn_complete->supv_timeout;

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            break;

        case BLE_HCI_ERR_CODE_UNKNOWN_CONNECTION_IDENTIFIER:
            mem_free(p_queue_param);
            /* handle "central" create connection cancelation completed event. */
            bhc_host_id_state_active_release(BLE_GAP_ROLE_CENTRAL);
            break;

        case BLE_HCI_ERR_CODE_DIRECTED_ADVERTISING_TIMEOUT:
            /* handle "peripheral" direct advertising failed event. */
            host_id = bhc_host_id_state_active_get(BLE_GAP_ROLE_PERIPHERAL);
            bhc_host_id_state_active_release(BLE_GAP_ROLE_PERIPHERAL);
            // event data
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_complete.host_id = host_id;
            bhc_host_param_init(host_id);

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            break;

        default:
            mem_free(p_queue_param);
            break;
        }

        return ERR_OK;
    }

    return ERR_MEM;
}

static int8_t hci_le_advertising_report_event(uint8_t *p_data)
{
    ble_hci_le_meta_evt_param_adv_report_t *p_adv_report;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;

    p_adv_report = (ble_hci_le_meta_evt_param_adv_report_t *)p_data;

    if (bhc_host_privacy_private_addr_scan_check((ble_gap_addr_t *)&p_adv_report->addr_type0) != TRUE)
    {
        return ERR_OK;
    }

    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
    if (p_queue_param != NULL)
    {
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        p_evt_param->event = BLE_SCAN_EVT_ADV_REPORT;
        p_evt_param->event_param.ble_evt_scan.param.evt_adv_report.adv_type = p_adv_report->event_type0;
        p_evt_param->event_param.ble_evt_scan.param.evt_adv_report.peer_addr.addr_type = p_adv_report->addr_type0;
        memcpy(p_evt_param->event_param.ble_evt_scan.param.evt_adv_report.peer_addr.addr, p_adv_report->addr0, BLE_ADDR_LEN);
        p_evt_param->event_param.ble_evt_scan.param.evt_adv_report.length = p_adv_report->length0;
        if (p_adv_report->length0 >= 0x1F)
        {
            p_adv_report->length0 = 0x1F;
        }
        memcpy(p_evt_param->event_param.ble_evt_scan.param.evt_adv_report.data, p_adv_report->data0, p_adv_report->length0);
        p_evt_param->event_param.ble_evt_scan.param.evt_adv_report.rssi = p_adv_report->data0[p_adv_report->length0];

        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
        return ERR_OK;
    }

    return ERR_MEM;
}

static int8_t hci_le_connection_update_complete_event(uint8_t *p_data)
{
    ble_hci_le_meta_evt_param_conn_update_t *p_conn_updata_complete;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;
    uint8_t host_id;

    p_conn_updata_complete = (ble_hci_le_meta_evt_param_conn_update_t *)p_data;
    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
    if (p_queue_param != NULL)
    {
        bhc_connection_update_handle(p_conn_updata_complete);

        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        memset(&p_evt_param->event_param, 0, sizeof(ble_evt_gap_conn_param_update_t));
        p_evt_param->event = BLE_GAP_EVT_CONN_PARAM_UPDATE;

        host_id = bhc_query_host_id_by_conn_id(p_conn_updata_complete->conn_handle);
        p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.host_id = host_id;
        p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.status = p_conn_updata_complete->status;

        if (p_conn_updata_complete->status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.conn_interval = p_conn_updata_complete->conn_interval;
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.periph_latency = p_conn_updata_complete->periph_latency;
            p_evt_param->event_param.ble_evt_gap.param.evt_conn_param_update.supv_timeout = p_conn_updata_complete->supv_timeout;
        }

        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

        if (bhc_timer_evt_get(host_id, TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP) == TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP)
        {
            bhc_timer_clear(host_id, TIMER_EVENT_CONN_PARAMETER_UPDATE_RSP);
        }
        if (bhc_timer_evt_get(host_id, TIMER_EVENT_CONN_UPDATE_COMPLETE) == TIMER_EVENT_CONN_UPDATE_COMPLETE)
        {
            bhc_timer_clear(host_id, TIMER_EVENT_CONN_UPDATE_COMPLETE);
        }

        return ERR_OK;
    }

    return ERR_MEM;
}

static int8_t hci_le_read_remote_features_complete_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_long_term_key_request_event(uint8_t *p_data)
{
    return bhc_smp_long_term_key_req((ble_hci_le_meta_evt_param_long_term_key_req_t *)p_data);
}

static int8_t hci_le_remote_connection_parameter_request_event(uint8_t *p_data)
{
    ble_hci_le_meta_evt_param_conn_para_req_t *p_para_req;

    p_para_req = (ble_hci_le_meta_evt_param_conn_para_req_t *)p_data;
    // check parameters
    if ( (p_para_req->min_interval < BLE_CONN_INTERVAL_MIN || p_para_req->min_interval > BLE_CONN_INTERVAL_MAX) ||
            (p_para_req->max_interval < BLE_CONN_INTERVAL_MIN || p_para_req->max_interval > BLE_CONN_INTERVAL_MAX) ||
            (p_para_req->min_interval > p_para_req->max_interval) ||
            (p_para_req->max_latency > BLE_CONN_LATENCY_MAX) ||
            (p_para_req->timeout < BLE_CONN_SUPV_TIMEOUT_MIN || p_para_req->timeout > BLE_CONN_SUPV_TIMEOUT_MAX) ||
            ((p_para_req->timeout * 4) <= ((1 + p_para_req->max_latency) * p_para_req->max_interval )))
    {
        ble_hci_cmd_le_remote_conn_param_req_neg_reply_param_t p_neg_para;

        p_neg_para.conn_handle = p_para_req->conn_handle;
        p_neg_para.reason = 0x3B;

        return bhc_le_remote_conn_parameter_req_neg_handle(&p_neg_para);
    }

    return bhc_le_remote_conn_parameter_req_handle((ble_hci_le_meta_evt_param_conn_para_req_t *)p_data);
}

static int8_t hci_le_data_length_change_event(uint8_t *p_data)
{
    ble_hci_le_meta_evt_param_data_length_change_t *p_data_len_change;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;
    uint8_t host_id;

    p_data_len_change = (ble_hci_le_meta_evt_param_data_length_change_t *)p_data;
    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
    if (p_queue_param != NULL)
    {
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        p_evt_param->event = BLE_ATT_GATT_EVT_DATA_LENGTH_CHANGE;
        host_id = bhc_query_host_id_by_conn_id(p_data_len_change->conn_handle);
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_data_length_change.host_id = host_id;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_data_length_change.max_tx_octets = p_data_len_change->max_tx_octets;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_data_length_change.max_tx_time = p_data_len_change->max_tx_time;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_data_length_change.max_rx_octets = p_data_len_change->max_rx_octets;
        p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_data_length_change.max_rx_time = p_data_len_change->max_rx_time;

        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
        return ERR_OK;
    }

    return ERR_MEM;
}

static int8_t hci_le_read_local_p256_public_key_complete_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_generate_dhkey_complete_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_enhanced_connection_complete_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_directed_advertising_report_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_phy_update_complete_event(uint8_t *p_data)
{
    ble_hci_le_meta_evt_param_phy_update_complete_t *p_phy_update;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;
    uint8_t host_id;

    p_phy_update = (ble_hci_le_meta_evt_param_phy_update_complete_t *)p_data;
    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
    if (p_queue_param != NULL)
    {
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        memset(&p_evt_param->event_param, 0, sizeof(ble_evt_gap_phy_t));
        p_evt_param->event = BLE_GAP_EVT_PHY_UPDATE;
        host_id = bhc_query_host_id_by_conn_id(p_phy_update->conn_handle);
        p_evt_param->event_param.ble_evt_gap.param.evt_phy.host_id = host_id;
        p_evt_param->event_param.ble_evt_gap.param.evt_phy.status = p_phy_update->status;

        if (p_phy_update->status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            p_evt_param->event_param.ble_evt_gap.param.evt_phy.tx_phy = p_phy_update->tx_phy;
            p_evt_param->event_param.ble_evt_gap.param.evt_phy.rx_phy = p_phy_update->rx_phy;
        }
        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
        return ERR_OK;
    }

    return ERR_MEM;
}

static int8_t hci_le_extended_advertising_report_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_periodic_advertising_sync_established_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_periodic_advertising_report_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_periodic_advertising_sync_lost_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_scan_timeout_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_advertising_set_terminated_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_scan_request_received_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_channel_selection_algorithm_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_connectionless_iq_report_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_connection_iq_report_event(uint8_t *p_data)
{
    ble_hci_le_meta_evt_param_conn_iq_report_t *p_iq_report;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;
    uint8_t host_id, i;

    p_iq_report = (ble_hci_le_meta_evt_param_conn_iq_report_t *)p_data;
    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));

    if ((sizeof(ble_hci_le_meta_evt_param_conn_iq_report_t) + (p_iq_report->sample_cnt * 2)) > sizeof(ble_evt_param_t))
    {
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "Sample cnt bigger than event max size\n");
        return ERR_OK;
    }
    if (p_queue_param != NULL)
    {
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        p_evt_param->event = BLE_CTE_EVT_IQ_REPORT;
        host_id = bhc_query_host_id_by_conn_id(p_iq_report->conn_handle);
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.host_id = host_id;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.rx_phy = p_iq_report->rx_phy;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.data_ch_idx = p_iq_report->data_ch_idx;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.rssi = p_iq_report->rssi;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.rssi_antenna_id = p_iq_report->rssi_antenna_id;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.cte_type = p_iq_report->cte_type;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.slot_durations = p_iq_report->slot_durations;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.packet_status = p_iq_report->packet_status;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.conn_evt_cnt = p_iq_report->conn_evt_cnt;
        p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.sample_cnt = p_iq_report->sample_cnt;

        for (i = 0; i < p_iq_report->sample_cnt * 2; i++)
        {
            *(p_evt_param->event_param.ble_evt_cte.param.evt_conn_iq_report.sample_iq + i) = *(p_iq_report->sample_iq + i);
        }

        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
        return ERR_OK;
    }

    return ERR_MEM;
}

static int8_t hci_le_cte_request_failed_event(uint8_t *p_data)
{
    ble_hci_le_meta_evt_param_cte_req_failed_t *p_cte_req_failed;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;
    uint8_t host_id;

    p_cte_req_failed = (ble_hci_le_meta_evt_param_cte_req_failed_t *)p_data;
    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
    if (p_queue_param != NULL)
    {
        p_queue_param->u32_send_systick = sys_now();
        p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

        p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
        memset(&p_evt_param->event_param, 0, sizeof(ble_hci_le_meta_evt_param_cte_req_failed_t));
        p_evt_param->event = BLE_CTE_EVT_CTE_REQ_FAILED;
        host_id = bhc_query_host_id_by_conn_id(p_cte_req_failed->conn_handle);
        p_evt_param->event_param.ble_evt_cte.param.evt_cte_req_failed.host_id = host_id;
        p_evt_param->event_param.ble_evt_cte.param.evt_cte_req_failed.status = p_cte_req_failed->status;

        task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
        return ERR_OK;
    }

    return ERR_MEM;
}

static int8_t hci_le_periodic_advertising_sync_transfer_received_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_cis_established_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_cis_request_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_create_big_complete_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_terminate_big_complete_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_big_sync_established_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_big_sync_lost_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_requst_peer_sca_complete_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_path_loss_threshold_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_transmit_power_reporting_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_biginfo_advertising_report_event(uint8_t *p_data)
{
    return ERR_OK;
}

static int8_t hci_le_subrate_change_event(uint8_t *p_data)
{
    return ERR_OK;
}

int8_t (* const prcss_le_meta_event[])(uint8_t *p_data) =
{
    hci_le_null_event,                                          /*sub event code 0x00*/
    hci_le_connection_complete_evnet,                           /*sub event code 0x01*/
    hci_le_advertising_report_event,                            /*sub event code 0x02*/
    hci_le_connection_update_complete_event,                    /*sub event code 0x03*/
    hci_le_read_remote_features_complete_event,                 /*sub event code 0x04*/
    hci_le_long_term_key_request_event,                         /*sub event code 0x05*/
    hci_le_remote_connection_parameter_request_event,           /*sub event code 0x06*/
    hci_le_data_length_change_event,                            /*sub event code 0x07*/
    hci_le_read_local_p256_public_key_complete_event,           /*sub event code 0x08*/
    hci_le_generate_dhkey_complete_event,                       /*sub event code 0x09*/
    hci_le_enhanced_connection_complete_event,                  /*sub event code 0x0A*/
    hci_le_directed_advertising_report_event,                   /*sub event code 0x0B*/
    hci_le_phy_update_complete_event,                           /*sub event code 0x0C*/
    hci_le_extended_advertising_report_event,                   /*sub event code 0x0D*/
    hci_le_periodic_advertising_sync_established_event,         /*sub event code 0x0E*/
    hci_le_periodic_advertising_report_event,                   /*sub event code 0x0F*/
    hci_le_periodic_advertising_sync_lost_event,                /*sub event code 0x10*/
    hci_le_scan_timeout_event,                                  /*sub event code 0x11*/
    hci_le_advertising_set_terminated_event,                    /*sub event code 0x12*/
    hci_le_scan_request_received_event,                         /*sub event code 0x13*/
    hci_le_channel_selection_algorithm_event,                   /*sub event code 0x14*/
    hci_le_connectionless_iq_report_event,                      /*sub event code 0x15*/
    hci_le_connection_iq_report_event,                          /*sub event code 0x16*/
    hci_le_cte_request_failed_event,                            /*sub event code 0x17*/
    hci_le_periodic_advertising_sync_transfer_received_event,   /*sub event code 0x18*/
    hci_le_cis_established_event,                               /*sub event code 0x19*/
    hci_le_cis_request_event,                                   /*sub event code 0x1A*/
    hci_le_create_big_complete_event,                           /*sub event code 0x1B*/
    hci_le_terminate_big_complete_event,                        /*sub event code 0x1C*/
    hci_le_big_sync_established_event,                          /*sub event code 0x1D*/
    hci_le_big_sync_lost_event,                                 /*sub event code 0x1E*/
    hci_le_requst_peer_sca_complete_event,                      /*sub event code 0x1F*/
    hci_le_path_loss_threshold_event,                           /*sub event code 0x20*/
    hci_le_transmit_power_reporting_event,                      /*sub event code 0x21*/
    hci_le_biginfo_advertising_report_event,                    /*sub event code 0x22*/
    hci_le_subrate_change_event,                                /*sub event code 0x23*/
};

static int8_t hci_null_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_inquiry_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_inquiry_result_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_connection_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_connection_request_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_disconnection_complete_event(uint8_t *p_data, uint8_t length)
{
    ble_hci_evt_param_disconn_complete_t *p_dis_complete;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;
    uint8_t host_id;

    p_dis_complete = (ble_hci_evt_param_disconn_complete_t *)p_data;
    host_id = bhc_query_host_id_by_conn_id(p_dis_complete->conn_handle);

    p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
    if (p_queue_param != NULL)
    {
        if (bhc_disconnect_handle(p_dis_complete->conn_handle) == ERR_OK)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            memset(&p_evt_param->event_param, 0, sizeof(ble_evt_gap_disconn_complete_t));
            p_evt_param->event = BLE_GAP_EVT_DISCONN_COMPLETE;
            p_evt_param->event_param.ble_evt_gap.param.evt_disconn_complete.host_id = host_id;
            p_evt_param->event_param.ble_evt_gap.param.evt_disconn_complete.status = p_dis_complete->status;
            p_evt_param->event_param.ble_evt_gap.param.evt_disconn_complete.reason = p_dis_complete->reason;
            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);

            bhc_host_release_db_parsing_resource(host_id);
            bhc_att_queue_write_resource(host_id);
            bhc_host_param_init(host_id);
            bhc_host_gen_resolvable_address(host_id);
            return ERR_OK;
        }
        else
        {
            mem_free(p_queue_param);
            return ERR_MEM;
        }
    }

    return ERR_MEM;
}

static int8_t hci_authentication_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_remote_name_request_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_encryption_change_event(uint8_t *p_data, uint8_t length)
{
    bhc_prcss_encrpytion_change((ble_hci_evt_param_encrypt_change_t *)p_data);

    return ERR_OK;
}

static int8_t hci_change_connection_link_key_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_link_key_type_changed_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_read_remote_supported_features_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_read_remote_version_information_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_qos_setup_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}


static int8_t hci_command_complete_event(uint8_t *p_data, uint8_t length)
{
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;
    ble_hci_evt_param_cmd_complete_t *p_cmd_cmp;
    uint16_t opcode;
    int8_t status;

    p_cmd_cmp = (ble_hci_evt_param_cmd_complete_t *)p_data;
    opcode = (p_cmd_cmp->ocf | (p_cmd_cmp->ogf << 8));
    status = ERR_OK;

    // Parsing Completed Event
    switch (opcode)
    {
    /* ==================================================
     *  Information Parameters: OGF = 0x04
     * ================================================== */
    case GET_OPCODE(HCI_CMD_INFO_PARAM, INFO_PARAM_READ_LOCAL_VERSION_INFO):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_READ_LOCAL_VER;
            memcpy(&p_evt_param->event_param.ble_evt_common.param.evt_read_local_ver, &p_cmd_cmp->para.read_local_ver, sizeof(ble_hci_return_param_read_local_ver_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] controller ver: %d\n", p_cmd_cmp->para.read_local_ver.hci_revision);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_INFO_PARAM, INFO_PARAM_READ_LOCAL_SUPPORTED_CMDS):
        break;

    case GET_OPCODE(HCI_CMD_INFO_PARAM, INFO_PARAM_READ_LOCAL_SUPPORTED_FETAUTES):
        break;

    case GET_OPCODE(HCI_CMD_INFO_PARAM, INFO_PARAM_READ_BD_ADDR):
        break;

    /* ==================================================
     *  Status Parameter: OGF = 0x05
     * ================================================== */
    case GET_OPCODE(HCI_CMD_STATUS_PARAM, STATUS_PARAM_READ_RSSI):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_GAP_EVT_RSSI_READ;
            p_evt_param->event_param.ble_evt_gap.param.evt_rssi.host_id = bhc_query_host_id_by_conn_id(p_cmd_cmp->para.read_rssi.handle);
            p_evt_param->event_param.ble_evt_gap.param.evt_rssi.status = p_cmd_cmp->para.read_rssi.status;
            p_evt_param->event_param.ble_evt_gap.param.evt_rssi.rssi = p_cmd_cmp->para.read_rssi.rssi;

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] read RSSI status %d\n", p_cmd_cmp->para.read_rssi.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    /* ==================================================
     *  LE Controller Command: OGF = 0x08
     * ================================================== */
    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_EVENT_MASK):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_SET_EVENT_MASK;
            memcpy(&p_evt_param->event_param.ble_evt_common.param.evt_set_event_mask, &p_cmd_cmp->para.set_event_mask, sizeof(ble_hci_return_param_set_event_mask_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] set event mask %d\n", p_cmd_cmp->para.set_event_mask.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_READ_BUFFER_SIZE):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_READ_BUFFER_SIZE;
            memcpy(&p_evt_param->event_param.ble_evt_common.param.evt_read_buffer_size, &p_cmd_cmp->para.read_buffer_size, sizeof(ble_hci_return_param_read_buffer_size_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            host_notify_acl_data_complete(p_cmd_cmp->para.read_buffer_size.total_num_data_packet);

            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] buf num %d\n", p_cmd_cmp->para.read_buffer_size.total_num_data_packet);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_READ_LOCAL_SUPPORTED_FEATURES):
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_RANDOM_DEVICE_ADDRESS):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_GAP_EVT_SET_RANDOM_ADDR;
            memcpy(&p_evt_param->event_param.ble_evt_gap.param.evt_set_addr, &p_cmd_cmp->para.set_random_addr, sizeof(ble_hci_return_param_set_random_addr_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] rand addr status %d\n", p_cmd_cmp->para.set_random_addr.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_ADVERTISING_PARAMETERS):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_ADV_EVT_SET_PARAM;
            memcpy(&p_evt_param->event_param.ble_evt_adv.param.evt_set_adv_param, &p_cmd_cmp->para.set_adv_param, sizeof(ble_hci_return_param_set_adv_param_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] ADV param status %d\n", p_cmd_cmp->para.set_adv_param.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_ADVERTISING_DATA):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_ADV_EVT_SET_DATA;
            memcpy(&p_evt_param->event_param.ble_evt_adv.param.evt_set_adv_data, &p_cmd_cmp->para.set_adv_data, sizeof(ble_hci_return_param_set_adv_data_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] ADV data status %d\n", p_cmd_cmp->para.set_adv_data.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_SCAN_RSP_DATA):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_ADV_EVT_SET_SCAN_RSP;
            memcpy(&p_evt_param->event_param.ble_evt_adv.param.evt_set_scan_rsp, &p_cmd_cmp->para.set_scan_rsp, sizeof(ble_hci_return_param_set_scan_rsp_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] SCAN_RSP status %d\n", p_cmd_cmp->para.set_scan_rsp.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_ADVERTISING_ENABLE):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_ADV_EVT_SET_ENABLE;
            memcpy(&p_evt_param->event_param.ble_evt_adv.param.evt_set_adv_enable, &p_cmd_cmp->para.set_adv_enable, sizeof(ble_hci_return_param_set_adv_enable_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] ADV enable status %d\n", p_cmd_cmp->para.set_adv_enable.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_SCAN_PARAMETERS):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_SCAN_EVT_SET_PARAM;
            memcpy(&p_evt_param->event_param.ble_evt_scan.param.evt_set_scan_param, &p_cmd_cmp->para.set_scan_param, sizeof(ble_hci_return_param_set_scan_param_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Scan param status %d\n", p_cmd_cmp->para.set_scan_param.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_SCAN_ENABLE):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_SCAN_EVT_SET_ENABLE;
            memcpy(&p_evt_param->event_param.ble_evt_scan.param.evt_set_scan_enable, &p_cmd_cmp->para.set_scan_enable, sizeof(ble_hci_return_param_set_scan_enable_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Scan enable status %d\n", p_cmd_cmp->para.set_scan_enable.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_CREATE_CONNECTION_CANCEL):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_GAP_EVT_CONN_CANCEL;
            memcpy(&p_evt_param->event_param.ble_evt_gap.param, &p_cmd_cmp->para.create_conn_cancel, sizeof(ble_hci_return_param_create_conn_cancel_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Cancel create connection status %d\n", p_cmd_cmp->para.create_conn_cancel.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_READ_FILTER_ACCEPT_LIST_SIZE):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_READ_FILTER_ACCEPT_LIST_SIZE;
            memcpy(&p_evt_param->event_param.ble_evt_common.param, &p_cmd_cmp->para.read_accept_list_size, sizeof(ble_evt_common_read_filter_accept_list_size_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Read Filter accept list size status %d\n", p_cmd_cmp->para.read_accept_list_size.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_CLEAR_FILTER_ACCEPT_LIST):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_CLEAR_FILTER_ACCEPT_LIST;
            memcpy(&p_evt_param->event_param.ble_evt_common.param, &p_cmd_cmp->para.clear_accept_list, sizeof(ble_evt_common_clear_filter_accept_list_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Clear Filter accept list status %d\n", p_cmd_cmp->para.clear_accept_list.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_ADD_DEVICE_TO_FILTER_ACCEPT_LIST):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_ADD_FILTER_ACCEPT_LIST;
            memcpy(&p_evt_param->event_param.ble_evt_common.param, &p_cmd_cmp->para.add_accept_list, sizeof(ble_hci_return_param_add_accept_list_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Add Filter accept list status %d\n", p_cmd_cmp->para.add_accept_list.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_REMOVE_DEVICE_FROM_FILTER_ACCEPT_LIST):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_REMOVE_FILTER_ACCEPT_LIST;
            memcpy(&p_evt_param->event_param.ble_evt_common.param, &p_cmd_cmp->para.remove_accept_list, sizeof(ble_hci_return_param_remove_accept_list_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Remove Filter accept list status %d\n", p_cmd_cmp->para.remove_accept_list.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_ENCRYPT):
    {
        encrypt_queue_t encrypt_msg;

        if (p_cmd_cmp->para.set_encrypt.status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            if (sys_arch_queue_tryrecv(&g_host_encrypt_handle, &encrypt_msg) != SYS_ARCH_TIMEOUT)
            {
                status = bhc_prcss_le_encrypt_event(encrypt_msg, p_cmd_cmp->para.set_encrypt.encrypted_data);
            }
            else
            {
                BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_EVT] NO Encrypt queue data\n");
            }
        }
        else
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_EVT] encryption fail\n");
        }
    }
    break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_RANDOM):
    {
        encrypt_queue_t encrypt_msg;

        if (p_cmd_cmp->para.set_rand.status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            if (sys_arch_queue_tryrecv(&g_host_encrypt_handle, &encrypt_msg) != SYS_ARCH_TIMEOUT)
            {
                status = bhc_gen_random_value((ble_hci_return_param_rand_t *)&p_cmd_cmp->para.set_rand.status, encrypt_msg);
            }
            else
            {
                BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_EVT] NO Encrypt queue data\n");
            }
        }
        else
        {
            BLE_PRINTF(BLE_DEBUG_ERR, "[BLE_EVT] get random value fail\n");
        }
    }
    break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_LONG_TERM_KEY_REQ_NEG_REPLY):
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_READ_PHY):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_GAP_EVT_PHY_READ;
            p_evt_param->event_param.ble_evt_gap.param.evt_phy.host_id = bhc_query_host_id_by_conn_id(p_cmd_cmp->para.read_phy.conn_handle);
            p_evt_param->event_param.ble_evt_gap.param.evt_phy.status = p_cmd_cmp->para.read_phy.status;
            p_evt_param->event_param.ble_evt_gap.param.evt_phy.tx_phy = p_cmd_cmp->para.read_phy.tx_phy;
            p_evt_param->event_param.ble_evt_gap.param.evt_phy.rx_phy = p_cmd_cmp->para.read_phy.rx_phy;

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] tx phy%d rx phy%d\n", p_cmd_cmp->para.read_phy.tx_phy, p_cmd_cmp->para.read_phy.rx_phy);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_DATA_LENGTH):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_ATT_GATT_EVT_DATA_LENGTH_SET;
            p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_data_length_set.status = p_cmd_cmp->para.set_data_length.status;

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] set data length status %d\n", p_cmd_cmp->para.set_data_length.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH):
    {
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_ATT_GATT_EVT_WRITE_SUGGESTED_DEFAULT_DATA_LENGTH;
            p_evt_param->event_param.ble_evt_att_gatt.param.ble_evt_suggest_data_length_set.status = p_cmd_cmp->para.write_suggested_default_data_length.status;

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] set suggested default data length status %d\n", p_cmd_cmp->para.write_suggested_default_data_length.status);
        }
        else
        {
            status = ERR_MEM;
        }
    }
    break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_READ_CHANNEL_MAP):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_GAP_EVT_READ_CHANNEL_MAP;
            p_evt_param->event_param.ble_evt_gap.param.evt_channel_map.host_id = bhc_query_host_id_by_conn_id(p_cmd_cmp->para.read_channel_map.conn_handle);
            p_evt_param->event_param.ble_evt_gap.param.evt_channel_map.status = p_cmd_cmp->para.read_channel_map.status;
            memcpy(&p_evt_param->event_param.ble_evt_gap.param.evt_channel_map.channel_map[0], p_cmd_cmp->para.read_channel_map.channel_map, 5);

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] read RSSI status %d\n", p_cmd_cmp->para.read_channel_map.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_CONNECTION_CTE_RECEIVE_PARAMETERS):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_CTE_EVT_SET_CONN_CTE_RX_PARAM;
            p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_rx_param.status = p_cmd_cmp->para.set_cte_rx_param.status;
            p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_rx_param.host_id = bhc_query_host_id_by_conn_id(p_cmd_cmp->para.set_cte_rx_param.conn_handle);

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Set connection CTE rx parameters status %d\n", p_cmd_cmp->para.set_cte_rx_param.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_CONNECTION_CTE_TRANSMIT_PARAMETERS):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_CTE_EVT_SET_CONN_CTE_TX_PARAM;
            p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_tx_param.status = p_cmd_cmp->para.set_cte_tx_param.status;
            p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_tx_param.host_id = bhc_query_host_id_by_conn_id(p_cmd_cmp->para.set_cte_tx_param.conn_handle);

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Set connection CTE tx parameters status %d\n", p_cmd_cmp->para.set_cte_tx_param.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_CONNECTION_CTE_REQUEST_ENABLE):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_CTE_EVT_SET_CONN_CTE_REQ;
            p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_req.status = p_cmd_cmp->para.set_cte_req.status;
            p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_req.host_id = bhc_query_host_id_by_conn_id(p_cmd_cmp->para.set_cte_req.conn_handle);

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Set connection CTE tx parameters status %d\n", p_cmd_cmp->para.set_cte_tx_param.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_CONNECTION_CTE_RESPONSE_ENABLE):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_CTE_EVT_SET_CONN_CTE_RSP;
            p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_rsp.status = p_cmd_cmp->para.set_cte_rsp.status;
            p_evt_param->event_param.ble_evt_cte.param.evt_conn_cte_rsp.host_id = bhc_query_host_id_by_conn_id(p_cmd_cmp->para.set_cte_rsp.conn_handle);

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] Set connection CTE tx parameters status %d\n", p_cmd_cmp->para.set_cte_tx_param.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_READ_ANTENNA_INFORMATION):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_READ_ANTENNA_INFO;
            memcpy(&p_evt_param->event_param.ble_evt_common.param.evt_read_antenna_info.status, &p_cmd_cmp->para.read_antenna_info.status, sizeof(ble_hci_return_param_read_antenna_info_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] read antenna info status %d\n", p_cmd_cmp->para.read_antenna_info.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    /*=========== HCI_CMD_LE_VENDOR_CMD =====================================================================*/
    case GET_OPCODE(HCI_CMD_LE_VENDOR_CMD, LE_SET_CONTROLLER_INFO):
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_COMMON_EVT_SET_CONTROLLER_INFO;
            p_evt_param->event_param.ble_evt_common.param.evt_set_cntlr_info.status = p_cmd_cmp->para.set_ctrl_info.status;

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
            BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] set controller info status %d\n", p_cmd_cmp->para.set_ctrl_info.status);
        }
        else
        {
            status = ERR_MEM;
        }
        break;

    default:
        break;
    }

    return status;
}

static int8_t hci_command_status_event(uint8_t *p_data, uint8_t length)
{
    ble_hci_evt_param_cmd_status_t *p_cmd_status;
    uint16_t opcode;

    p_cmd_status = (ble_hci_evt_param_cmd_status_t *)p_data;
    opcode = (p_cmd_status->ocf | (p_cmd_status->ogf << 8));
    switch (opcode)
    {
    /* ==================================================
     *  Controller and Baseband Commands: OGF = 0x01
     * ================================================== */
    case GET_OPCODE(HCI_CMD_LINK_CTRL_CMD, LINK_CTRL_CMD_DISCONNECT):
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] disconnect cmd status:%d\n", p_cmd_status->status);
        break;

    case GET_OPCODE(HCI_CMD_LINK_CTRL_CMD, LINK_CTRL_CMD_READ_REMOTE_VERSION_INFO):
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] read remote version cmd status:%d\n", p_cmd_status->status);
        break;

    /* ==================================================
     *  LE Controller Command: OGF = 0x08
     * ================================================== */
    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_CREATE_CONNECTION):
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] create connection cmd status:%d\n", p_cmd_status->status);
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_CONNECTION_UPDATE):
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] connection update cmd status:%d\n", p_cmd_status->status);
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_READ_REMOTE_FEATURES):
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] read remote features cmd status:%d\n", p_cmd_status->status);
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_ENABLE_ENCRYPT):
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] le enable encrypt cmd status:%d\n", p_cmd_status->status);
        break;

    case GET_OPCODE(HCI_CMD_LE_CONTROLLER_CMD, LE_SET_PHY):
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] set PHY cmd status:%d\n", p_cmd_status->status);
        break;

    default:
        BLE_PRINTF(BLE_DEBUG_CMD_INFO, "[BLE_EVT] unknow cmd status %d, HCI_Ogf 0x%x, HCI_Ocf 0x%x\n", p_cmd_status->status, p_cmd_status->ogf, p_cmd_status->ocf);
        break;
    }

    return ERR_OK;
}

static int8_t hci_hardware_error_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_flush_occurred_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_role_change_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_number_of_complete_packets_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_mode_change_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_return_link_keys_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_pin_code_request_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_link_key_request_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_link_notification_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_loopback_command_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_data_buffer_overflow_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_max_slots_change_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_read_clock_offset_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_connection_packet_type_changed_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_qos_violation_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_page_scan_repetition_mode_change_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_flow_specification_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_inquiry_result_with_rssi_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_read_remote_extended_features_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_synchronous_connection_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_synchronous_connection_changed_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_sniff_subrating_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_extended_inquiry_result_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_encryption_key_refresh_event(uint8_t *p_data, uint8_t length)
{
    ble_hci_evt_param_key_refresh_complete_t *p_key_refresh_param = (ble_hci_evt_param_key_refresh_complete_t *)p_data;

    return bhc_prcss_encrpyt_key_refresh(p_key_refresh_param);
}

static int8_t hci_io_capability_request_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_io_capability_response_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_user_confirmation_request_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_user_passkey_request_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_remote_oob_data_request_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_simple_pairing_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_supervision_timeout_changed_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_enhanced_flush_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_user_passkey_notification_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_keypress_notification_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_remote_host_supported_features_notification_evnet(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_le_meta_event(uint8_t *p_data, uint8_t length)
{
    if (((ble_hci_le_meta_evt_t *)p_data)->subevent_code <= HCI_EVENT_CODE_LE_SUBRATE_CHANGE)
    {
        return prcss_le_meta_event[((ble_hci_le_meta_evt_t *)p_data)->subevent_code]((uint8_t *) & ((ble_hci_le_meta_evt_t *)p_data)->para);
    }
    else
    {
        return ERR_OK;
    }
}

static int8_t hci_number_of_complete_data_blocks_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_trigger_clock_capture_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_synchronization_train_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_synchronization_train_reveived_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_connectionless_peripheral_broadcast_receive_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_connectionless_peripheral_broadcast_timeout_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_truncated_page_complete_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_peripheral_page_response_timeout_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_connectionless_peripheral_broadcast_channel_map_change_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_inquiry_response_notification_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_authenticated_payload_timeout_expired_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}

static int8_t hci_sam_status_change_event(uint8_t *p_data, uint8_t length)
{
    return ERR_OK;
}



/**************************************************************************************************
 *    PUBLIC FUNCTIONS
 *************************************************************************************************/
int8_t (* const host_prcss_hci_event[])(uint8_t *p_data, uint8_t length) =
{
    hci_null_event,                                                     /*event code 0x00*/
    hci_inquiry_complete_event,                                         /*event code 0x01*/
    hci_inquiry_result_event,                                           /*event code 0x02*/
    hci_connection_complete_event,                                      /*event code 0x03*/
    hci_connection_request_event,                                       /*event code 0x04*/
    hci_disconnection_complete_event,                                   /*event code 0x05*/
    hci_authentication_complete_event,                                  /*event code 0x06*/
    hci_remote_name_request_complete_event,                             /*event code 0x07*/
    hci_encryption_change_event,                                        /*event code 0x08*/
    hci_change_connection_link_key_complete_event,                      /*event code 0x09*/
    hci_link_key_type_changed_event,                                    /*event code 0x0A*/
    hci_read_remote_supported_features_complete_event,                  /*event code 0x0B*/
    hci_read_remote_version_information_complete_event,                 /*event code 0x0C*/
    hci_qos_setup_complete_event,                                       /*event code 0x0D*/
    hci_command_complete_event,                                         /*event code 0x0E*/
    hci_command_status_event,                                           /*event code 0x0F*/
    hci_hardware_error_event,                                           /*event code 0x10*/
    hci_flush_occurred_event,                                           /*event code 0x11*/
    hci_role_change_event,                                              /*event code 0x12*/
    hci_number_of_complete_packets_event,                               /*event code 0x13*/
    hci_mode_change_event,                                              /*event code 0x14*/
    hci_return_link_keys_event,                                         /*event code 0x15*/
    hci_pin_code_request_event,                                         /*event code 0x16*/
    hci_link_key_request_event,                                         /*event code 0x17*/
    hci_link_notification_event,                                        /*event code 0x18*/
    hci_loopback_command_event,                                         /*event code 0x19*/
    hci_data_buffer_overflow_event,                                     /*event code 0x1A*/
    hci_max_slots_change_event,                                         /*event code 0x1B*/
    hci_read_clock_offset_complete_event,                               /*event code 0x1C*/
    hci_connection_packet_type_changed_event,                           /*event code 0x1D*/
    hci_qos_violation_event,                                            /*event code 0x1E*/
    hci_null_event,                                                     /*event code 0x1F*/
    hci_page_scan_repetition_mode_change_event,                         /*event code 0x20*/
    hci_flow_specification_complete_event,                              /*event code 0x21*/
    hci_inquiry_result_with_rssi_event,                                 /*event code 0x22*/
    hci_read_remote_extended_features_complete_event,                   /*event code 0x23*/
    hci_null_event,                                                     /*event code 0x24*/
    hci_null_event,                                                     /*event code 0x25*/
    hci_null_event,                                                     /*event code 0x26*/
    hci_null_event,                                                     /*event code 0x27*/
    hci_null_event,                                                     /*event code 0x28*/
    hci_null_event,                                                     /*event code 0x29*/
    hci_null_event,                                                     /*event code 0x2A*/
    hci_null_event,                                                     /*event code 0x2B*/
    hci_synchronous_connection_complete_event,                          /*event code 0x2C*/
    hci_synchronous_connection_changed_event,                           /*event code 0x2D*/
    hci_sniff_subrating_event,                                          /*event code 0x2E*/
    hci_extended_inquiry_result_event,                                  /*event code 0x2F*/
    hci_encryption_key_refresh_event,                                   /*event code 0x30*/
    hci_io_capability_request_event,                                    /*event code 0x31*/
    hci_io_capability_response_event,                                   /*event code 0x32*/
    hci_user_confirmation_request_event,                                /*event code 0x33*/
    hci_user_passkey_request_event,                                     /*event code 0x34*/
    hci_remote_oob_data_request_event,                                  /*event code 0x35*/
    hci_simple_pairing_complete_event,                                  /*event code 0x36*/
    hci_null_event,                                                     /*event code 0x37*/
    hci_supervision_timeout_changed_event,                              /*event code 0x38*/
    hci_enhanced_flush_complete_event,                                  /*event code 0x39*/
    hci_null_event,                                                     /*event code 0x3A*/
    hci_user_passkey_notification_event,                                /*event code 0x3B*/
    hci_keypress_notification_event,                                    /*event code 0x3C*/
    hci_remote_host_supported_features_notification_evnet,              /*event code 0x3D*/
    hci_le_meta_event,                                                  /*event code 0x3E*/
    hci_null_event,                                                     /*event code 0x3F*/
    hci_null_event,                                                     /*event code 0x40*/
    hci_null_event,                                                     /*event code 0x41*/
    hci_null_event,                                                     /*event code 0x42*/
    hci_null_event,                                                     /*event code 0x43*/
    hci_null_event,                                                     /*event code 0x44*/
    hci_null_event,                                                     /*event code 0x45*/
    hci_null_event,                                                     /*event code 0x46*/
    hci_null_event,                                                     /*event code 0x47*/
    hci_number_of_complete_data_blocks_event,                           /*event code 0x48*/
    hci_null_event,                                                     /*event code 0x49*/
    hci_null_event,                                                     /*event code 0x4A*/
    hci_null_event,                                                     /*event code 0x4B*/
    hci_null_event,                                                     /*event code 0x4C*/
    hci_null_event,                                                     /*event code 0x4D*/
    hci_trigger_clock_capture_event,                                    /*event code 0x4E*/
    hci_synchronization_train_complete_event,                           /*event code 0x4F*/
    hci_synchronization_train_reveived_event,                           /*event code 0x50*/
    hci_connectionless_peripheral_broadcast_receive_event,              /*event code 0x51*/
    hci_connectionless_peripheral_broadcast_timeout_event,              /*event code 0x52*/
    hci_truncated_page_complete_event,                                  /*event code 0x53*/
    hci_peripheral_page_response_timeout_event,                         /*event code 0x54*/
    hci_connectionless_peripheral_broadcast_channel_map_change_event,   /*event code 0x55*/
    hci_inquiry_response_notification_event,                            /*event code 0x56*/
    hci_authenticated_payload_timeout_expired_event,                    /*event code 0x57*/
    hci_sam_status_change_event,                                        /*event code 0x58*/
};

int8_t prcss_hci_vendor_event(uint8_t *p_data, uint8_t length)
{
    ble_hci_evt_param_vendor_t *p_vendor_param = (ble_hci_evt_param_vendor_t *)p_data;
    ble_host_to_app_evt_t *p_queue_param;
    ble_evt_param_t *p_evt_param;

    switch (p_vendor_param->subevent_code)
    {
    case HCI_SUBEVENT_VENDOR_SCAN_REQUEST_REPORT:
    {
        p_queue_param = mem_malloc(sizeof(ble_host_to_app_evt_t) + sizeof(ble_evt_param_t));
        if (p_queue_param != NULL)
        {
            p_queue_param->u32_send_systick = sys_now();
            p_queue_param->evt_type = BLE_APP_GENERAL_EVENT;

            p_evt_param = (ble_evt_param_t *)p_queue_param->parameter;
            p_evt_param->event = BLE_VENDOR_EVT_SCAN_REQ_REPORT;
            memcpy(&p_evt_param->event_param.ble_evt_common.param.evt_scan_req_rpt.addr_type, &p_vendor_param->addr_type, sizeof(ble_evt_vendor_scan_req_rpt_t));

            task_host_queue_send(TASK_HOST_QUEUE_TO_APP, p_queue_param);
        }
    }
    break;

    default:
        break;
    }

    return ERR_OK;
}

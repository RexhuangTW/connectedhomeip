#include "atcmd_command_list.h"

// PRIVATE FUNCTION DECLARE
static void con_latency_cmd_init(cmd_info_t *this);
static ble_err_t set_cmd(atcmd_item_t *item);
static ble_err_t read_cmd(atcmd_item_t *item);
static void test_cmd(atcmd_item_t *item);
static void check_event(ble_module_evt_t event, void *param, atcmd_item_t *item);

// PUBLIC VARIABLE DECLARE
static uint8_t g_host_id;
cmd_info_t con_latency =
{
    .cmd_name = "+CONLAT",
    .description = "connection latency",
    .init = con_latency_cmd_init
};

// PRIVATE FUNCTION IMPLEMENT
static void con_latency_cmd_init(cmd_info_t *this)
{
    cmd_info_init(this);
    this->set_cmd = set_cmd;
    this->read_cmd = read_cmd;
    this->test_cmd = test_cmd;
    this->check_event = check_event;
}
static ble_err_t set_cmd(atcmd_item_t *item)
{
    atcmd_param_block_t *param = item->param;
    ble_gap_conn_param_t con_param;
    uint8_t host_id = 0;

    if (item->param_length == 1)
    {
        atcmd_param_type param_type_list[] = {INT};
        bool check = parse_param_type(item, param_type_list, SIZE_ARR(param_type_list));
        CHECK_PARAM(check);
        host_id = 0;
        con_param = item->ble_param->con_param[host_id];
        con_param.periph_latency = param[0].num;
    }
    else if (item->param_length == 2)
    {
        atcmd_param_type param_type_list[] = {INT, INT};
        bool check = parse_param_type(item, param_type_list, SIZE_ARR(param_type_list));
        CHECK_PARAM(check);
        host_id = param[0].num;
        con_param = item->ble_param->con_param[host_id];
        con_param.periph_latency = param[1].num;
    }
    else
    {
        return BLE_ERR_INVALID_PARAMETER;
    }
    ble_gap_conn_param_update_param_t gap_con_param =
    {
        .host_id = host_id,
        .ble_conn_param = con_param
    };
    g_host_id = host_id;
    ble_err_t status = ble_cmd_conn_param_update(&gap_con_param);
    return status;
}
static ble_err_t read_cmd(atcmd_item_t *item)
{
    ble_gap_conn_param_t *con_param = item->ble_param->con_param;
    printf("%u\n", con_param[0].periph_latency);
    item->status = AT_CMD_STATUS_OK;
    return BLE_ERR_OK;
}
static void test_cmd(atcmd_item_t *item)
{
    printf(
        "+CONLAT?\n"
        "  get the connection latency\n"
        "+CONLAT = <num1>, <num2>\n"
        "  set the connection latency\n"
        "    <num1> : host ID\n"
        "      range : 0-0\n"
        "    <num2> : the connection latency\n"
        "      range : 0-499\n"
        "      interval = <num2> * 1.25ms\n"
        "    notice\n"
        "       must match this formula : timeout * 4 > interval_max * (1+latency)\n"
    );
}
static void check_event(ble_module_evt_t event, void *param, atcmd_item_t *item)
{
    if (event == BLE_GAP_EVT_CONN_PARAM_UPDATE)
    {
        ble_evt_gap_conn_param_update_t *p_update_conn_param = (ble_evt_gap_conn_param_update_t *)param;
        if (p_update_conn_param->status == BLE_HCI_ERR_CODE_SUCCESS)
        {
            item->status = AT_CMD_STATUS_OK;
        }
    }
    else if (event == BLE_GAP_EVT_DISCONN_COMPLETE)
    {
        ble_evt_gap_disconn_complete_t *p_disconn_param = (ble_evt_gap_disconn_complete_t *)param;
        if (p_disconn_param->host_id == g_host_id)
        {
            item->status = AT_CMD_STATUS_FAIL;
        }
    }
}

#include "atcmd_command_list.h"

// PRIVATE FUNCTION DECLARE
static void read_con_interval_cmd_init(cmd_info_t *this);
static ble_err_t set_cmd(atcmd_item_t *item);
static void test_cmd(atcmd_item_t *item);

// PUBLIC VARIABLE DECLARE
cmd_info_t read_con_interval =
{
    .cmd_name = "+READCONINT",
    .description = "read connection interval",
    .init = read_con_interval_cmd_init
};

// PRIVATE FUNCTION IMPLEMENT
static void read_con_interval_cmd_init(cmd_info_t *this)
{
    cmd_info_init(this);
    this->set_cmd = set_cmd;
    this->test_cmd = test_cmd;
}
static ble_err_t set_cmd(atcmd_item_t *item)
{
    atcmd_param_block_t *param = item->param;
    uint8_t host_id = 0;

    if (item->param_length == 1)
    {
        atcmd_param_type param_type_list[] = {INT};
        bool check = parse_param_type(item, param_type_list, SIZE_ARR(param_type_list));
        CHECK_PARAM(check);

        host_id = param[0].num;
        CHECK_HOST_ID(host_id);

        ble_gap_conn_param_t *con_param = item->ble_param->con_param;
        //printf(("min:%u max:%u\n", con_param[host_id].min_conn_interval, con_param[host_id].max_conn_interval);
        item->status = AT_CMD_STATUS_OK;
        return BLE_ERR_OK;
    }
    return BLE_ERR_INVALID_PARAMETER;
}
static void test_cmd(atcmd_item_t *item)
{
    //printf((
        "+READCONINT = <num>\n"
        "  read the connection interval of specific host ID\n"
        "    <num> : host ID\n"
        "      range : 0-0\n"
    );
}

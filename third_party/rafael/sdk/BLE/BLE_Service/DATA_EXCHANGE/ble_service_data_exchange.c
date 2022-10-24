/************************************************************************
 *
 * File Name  : ble_service_data_exchange.c
 * Description: This file contains the definitions and functions of BLE DATA_EXCHANGE
 *
 *
 ************************************************************************/
#include "ble_service_data_exchange.h"
#include "ble_profile.h"

/** ble_svcs_data_exchange_handler
 * @note This callback receives the DATA_EXCHANGE events.
 * Each of these events can be associated with parameters.
*/

void ble_svcs_data_exchange_handler(ble_evt_att_param_t *p_param);


/**************************************************************************
 * DATA_EXCHANGE UUID Definitions
 **************************************************************************/

/** DATA_EXCHANGE UUID.
 * @note 128-bits UUID
 * @note UUID: 00005301000000414C50574953450000
*/

const uint16_t attr_uuid_data_exchange_primary_service[] =
{
    0x0000, 0x5345,
    0x5749, 0x4C50,
    0x0041, 0x0000,
    0x5301, 0x0000,
};

/** DATA_EXCHANGE characteristic WRITE UUID.
 * @note 128-bits UUID
 * @note UUID: 00005302000000414C50574953450000
*/

const uint16_t attr_uuid_data_exchange_charc_write[] =
{
    0x0000, 0x5345,
    0x5749, 0x4C50,
    0x0041, 0x0000,
    0x5302, 0x0000,
};

/** DATA_EXCHANGE characteristic NOTIFY_INDICATE UUID.
 * @note 128-bits UUID
 * @note UUID: 00005303000000414C50574953450000
*/

const uint16_t attr_uuid_data_exchange_charc_notify_indicate[] =
{
    0x0000, 0x5345,
    0x5749, 0x4C50,
    0x0041, 0x0000,
    0x5303, 0x0000,
};


/**************************************************************************
 * DATA_EXCHANGE Service Value Definitions
 **************************************************************************/


/**************************************************************************
 * DATA_EXCHANGE Service/ Characteristic Definitions
 **************************************************************************/

const ble_att_param_t att_data_exchange_primary_service =
{
    (void *)attr_uuid_type_primary_service,
    (void *)attr_uuid_data_exchange_primary_service,
    sizeof(attr_uuid_data_exchange_primary_service),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_data_exchange_characteristic_write =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_data_exchange_charc_write,
    sizeof(attr_uuid_data_exchange_charc_write),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_data_exchange_write =
{
    (void *)attr_uuid_data_exchange_charc_write,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        //GATT_DECLARATIONS_PROPERTIES_READ |
        GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        //ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_data_exchange_handler,       //registered callback function
};

const ble_att_param_t att_data_exchange_characteristic_notify_indicate =
{
    (void *)attr_uuid_type_characteristic,
    (void *)attr_uuid_data_exchange_charc_notify_indicate,
    sizeof(attr_uuid_data_exchange_charc_notify_indicate),
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    attr_null_access,                       //This function should be set to attr_null_access when att_len or p_uuid_value is a null value.
};

const ble_att_param_t att_data_exchange_notify_indicate =
{
    (void *)attr_uuid_data_exchange_charc_notify_indicate,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        //GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        //GATT_DECLARATIONS_PROPERTIES_WRITE |
        GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        //ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_data_exchange_handler,       //registered callback function
};

const ble_att_param_t att_data_exchange_notify_indicate_client_charc_configuration =
{
    (void *)attr_uuid_type_client_charc_configuration,
    (void *)0,
    0,
    (
        //GATT_DECLARATIONS_PROPERTIES_BROADCAST |
        GATT_DECLARATIONS_PROPERTIES_READ |
        //GATT_DECLARATIONS_PROPERTIES_WRITE_WITHOUT_RESPONSE |
        GATT_DECLARATIONS_PROPERTIES_WRITE |
        //GATT_DECLARATIONS_PROPERTIES_NOTIFY |
        //GATT_DECLARATIONS_PROPERTIES_INDICATE |
        //GATT_DECLARATIONS_PROPERTIES_AUTHENTICATED_SIGNED_WRITES |
        //GATT_DECLARATIONS_PROPERTIES_EXTENDED_PROPERTIES |
        0x00
    ),
    (
        ATT_TYPE_FORMAT_16UUID |            //otherwise, 128bit UUID
        //ATT_VALUE_BOND_ENABLE |
        //ATT_PERMISSION_ENC_READ |
        //ATT_PERMISSION_ENC_WRITE |
        //ATT_PERMISSION_AUTHE_READ |
        //ATT_PERMISSION_AUTHE_WRITE |
        //ATT_PERMISSION_AUTHO_READ |
        //ATT_PERMISSION_AUTHO_WRITE |
        0x00
    ),
    ble_svcs_data_exchange_handler,       //registered callback function
};


/**************************************************************************
 * BLE Service << DATA_EXCHANGE >> Local Variable
 **************************************************************************/
#ifndef MAX_NUM_CONN_DATA_EXCHANGE
// check MAX_NUM_CONN_DATA_EXCHANGE if defined or set to default 1.
#define MAX_NUM_CONN_DATA_EXCHANGE       1
#endif


// Service basic information
ble_svcs_common_info_t                    data_exchange_basic_info[MAX_NUM_CONN_DATA_EXCHANGE];

// DATA_EXCHANGE information
ble_svcs_data_exchange_info_t             *data_exchange_info[MAX_NUM_CONN_DATA_EXCHANGE];

// DATA_EXCHANGE callback function
ble_svcs_evt_data_exchange_handler_t      data_exchange_callback[MAX_NUM_CONN_DATA_EXCHANGE];

// DATA_EXCHANGE registered total count
uint8_t                                   data_exchange_count = 0;


/**************************************************************************
 * BLE Service << DATA_EXCHANGE >> Public Function
 **************************************************************************/
/* DATA_EXCHANGE Initialization */
ble_err_t ble_svcs_data_exchange_init(uint8_t host_id, ble_gatt_role_t role, ble_svcs_data_exchange_info_t *p_info, ble_svcs_evt_data_exchange_handler_t callback)
{
    ble_err_t status;
    uint8_t config_index;

    if (p_info == NULL)
    {
        return BLE_ERR_INVALID_PARAMETER;
    }

    // init service client basic information and get "config_index" & "data_exchange_count"
    status = ble_svcs_common_init(host_id, role, MAX_NUM_CONN_DATA_EXCHANGE, data_exchange_basic_info, &config_index, &data_exchange_count);
    if (status != BLE_ERR_OK)
    {
        return status;
    }

    // Set service role
    p_info->role = role;

    // Set DATA_EXCHANGE data
    data_exchange_info[config_index] = p_info;

    // Register DATA_EXCHANGE callback function
    data_exchange_callback[config_index] = callback;

    // Get handles at initialization if role is set to BLE_GATT_ROLE_SERVER
    if ((role & BLE_GATT_ROLE_SERVER) != 0)
    {
        status = ble_svcs_data_exchange_handles_get(host_id, BLE_GATT_ROLE_SERVER, data_exchange_info[config_index]);
        if (status != BLE_ERR_OK)
        {
            return status;
        }
    }

    return BLE_ERR_OK;
}


/* Get DATA_EXCHANGE Handle Numbers */
ble_err_t ble_svcs_data_exchange_handles_get(uint8_t host_id, ble_gatt_role_t role, ble_svcs_data_exchange_info_t *p_info)
{
    ble_err_t status;
    ble_gatt_handle_table_param_t ble_gatt_handle_table_param;

    status = BLE_ERR_OK;
    do
    {
        ble_gatt_handle_table_param.host_id = host_id;
        ble_gatt_handle_table_param.gatt_role = p_info->role;
        ble_gatt_handle_table_param.p_element = (ble_att_param_t *)&att_data_exchange_primary_service;

        if (role == BLE_GATT_ROLE_SERVER)
        {
            ble_gatt_handle_table_param.p_handle_num_addr = (void *)&p_info->server_info.handles;
        }
        else if (role == BLE_GATT_ROLE_CLIENT)
        {
            ble_gatt_handle_table_param.p_handle_num_addr = (void *)&p_info->client_info.handles;
        }
        else
        {
            info_color(LOG_RED, "Error role setting.\n");
            status = BLE_ERR_INVALID_PARAMETER;
            break;
        }
        status = ble_svcs_handles_mapping_get(&ble_gatt_handle_table_param);
    } while (0);

    return status;
}

/**************************************************************************
 * BLE Service << DATA_EXCHANGE >> General Callback Function
 **************************************************************************/
// post the event to the callback function
static void data_exchange_evt_post(ble_evt_att_param_t *p_param, ble_svcs_evt_data_exchange_handler_t *p_callback)
{
    // check callback is null or not
    if (*p_callback == NULL)
    {
        return;
    }
    // post event to user
    (*p_callback)(p_param);
}


// handle DATA_EXCHANGE client GATT event
static void handle_data_exchange_client(uint8_t index, ble_evt_att_param_t *p_param)
{
    switch (p_param->opcode)
    {
    case OPCODE_ATT_READ_RESPONSE:
        if (p_param->handle_num == data_exchange_info[index]->client_info.handles.hdl_notify_indicate_cccd)
        {
            // received cccd read response from server
            p_param->event = BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_CCCD_READ_RSP_EVENT;
            data_exchange_evt_post(p_param, &data_exchange_callback[index]);
        }
        break;
    case OPCODE_ATT_WRITE_RESPONSE:
    case OPCODE_ATT_RESTORE_BOND_DATA_COMMAND:
        if (p_param->handle_num == data_exchange_info[index]->client_info.handles.hdl_write)
        {
            // received write response from server
            p_param->event = BLESERVICE_DATA_EXCHANGE_WRITE_WRITE_RSP_EVENT;
            data_exchange_evt_post(p_param, &data_exchange_callback[index]);
        }
        else if (p_param->handle_num == data_exchange_info[index]->client_info.handles.hdl_notify_indicate_cccd)
        {
            // received cccd write response from server
            p_param->event = BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_CCCD_WRITE_RSP_EVENT;
            data_exchange_evt_post(p_param, &data_exchange_callback[index]);
        }
        break;
    case OPCODE_ATT_HANDLE_VALUE_NOTIFICATION:
        if (p_param->handle_num == data_exchange_info[index]->client_info.handles.hdl_notify_indicate)
        {
            // received notify from server
            p_param->event = BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_NOTIFY_EVENT;
            data_exchange_evt_post(p_param, &data_exchange_callback[index]);
        }
        break;
    case OPCODE_ATT_HANDLE_VALUE_INDICATION:
        if (p_param->handle_num == data_exchange_info[index]->client_info.handles.hdl_notify_indicate)
        {
            // received indicate from server
            p_param->event = BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_INDICATE_EVENT;
            data_exchange_evt_post(p_param, &data_exchange_callback[index]);
        }
        break;

    default:
        break;
    }
}


// handle DATA_EXCHANGE server GATT event
static void handle_data_exchange_server(uint8_t index, ble_evt_att_param_t *p_param)
{
    switch (p_param->opcode)
    {
    case OPCODE_ATT_READ_REQUEST:
    case OPCODE_ATT_READ_BY_TYPE_REQUEST:
        if (p_param->handle_num == data_exchange_info[index]->server_info.handles.hdl_notify_indicate_cccd)
        {
            // received cccd read from client
            ble_svcs_auto_handle_cccd_read_req(p_param, data_exchange_info[index]->server_info.data.notify_indicate_cccd);
        }
        break;
    case OPCODE_ATT_WRITE_REQUEST:
        if (p_param->handle_num == data_exchange_info[index]->server_info.handles.hdl_write)
        {
            // received write from client
            p_param->event = BLESERVICE_DATA_EXCHANGE_WRITE_WRITE_EVENT;
            data_exchange_evt_post(p_param, &data_exchange_callback[index]);
        }
        else if (p_param->handle_num == data_exchange_info[index]->server_info.handles.hdl_notify_indicate_cccd)
        {
            // received cccd write from client
            ble_svcs_handle_cccd_write_req(p_param->data, p_param->length, &data_exchange_info[index]->server_info.data.notify_indicate_cccd);
        }
        break;
    case OPCODE_ATT_WRITE_COMMAND:
        if (p_param->handle_num == data_exchange_info[index]->server_info.handles.hdl_write)
        {
            // received write without response from client
            p_param->event = BLESERVICE_DATA_EXCHANGE_WRITE_WRITE_WITHOUT_RSP_EVENT;
            data_exchange_evt_post(p_param, &data_exchange_callback[index]);
        }
        break;
    case OPCODE_ATT_HANDLE_VALUE_CONFIRMATION:
        if (p_param->handle_num == data_exchange_info[index]->server_info.handles.hdl_notify_indicate)
        {
            // received indicate confirm from client
            p_param->event = BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_INDICATE_CONFIRM_EVENT;
            data_exchange_evt_post(p_param, &data_exchange_callback[index]);
        }
        break;

    default:
        break;
    }
}



/** ble_svcs_data_exchange_handler
 * @note This callback receives the DATA_EXCHANGE events.
 * Each of these events can be associated with parameters.
*/

void ble_svcs_data_exchange_handler(ble_evt_att_param_t *p_param)
{
    uint8_t index;

    if (ble_svcs_common_info_index_query(p_param->host_id, p_param->gatt_role, MAX_NUM_CONN_DATA_EXCHANGE, data_exchange_basic_info, &index) != BLE_ERR_OK)
    {
        // Host id has not registered so there is no callback function -> do nothing
        return;
    }

    if (p_param->gatt_role == BLE_GATT_ROLE_CLIENT)
    {
        // handle DATA_EXCHANGE client GATT event
        handle_data_exchange_client(index, p_param);
    }

    if (p_param->gatt_role == BLE_GATT_ROLE_SERVER)
    {
        // handle DATA_EXCHANGE server GATT event
        handle_data_exchange_server(index, p_param);
    }
}

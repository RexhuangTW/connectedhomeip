#ifndef _BLE_SERVICE_DATA_EXCHANGE_H_
#define _BLE_SERVICE_DATA_EXCHANGE_H_

/**************************************************************************//**
 * @file  ble_service_data_exchange.h
 * @brief Provide the Definition of DATA_EXCHANGE.
*****************************************************************************/

#include "ble_service_common.h"
#include "ble_att_gatt.h"


/**************************************************************************
 * DATA_EXCHANGE Definitions
 **************************************************************************/
/** @defgroup service_data_exchange_def BLE DATA_EXCHANGE Definitions
 * @{
 * @ingroup service_def
 * @details Here shows the definitions of the DATA_EXCHANGE.
 * @}
**************************************************************************/

/**
 * @ingroup service_data_exchange_def
 * @defgroup service_data_exchange_UUIDDef BLE DATA_EXCHANGE UUID Definitions
 * @{
 * @details Here shows the definitions of the BLE DATA_EXCHANGE UUID Definitions.
*/
extern const uint16_t attr_uuid_data_exchange_primary_service[];          /**< DATA_EXCHANGE service UUID. */
extern const uint16_t attr_uuid_data_exchange_charc_write[];              /**< DATA_EXCHANGE characteristic WRITE UUID. */
extern const uint16_t attr_uuid_data_exchange_charc_notify_indicate[];    /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE UUID. */
/** @} */

/**
 * @defgroup service_data_exchange_ServiceChardef BLE DATA_EXCHANGE Service and Characteristic Definitions
 * @{
 * @ingroup service_data_exchange_def
 * @details Here shows the definitions of the DATA_EXCHANGE service and characteristic.
 * @}
*/

/**
 * @ingroup service_data_exchange_ServiceChardef
 * @{
*/
extern const ble_att_param_t att_data_exchange_primary_service;                               /**< DATA_EXCHANGE primary service. */
extern const ble_att_param_t att_data_exchange_characteristic_write;                          /**< DATA_EXCHANGE characteristic WRITE. */
extern const ble_att_param_t att_data_exchange_write;                                         /**< DATA_EXCHANGE WRITE value. */
extern const ble_att_param_t att_data_exchange_characteristic_notify_indicate;                /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE. */
extern const ble_att_param_t att_data_exchange_notify_indicate;                               /**< DATA_EXCHANGE NOTIFY_INDICATE value. */
extern const ble_att_param_t att_data_exchange_notify_indicate_client_charc_configuration;    /**< DATA_EXCHANGE NOTIFY_INDICATE client characteristic configuration. */
/** @} */


/** DATA_EXCHANGE Definition
 * @ingroup service_data_exchange_ServiceChardef
*/
#define ATT_DATA_EXCHANGE_SERVICE                                          \
    &att_data_exchange_primary_service,                                    \
    &att_data_exchange_characteristic_write,                               \
    &att_data_exchange_write,                                              \
    &att_data_exchange_characteristic_notify_indicate,                     \
    &att_data_exchange_notify_indicate,                                    \
    &att_data_exchange_notify_indicate_client_charc_configuration          \


/**************************************************************************
 * DATA_EXCHANGE Application Definitions
 **************************************************************************/
/** @defgroup app_data_exchange_def BLE DATA_EXCHANGE Application Definitions
 * @{
 * @ingroup service_data_exchange_def
 * @details Here shows the definitions of the DATA_EXCHANGE for application.
 * @}
**************************************************************************/

/**
 * @ingroup app_data_exchange_def
 * @defgroup app_data_exchange_eventDef BLE DATA_EXCHANGE Service and Characteristic Definitions
 * @{
 * @details Here shows the event definitions of the DATA_EXCHANGE.
*/
#define BLESERVICE_DATA_EXCHANGE_WRITE_WRITE_EVENT                          0x01     /**< DATA_EXCHANGE characteristic WRITE write event.*/
#define BLESERVICE_DATA_EXCHANGE_WRITE_WRITE_RSP_EVENT                      0x02     /**< DATA_EXCHANGE characteristic WRITE write response event.*/
#define BLESERVICE_DATA_EXCHANGE_WRITE_WRITE_WITHOUT_RSP_EVENT              0x03     /**< DATA_EXCHANGE characteristic WRITE write without response event.*/
#define BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_NOTIFY_EVENT               0x04     /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE notify event.*/
#define BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_INDICATE_CONFIRM_EVENT     0x05     /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE indicate confirm event.*/
#define BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_INDICATE_EVENT             0x06     /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE indicate event.*/
#define BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_CCCD_READ_EVENT            0x07     /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE cccd read event.*/
#define BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_CCCD_READ_RSP_EVENT        0x08     /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE cccd read response event.*/
#define BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_CCCD_WRITE_EVENT           0x09     /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE cccd write event.*/
#define BLESERVICE_DATA_EXCHANGE_NOTIFY_INDICATE_CCCD_WRITE_RSP_EVENT       0x0a     /**< DATA_EXCHANGE characteristic NOTIFY_INDICATE cccd write response event.*/
/** @} */


/**
 * @ingroup app_data_exchange_def
 * @defgroup app_data_exchange_structureDef BLE DATA_EXCHANGE Structure Definitions
 * @{
 * @details Here shows the structure definitions of the DATA_EXCHANGE.
 * @}
*/

/** DATA_EXCHANGE Handles Definition
 * @ingroup app_data_exchange_structureDef
*/
typedef struct ble_svcs_data_exchange_handles_s
{
    uint16_t hdl_write;                   /**< Handle of WRITE. */
    uint16_t hdl_notify_indicate;         /**< Handle of NOTIFY_INDICATE. */
    uint16_t hdl_notify_indicate_cccd;    /**< Handle of NOTIFY_INDICATE client characteristic configuration. */
} ble_svcs_data_exchange_handles_t;


/** DATA_EXCHANGE Data Definition
 * @ingroup app_data_exchange_structureDef
 * @note User defined.
*/
typedef struct ble_svcs_data_exchange_data_s
{
    uint16_t notify_indicate_cccd;    /**< NOTIFY_INDICATE cccd value */
} ble_svcs_data_exchange_data_t;


/** DATA_EXCHANGE Application Data Structure Definition
 * @ingroup app_data_exchange_structureDef
*/
typedef struct ble_svcs_data_exchange_subinfo_s
{
    ble_svcs_data_exchange_handles_t handles;    /**< DATA_EXCHANGE attribute handles */
    ble_svcs_data_exchange_data_t    data;       /**< DATA_EXCHANGE attribute data */
} ble_svcs_data_exchange_subinfo_t;


typedef struct ble_svcs_data_exchange_info_s
{
    ble_gatt_role_t           role;         /**< BLE GATT role */
    ble_svcs_data_exchange_subinfo_t   client_info;
    ble_svcs_data_exchange_subinfo_t   server_info;
} ble_svcs_data_exchange_info_t;


/**
 * @ingroup app_data_exchange_def
 * @defgroup app_data_exchange_App BLE DATA_EXCHANGE Definitions for Application
 * @{
 * @details Here shows the definitions of the DATA_EXCHANGE for application uses.
 * @}
*/

/** ble_svcs_evt_data_exchange_handler_t
 * @ingroup app_data_exchange_App
 * @note This callback receives the DATA_EXCHANGE events. Each of these events can be associated with parameters.
*/
typedef void (*ble_svcs_evt_data_exchange_handler_t)(ble_evt_att_param_t *p_param);

/** DATA_EXCHANGE Initialization
*
* @ingroup app_data_exchange_App
*
* @attention There is only one instance of DATA_EXCHANGE shall be exposed on a device (if role is @ref BLE_GATT_ROLE_SERVER). \n
*            Callback shall be ignored if role is @ref BLE_GATT_ROLE_SERVER).
*
* @param[in] host_id : the link's host id.
* @param[in] role : @ref ble_gatt_role_t "BLE GATT role".
* @param[in] p_info : a pointer to DATA_EXCHANGE information.
* @param[in] callback : a pointer to a callback function that receive the service events.
*
* @retval BLE_ERR_INVALID_HOST_ID : Error host id.
* @retval BLE_ERR_INVALID_PARAMETER : Invalid parameter.
* @retval BLE_ERR_CMD_NOT_SUPPORTED : Registered services buffer full.
* @retval BLE_ERR_OK  : Setting success.
*/
ble_err_t ble_svcs_data_exchange_init(uint8_t host_id, ble_gatt_role_t role, ble_svcs_data_exchange_info_t *p_info, ble_svcs_evt_data_exchange_handler_t callback);

/** Get DATA_EXCHANGE Handle Numbers
*
* @ingroup app_data_exchange_App
*
* @attention - role = @ref BLE_GATT_ROLE_CLIENT \n
*              MUST call this API to get service information after received @ref BLECMD_EVENT_ATT_DATABASE_PARSING_FINISHED  \n
*            - role = @ref BLE_GATT_ROLE_SERVER \n
*              MUST call this API to get service information before connection established. \n
*
* @param[in] host_id : the link's host id.
* @param[out] p_info : a pointer to DATA_EXCHANGE information.
*
* @retval BLE_ERR_INVALID_HOST_ID : Error host id.
* @retval BLE_ERR_INVALID_PARAMETER : Invalid parameter.
* @retval BLE_ERR_OK : Setting success.
*/
ble_err_t ble_svcs_data_exchange_handles_get(uint8_t host_id, ble_gatt_role_t role, ble_svcs_data_exchange_info_t *p_info);

#endif //_BLE_SERVICE_DATA_EXCHANGE_H_


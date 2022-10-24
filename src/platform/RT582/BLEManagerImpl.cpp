/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
 *    Copyright (c) 2019 Nest Labs, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>
#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE


#include <platform/internal/BLEManager.h>

#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"
#include <ble/CHIPBleServiceData.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>

#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
#include <setup_payload/AdditionalDataPayloadGenerator.h>
#endif

#include "task_hci.h"
#include "ble_event.h"
#include "ble_profile.h"

#include "util_log.h"


using namespace ::chip;
using namespace ::chip::Ble;

namespace chip {
namespace DeviceLayer {
namespace Internal {

namespace {

#define CHIP_ADV_DATA_TYPE_FLAGS 0x01
#define CHIP_ADV_DATA_TYPE_UUID 0x03
#define CHIP_ADV_DATA_TYPE_NAME 0x09
#define CHIP_ADV_DATA_TYPE_SERVICE_DATA 0x16

#define CHIP_ADV_DATA_FLAGS 0x06

#define CHIP_ADV_DATA 0
#define CHIP_ADV_SCAN_RESPONSE_DATA 1
#define CHIP_ADV_SHORT_UUID_LEN 2

// Default Connection  parameters
#define BLE_CONFIG_MIN_INTERVAL (16) // Time = Value x 1.25 ms = 30ms
#define BLE_CONFIG_MAX_INTERVAL (80) // Time = Value x 1.25 ms = 100ms
#define BLE_CONFIG_LATENCY (0)
#define BLE_CONFIG_TIMEOUT (100)          // Time = Value x 10 ms = 1s
#define BLE_CONFIG_MIN_CE_LENGTH (0)      // Leave to min value
#define BLE_CONFIG_MAX_CE_LENGTH (0xFFFF) // Leave to max value

#define BLE_APP_CB_QUEUE_SIZE           16
#define APP_ISR_QUEUE_SIZE              2
#define APP_REQ_QUEUE_SIZE              6
#define APP_QUEUE_SIZE                  (BLE_APP_CB_QUEUE_SIZE + APP_ISR_QUEUE_SIZE + APP_REQ_QUEUE_SIZE)


static TaskHandle_t BluetoothEventTaskHandle;

static xQueueHandle g_app_msg_q;
static SemaphoreHandle_t semaphore_cb;
static SemaphoreHandle_t semaphore_isr;
static SemaphoreHandle_t semaphore_app;

static ble_cfg_t gt_app_cfg;
const uint8_t UUID_CHIPoBLEService[]    = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
                                         0x00, 0x10, 0x00, 0x00, 0xF6, 0xFF, 0x00, 0x00 };
const uint8_t ShortUUID_CHIPoBLEService[]  = { 0xF6, 0xFF };
const ChipBleUUID ChipUUID_CHIPoBLEChar_RX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F,
                                                 0x9D, 0x11 } };
const ChipBleUUID ChipUUID_CHIPoBLEChar_TX = { { 0x18, 0xEE, 0x2E, 0xF5, 0x26, 0x3D, 0x45, 0x59, 0x95, 0x9F, 0x4F, 0x9C, 0x42, 0x9F,
                                                 0x9D, 0x12 } };

typedef enum
{
    QUEUE_TYPE_APP_REQ,   /**< Application queue type: application request.*/
    QUEUE_TYPE_OTHERS,    /**< Application queue type: others including BLE events and BLE service events.*/
} app_queue_param_type_t;

typedef enum
{
    APP_REQUEST_IDLE,             /**< Application request event: idle.*/
    APP_REQUEST_ADV_START,        /**< Application request event: advertising start.*/
    APP_REQUEST_TRSPS_DATA_SEND,  /**< Application request event: TRSP server data send.*/
} app_request_t;

typedef enum
{
    STATE_STANDBY,        /**< Application state: standby.*/
    STATE_ADVERTISING,    /**< Application state: advertising.*/
    STATE_CONNECTED,      /**< Application state: connected.*/
} ble_state_t;


typedef struct
{
    uint8_t   host_id;    /**< Application request parameter: host id.*/
    uint16_t  app_req;    /**< Application request parameter: @ref app_request_t "application request event".*/
} app_req_param_t;

typedef struct
{
    uint8_t   event;        /**< Application queue parameter: event.*/
    uint8_t   from_isr;     /**< Application queue parameter: Dose the Request come from interruption? */
    uint16_t  param_type;   /**< Application queue parameter: @ref app_queue_param_type_t "application queue type".*/
    union
    {
        app_req_param_t  app_req;   /**< Application queue parameter: application request event. */
        ble_tlv_t        *pt_tlv;   /**< Application queue parameter: parameters (type: @ref ble_event_t, length, and value). */
    } param;
} app_queue_t;

} // namespace

BLEManagerImpl BLEManagerImpl::sInstance;


void BLEManagerImpl::app_evt_indication_cb(uint32_t data_len)
{
    int i32_err;
    uint8_t *p_buf;
    

    i32_err = ble_event_msg_recvfrom(p_buf, &data_len);
}

CHIP_ERROR BLEManagerImpl::_Init()
{
    CHIP_ERROR err;

    util_log_on(UTIL_LOG_PROTOCOL);

    gt_app_cfg.pf_evt_indication = app_evt_indication_cb;

    task_hci_init();

    ble_host_stack_init(&gt_app_cfg);

    g_app_msg_q = xQueueCreate(APP_QUEUE_SIZE, sizeof(app_queue_t));
    semaphore_cb = xSemaphoreCreateCounting(BLE_APP_CB_QUEUE_SIZE, BLE_APP_CB_QUEUE_SIZE);
    semaphore_isr = xSemaphoreCreateCounting(APP_ISR_QUEUE_SIZE, APP_ISR_QUEUE_SIZE);
    semaphore_app = xSemaphoreCreateCounting(APP_REQ_QUEUE_SIZE, APP_REQ_QUEUE_SIZE);    

    if (BluetoothEventTaskHandle != NULL)
    {
        return CHIP_ERROR_INCORRECT_STATE;
    }

    xTaskCreate(_BLETaskMain, 
        CHIP_DEVICE_CONFIG_THREAD_TASK_NAME,
        CHIP_DEVICE_CONFIG_BLE_APP_TASK_STACK_SIZE / sizeof(StackType_t), 
        this, 
        TASK_PRIORITY_APP,
        &BluetoothEventTaskHandle);

    if (BluetoothEventTaskHandle == NULL)
    {
        return CHIP_ERROR_NO_MEMORY;
    }    

exit:
    return err;
}

uint16_t BLEManagerImpl::_NumConnections(void)
{
    return 1;
}

bool BLEManagerImpl::_IsAdvertisingEnabled(void)
{
    return true;
}

CHIP_ERROR BLEManagerImpl::_SetAdvertisingEnabled(bool val)
{
    //Need to implement
    CHIP_ERROR err = CHIP_NO_ERROR;

    
exit:
    return err;
}

CHIP_ERROR BLEManagerImpl::_SetAdvertisingMode(BLEAdvertisingMode mode)
{
    //Need to implement

    switch (mode)
    {
    case BLEAdvertisingMode::kFastAdvertising:
        
        break;
    case BLEAdvertisingMode::kSlowAdvertising:
        
        break;
    default:
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::_GetDeviceName(char * buf, size_t bufSize)
{
    //Need to implement

    return CHIP_NO_ERROR;
}

CHIP_ERROR BLEManagerImpl::_SetDeviceName(const char * deviceName)
{
    //Need to implement

    return CHIP_NO_ERROR;
}

void BLEManagerImpl::_OnPlatformEvent(const ChipDeviceEvent * event)
{
    switch (event->Type)
    {
    case DeviceEventType::kCHIPoBLESubscribe: {
        ChipDeviceEvent connEstEvent;

        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLESubscribe");
        HandleSubscribeReceived(event->CHIPoBLESubscribe.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
        connEstEvent.Type = DeviceEventType::kCHIPoBLEConnectionEstablished;
        PlatformMgr().PostEventOrDie(&connEstEvent);
    }
    break;

    case DeviceEventType::kCHIPoBLEUnsubscribe: {
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLEUnsubscribe");
        HandleUnsubscribeReceived(event->CHIPoBLEUnsubscribe.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
    }
    break;

    case DeviceEventType::kCHIPoBLEWriteReceived: {
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLEWriteReceived");
        HandleWriteReceived(event->CHIPoBLEWriteReceived.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_RX,
                            PacketBufferHandle::Adopt(event->CHIPoBLEWriteReceived.Data));
    }
    break;

    case DeviceEventType::kCHIPoBLEConnectionError: {
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLEConnectionError");
        HandleConnectionError(event->CHIPoBLEConnectionError.ConId, event->CHIPoBLEConnectionError.Reason);
    }
    break;

    case DeviceEventType::kCHIPoBLEIndicateConfirm: {
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent kCHIPoBLEIndicateConfirm");
        HandleIndicationConfirmation(event->CHIPoBLEIndicateConfirm.ConId, &CHIP_BLE_SVC_ID, &ChipUUID_CHIPoBLEChar_TX);
    }
    break;

    default:
        ChipLogProgress(DeviceLayer, "_OnPlatformEvent default:  event->Type = %d", event->Type);
        break;
    }
}

bool BLEManagerImpl::SubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    ChipLogProgress(DeviceLayer, "BLEManagerImpl::SubscribeCharacteristic() not supported");
    return false;
}

bool BLEManagerImpl::UnsubscribeCharacteristic(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    ChipLogProgress(DeviceLayer, "BLEManagerImpl::UnsubscribeCharacteristic() not supported");
    return false;
}

bool BLEManagerImpl::CloseConnection(BLE_CONNECTION_OBJECT conId)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    ChipLogProgress(DeviceLayer, "Closing BLE GATT connection (con %u)", conId);

    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "sl_bt_connection_close() failed: %s", ErrorStr(err));
    }

    return (err == CHIP_NO_ERROR);
}

uint16_t BLEManagerImpl::GetMTU(BLE_CONNECTION_OBJECT conId) const
{
    //Need to implement
    return 0;
}

bool BLEManagerImpl::SendIndication(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                    PacketBufferHandle data)
{
    CHIP_ERROR err              = CHIP_NO_ERROR;
    //Need to implement
exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "BLEManagerImpl::SendIndication() failed: %s", ErrorStr(err));
        return false;
    }

    return true;
}

bool BLEManagerImpl::SendWriteRequest(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                      PacketBufferHandle pBuf)
{
    ChipLogProgress(DeviceLayer, "BLEManagerImpl::SendWriteRequest() not supported");
    return false;
}

bool BLEManagerImpl::SendReadRequest(BLE_CONNECTION_OBJECT conId, const ChipBleUUID * svcId, const ChipBleUUID * charId,
                                     PacketBufferHandle pBuf)
{
    ChipLogProgress(DeviceLayer, "BLEManagerImpl::SendReadRequest() not supported");
    return false;
}

bool BLEManagerImpl::SendReadResponse(BLE_CONNECTION_OBJECT conId, BLE_READ_REQUEST_CONTEXT requestContext,
                                      const ChipBleUUID * svcId, const ChipBleUUID * charId)
{
    ChipLogProgress(DeviceLayer, "BLEManagerImpl::SendReadResponse() not supported");
    return false;
}

void BLEManagerImpl::NotifyChipConnectionClosed(BLE_CONNECTION_OBJECT conId)
{
    // Nothing to do
}
void BLEManagerImpl::_BLETaskMain(void * arg)
{
    ChipLogDetail(DeviceLayer, "BLE task running");

    while (true)
    {
        vTaskDelay(1000);
    }
}



#if CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING
CHIP_ERROR BLEManagerImpl::EncodeAdditionalDataTlv()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    BitFlags<AdditionalDataFields> additionalDataFields;
    AdditionalDataPayloadGeneratorParams additionalDataPayloadParams;

#if CHIP_ENABLE_ROTATING_DEVICE_ID && defined(CHIP_DEVICE_CONFIG_ROTATING_DEVICE_ID_UNIQUE_ID)
    uint8_t rotatingDeviceIdUniqueId[ConfigurationManager::kRotatingDeviceIDUniqueIDLength] = {};
    MutableByteSpan rotatingDeviceIdUniqueIdSpan(rotatingDeviceIdUniqueId);

    err = DeviceLayer::GetDeviceInstanceInfoProvider()->GetRotatingDeviceIdUniqueId(rotatingDeviceIdUniqueIdSpan);
    SuccessOrExit(err);
    err = ConfigurationMgr().GetLifetimeCounter(additionalDataPayloadParams.rotatingDeviceIdLifetimeCounter);
    SuccessOrExit(err);
    additionalDataPayloadParams.rotatingDeviceIdUniqueId = rotatingDeviceIdUniqueIdSpan;
    additionalDataFields.Set(AdditionalDataFields::RotatingDeviceId);
#endif /* CHIP_ENABLE_ROTATING_DEVICE_ID && defined(CHIP_DEVICE_CONFIG_ROTATING_DEVICE_ID_UNIQUE_ID) */

    err = AdditionalDataPayloadGenerator().generateAdditionalDataPayload(additionalDataPayloadParams, c3AdditionalDataBufferHandle,
                                                                         additionalDataFields);

exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Failed to generate TLV encoded Additional Data (%s)", __func__);
    }

    return err;
}


#endif // CHIP_ENABLE_ADDITIONAL_DATA_ADVERTISING

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip
#endif // CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE

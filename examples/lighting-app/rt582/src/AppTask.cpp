/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
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

#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/on-off-server/on-off-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

#include <assert.h>

#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

#include "queue.h"
// #include <platform/RT582/freertos_bluetooth.h>

#include <lib/support/CodeUtils.h>

#include <platform/CHIPDeviceLayer.h>

#include "util_log.h"

using namespace chip;
using namespace ::chip::DeviceLayer;

#define APP_TASK_STACK_SIZE (3 * 1024)
#define APP_TASK_PRIORITY 2
#define APP_EVENT_QUEUE_SIZE 10

// NOTE! This key is for test/certification only and should not be available in production devices!
// If CONFIG_CHIP_FACTORY_DATA is enabled, this value is read from the factory data.
uint8_t sTestEventTriggerEnableKey[TestEventTriggerDelegate::kEnableKeyLength] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                                                                                   0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };


namespace {
TaskHandle_t sAppTaskHandle;
QueueHandle_t sAppEventQueue;

uint8_t sAppEventQueueBuffer[APP_EVENT_QUEUE_SIZE * sizeof(AppEvent)];

StaticQueue_t sAppEventQueueStruct;

StackType_t appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
StaticTask_t appTaskStruct;


EmberAfIdentifyEffectIdentifier sIdentifyEffect = EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT;
/**********************************************************
 * Identify Callbacks
 *********************************************************/

namespace {
void OnTriggerIdentifyEffectCompleted(chip::System::Layer * systemLayer, void * appState)
{
    ChipLogProgress(Zcl, "Trigger Identify Complete");
    sIdentifyEffect = EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT;
}
} // namespace



void OnTriggerIdentifyEffect(Identify * identify)
{
    sIdentifyEffect = identify->mCurrentEffectIdentifier;

    if (identify->mCurrentEffectIdentifier == EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE)
    {
        ChipLogProgress(Zcl, "IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE - Not supported, use effect variant %d",
                        identify->mEffectVariant);
        sIdentifyEffect = static_cast<EmberAfIdentifyEffectIdentifier>(identify->mEffectVariant);
    }

    switch (sIdentifyEffect)
    {
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BLINK:
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BREATHE:
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_OKAY:
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(5), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        break;
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_FINISH_EFFECT:
        (void) chip::DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(1), OnTriggerIdentifyEffectCompleted,
                                                           identify);
        break;
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT:
        (void) chip::DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
        sIdentifyEffect = EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT;
        break;
    default:
        ChipLogProgress(Zcl, "No identifier effect");
    }
}
Identify gIdentify = {
    chip::EndpointId{ 1 },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStart"); },
    [](Identify *) { ChipLogProgress(Zcl, "onIdentifyStop"); },
    EMBER_ZCL_IDENTIFY_IDENTIFY_TYPE_VISIBLE_LED,
    OnTriggerIdentifyEffect,
};

} // namespace

using namespace chip::TLV;
using namespace ::chip::DeviceLayer;

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::Init()
{
    info("AppTask::Init Lighting-App\n");
    CHIP_ERROR err = CHIP_NO_ERROR;

    // CHIP_FACTORY_DATA
    ReturnErrorOnFailure(mFactoryDataProvider.Init());
    SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
    SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);
    SetCommissionableDataProvider(&mFactoryDataProvider);

    // Read EnableKey from the factory data.
    MutableByteSpan enableKey(sTestEventTriggerEnableKey);
    err = mFactoryDataProvider.GetEnableKey(enableKey);
    if (err != CHIP_NO_ERROR)
    {
        // LOG_ERR("mFactoryDataProvider.GetEnableKey() failed. Could not delegate a test event trigger");
        // memset(sTestEventTriggerEnableKey, 0, sizeof(sTestEventTriggerEnableKey));
    }

    err = LightMgr().Init();
    if (err != CHIP_NO_ERROR)
    {
        return err;
    }
    PrintOnboardingCodes(chip::RendezvousInformationFlag(chip::RendezvousInformationFlag::kBLE));

    return err;
}

CHIP_ERROR AppTask::StartAppTask()
{
    sAppEventQueue = xQueueCreateStatic(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent), 
                                    sAppEventQueueBuffer, &sAppEventQueueStruct);
    if (sAppEventQueue == nullptr)
    {
        ChipLogError(NotSpecified, "Failed to allocate app event queue");
        return CHIP_ERROR_NO_MEMORY;
    }

    // Start App task.
    sAppTaskHandle = xTaskCreateStatic(AppTaskMain, APP_TASK_NAME, 
                                    ArraySize(appStack), nullptr, 1, appStack, &appTaskStruct);
    if (sAppTaskHandle == nullptr)
    {
        return CHIP_ERROR_NO_MEMORY;
    }

    return CHIP_NO_ERROR;
}

void AppTask::InitServer(intptr_t arg)
{
}

void AppTask::AppTaskMain(void * pvParameter)
{
    AppEvent event;
    QueueHandle_t sAppEventQueue = *(static_cast<QueueHandle_t *>(pvParameter));

    CHIP_ERROR err = sAppTask.Init();
    if (err != CHIP_NO_ERROR)
    {
        return ;
    }

    while (true)
    {   
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, pdMS_TO_TICKS(10));
    }
}
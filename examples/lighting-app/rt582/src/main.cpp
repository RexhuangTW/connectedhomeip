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

#include <AppTask.h>

#include "AppConfig.h"
#include "init_rt582Platform.h"
// #include "sl_simple_button_instances.h"
// #include "sl_system_kernel.h"
#include <DeviceInfoProviderImpl.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <matter_config.h>
#ifdef EFR32_ATTESTATION_CREDENTIALS
#include <examples/platform/efr32/EFR32DeviceAttestationCreds.h>
#else
#include <credentials/examples/DeviceAttestationCredsExample.h>
#endif



#define BLE_DEV_NAME "SiLabs-Light"
using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::DeviceLayer;
using namespace ::chip::Credentials;

#define UNUSED_PARAMETER(a) (a = a)

#include <lib/core/CHIPError.h>

volatile int apperror_cnt;
static chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

// ================================================================================
// Main Code
// ================================================================================
int main(void)
{
    init_rt582Platform();

    info( "==================================================\n");
    info( "chip-rt582-light-example starting\n");
    info( "==================================================\n");

#if CONFIG_ENABLE_CHIP_SHELL
    chip::LaunchShell();
#endif

    info("StartAppTask...\n");
    if (AppTask::GetAppTask().StartAppTask() != CHIP_NO_ERROR)
    {
        err("GetAppTask().StartAppTask() failed\n");
    }

#if 0
    DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    CHIPDeviceManager & deviceMgr = CHIPDeviceManager::GetInstance();
    CHIP_ERROR error              = deviceMgr.Init(&EchoCallbacks);
    if (error != CHIP_NO_ERROR)
    {
        err("device.Init() failed: %s\n");
    }
#endif


#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    if (ThreadStackMgr().InitThreadStack() != CHIP_NO_ERROR)
    {
        err("Failed to initialize Thread stack\n");
    }
    if (ThreadStackMgr().StartThreadTask() != CHIP_NO_ERROR)
    {
        err("Failed to launch Thread task\n");
    }
#endif

    // Should never get here.
    
    return 0;
}

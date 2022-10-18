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
#include <DeviceInfoProviderImpl.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <matter_config.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>



#define BLE_DEV_NAME "Rafael-Light"
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
    chip::Platform::MemoryInit();
    
    if (PlatformMgr().InitChipStack()!= CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "PlatformMgr().InitChipStack() failed");
    }

    init_rt582Platform();
    info( "==================================================\n");
    info( "chip-rt582-light-example starting\n");
    info( "==================================================\n");

#if CONFIG_ENABLE_CHIP_SHELL
    chip::LaunchShell();
#endif

    if (ThreadStackMgr().InitThreadStack() != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "Failed to initialize Thread stack");
    }

#if CHIP_DEVICE_CONFIG_THREAD_FTD
    if (ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router) != CHIP_NO_ERROR) 
#else  // !CHIP_DEVICE_CONFIG_THREAD_FTD
    if (ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_MinimalEndDevice) != CHIP_NO_ERROR) 
#endif // CHIP_DEVICE_CONFIG_THREAD_FTD
    {
        ChipLogError(NotSpecified, "Failed to SetThreadDeviceType");
    }    

    if (PlatformMgr().StartEventLoopTask() != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "Error during PlatformMgr().StartEventLoopTask();");
    }
    
    if (ThreadStackMgr().StartThreadTask() != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "Failed to launch Thread task");
    }

    if (GetAppTask().StartAppTask() != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "GetAppTask().StartAppTask() failed");
    }
    
    vTaskStartScheduler();
    // Should never get here.
    return 0;
}

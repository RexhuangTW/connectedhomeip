/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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

/**
 *    @file
 *          Provides an implementation of the PlatformManager object
 *          for EFR32 platforms using the Silicon Labs EFR32 SDK.
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>
#include <crypto/CHIPCryptoPAL.h>
#include <platform/RT582/DiagnosticDataProviderImpl.h>
#include <platform/FreeRTOS/SystemTimeSupport.h>
#include <platform/KeyValueStoreManager.h>
#include <platform/PlatformManager.h>
#include <platform/internal/GenericPlatformManagerImpl_FreeRTOS.ipp>

#if CHIP_SYSTEM_CONFIG_USE_LWIP
#include <lwip/tcpip.h>
#endif // CHIP_SYSTEM_CONFIG_USE_LWIP

#include "AppConfig.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

#include "uart.h"

#include "cm3_mcu.h"


namespace chip {
namespace DeviceLayer {

PlatformManagerImpl PlatformManagerImpl::sInstance;

static int app_entropy_source(void * data, unsigned char * output, size_t len, size_t * olen)
{
    *olen = len;
    return 0;
}
static void init_default_pin_mux(void)
{
    int i, j;

    /*set all pin to gpio, except GPIO16, GPIO17 */
    for (i = 0; i < 32; i++)
    {
        if ((i != 16) && (i != 17))
        {
            pin_set_mode(i, MODE_GPIO);
        }
    }
    /*uart0 pinmux*/
    pin_set_mode(16, MODE_UART); /*GPIO16 as UART0 RX*/
    pin_set_mode(17, MODE_UART); /*GPIO17 as UART0 TX*/

    return;
}
CHIP_ERROR PlatformManagerImpl::_InitChipStack(void)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    // Initialize the configuration system.
    /*
        TODO        
    */

    NVIC_SetPriority(Uart0_IRQn, 0x06);
    NVIC_SetPriority(CommSubsystem_IRQn, 0x01);
    init_default_pin_mux();

    uartConsoleInit();

    dma_init();

    Internal::RT582Config::Init();

    ReturnErrorOnFailure(System::Clock::InitClock_RealTime());

    err = chip::Crypto::add_entropy_source(app_entropy_source, NULL, 16);
    SuccessOrExit(err);

    // Call _InitChipStack() on the generic implementation base class
    // to finish the initialization process.
    err = Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::_InitChipStack();
    SuccessOrExit(err);

exit:
    return err;
}

} // namespace DeviceLayer
} // namespace chip

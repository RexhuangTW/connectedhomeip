/*
 *  Copyright (c) 2021, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * @brief
 *   This file includes the platform-specific initializers.
 */
//=============================================================================
//                Include
//=============================================================================
#include OPENTHREAD_PROJECT_CORE_CONFIG_FILE
#include <assert.h>
#include <string.h>

#include "openthread-system.h"

#include "common/logging.hpp"

#include "cm3_mcu.h"
#include "project_config.h"

#include "platform-rt58x.h"

//=============================================================================
//                Functions
//=============================================================================
void otSysInit(int argc, char * argv[])
{
    OT_UNUSED_VARIABLE(argc);
    OT_UNUSED_VARIABLE(argv);

    rt58x_alarm_init();

    random_number_init();
#if (MODULE_ENABLE(SUPPORT_MATTER_CONCURRENT))
    rafael_rfb_init();
#endif
}

bool otSysPseudoResetWasRequested(void)
{
    return false;
}

void otSysDeinit(void) {}

void otSysProcessDrivers(otInstance * aInstance)
{
    // UartProcessReceive();
    rt58x_alarm_process(aInstance);
    platformRadioProcess(aInstance);
}

// void otSysEventSignalPending(void)
// {
//     //gpio_pin_toggle(22);
// }

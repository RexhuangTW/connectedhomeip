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
 *   This file implements the OpenThread platform abstraction for UART
 * communication.
 *
 */
#include <stddef.h>
#include <string.h>

#include "openthread-system.h"
#include <openthread-core-config.h>

#include "utils/code_utils.h"

#include "bsp.h"
#include "bsp_console.h"
#include "cm3_mcu.h"

void UartProcessReceive(void)
{
    #if 0
    uint32_t u32_byte_cnt = 0;
    uint8_t uart_buff[4];

    u32_byte_cnt = bsp_console_stdin_str((char *) uart_buff, 1);
    if (u32_byte_cnt)
    {
        otPlatUartReceived(uart_buff, u32_byte_cnt);
    }
    #endif
}

otError otPlatUartFlush(void)
{
    
   // bsp_console_stdio_flush();
    return OT_ERROR_NONE;
}

otError otPlatUartEnable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartDisable(void)
{
    return OT_ERROR_NONE;
}

otError otPlatUartSend(const uint8_t * aBuf, uint16_t aBufLength)
{

    uartConsoleWrite(aBuf, aBufLength);
    //bsp_console_stdout_string((char *) aBuf, aBufLength);

    otPlatUartSendDone();
exit:
    return OT_ERROR_NONE;
}

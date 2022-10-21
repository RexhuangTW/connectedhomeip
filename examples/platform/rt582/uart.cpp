/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include "AppConfig.h"
#include "matter_shell.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "uart.h"
#include <stddef.h>
#include <stdio.h>

#include "uart_retarget.h"
#include "bsp_uart_drv.h"

#include "util_log.h"
#include "util_printf.h"

#define UART_CACHE_SIZE 256
#define UART_CACHE_MASK (UART_CACHE_SIZE - 1)

typedef struct uart_io
{
    volatile uint32_t wr_idx;
    volatile uint32_t rd_idx;

    volatile uint32_t is_flushing;
    uint8_t uart_cache[UART_CACHE_SIZE];
} uart_io_t;

static uart_io_t g_uart_rx_io = {
    .wr_idx = 0,
    .rd_idx = 0,
};

void uart_isr_event_handle(void)
{
    // receive data
    do
    {
        uint32_t wr_pos = g_uart_rx_io.wr_idx;
        uint32_t rd_pos = g_uart_rx_io.rd_idx;
        uint32_t pos = 0;
        uint8_t value[4] = {0};
        uint32_t rx_len = sizeof(value);

        bsp_uart_drv_recv(0, value, &rx_len);

        if (rx_len)
        {
            pos = (wr_pos + 1) % UART_CACHE_SIZE;
            if (pos == rd_pos)
            {
                break;
            }

            g_uart_rx_io.uart_cache[wr_pos] = value[0];
            g_uart_rx_io.wr_idx = pos;

            chip::NotifyShellProcessFromISR();
        }
    } while (0);
    return;
}

void uartConsoleInit(void)
{
    int rval = 0;
    bsp_uart_config_t debug_console_drv_config;
    uart_retarget_desc_t t_retarget_desc;
    do
    {
        /*uart0 pinmux*/
        pin_set_mode(16, MODE_UART); /*GPIO16 as UART0 RX*/
        pin_set_mode(17, MODE_UART); /*GPIO17 as UART0 TX*/

        /*init debug console uart0, 8bits 1 stopbit, none parity, no flow control.*/
        debug_console_drv_config.baud_rate = UART_BAUDRATE_115200;
        debug_console_drv_config.word_length = UART_DATA_BITS_8;
        debug_console_drv_config.hwfc = UART_HWFC_DISABLED;
        debug_console_drv_config.parity = UART_PARITY_NONE;
        debug_console_drv_config.stop_bits = UART_STOPBIT_ONE;
        debug_console_drv_config.irq_priority = 6;

        rval = bsp_uart_drv_init(0, &debug_console_drv_config, uart_isr_event_handle);

        if (rval != 0)
        {
            break;
        }
        t_retarget_desc.uart_id = 0;

        uart_retarget_init(&t_retarget_desc);
    } while (0);

    utility_register_stdout(uart_retarget_stdout_char, uart_retarget_stdout_string);

    util_log_init();
}

int16_t uartConsoleWrite(const char *Buf, uint16_t BufLength)
{
    uart_retarget_stdout_string((char *)Buf, BufLength);
    return (int16_t)BufLength;
}

int16_t uartConsoleRead(char *Buf, uint16_t BufLength)
{

    int16_t byte_cnt = 0;
    uint32_t rd_pos = g_uart_rx_io.rd_idx;
    uint32_t wr_pos = g_uart_rx_io.wr_idx;

    g_uart_rx_io.is_flushing = 0;

    do
    {
        while (1)
        {
            if (g_uart_rx_io.is_flushing)
            {
                wr_pos = rd_pos = byte_cnt = 0;
                break;
            }
            if (rd_pos == wr_pos)
            {
                break;
            }
            if (BufLength == byte_cnt)
            {
                break;
            }

            Buf[byte_cnt++] = g_uart_rx_io.uart_cache[rd_pos];

            rd_pos = (rd_pos + 1) % UART_CACHE_SIZE;
        }
        g_uart_rx_io.rd_idx = rd_pos;
    } while (0);
    return byte_cnt;
}

#ifdef __cplusplus
}
#endif

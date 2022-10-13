/**
 * @file logging.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2022-04-26
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <openthread-core-config.h>
#include <openthread/config.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/logging.h>

#include <utils/logging_rtt.h>

#include "util_log.h"
#include "util_printf.h"

#if (OPENTHREAD_CONFIG_LOG_OUTPUT == OPENTHREAD_CONFIG_LOG_OUTPUT_PLATFORM_DEFINED)
void otPlatLog(otLogLevel aLogLevel, otLogRegion aLogRegion, const char *aFormat, ...)
{
    va_list ap;

    va_start(ap, aFormat);
    utility_vsprintf(0, 0, aFormat, ap);
    va_end(ap);
    info("\r\n");
}

#endif
/**
 * @file alarm.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2022-04-26
 *
 * @copyright Copyright (c) 2022
 *
 */

#include OPENTHREAD_PROJECT_CORE_CONFIG_FILE

#include "common/logging.hpp"
#include "openthread-system.h"
#include <assert.h>
#include <openthread/config.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "cm3_mcu.h"
#include "util_log.h"
#include "utils/code_utils.h"

static uint32_t sMsAlarm = 0;
static bool sIsRunning   = false;
static bool sMsisPending = false;
static bool sMsIsFired   = false;

static TimerHandle_t sAlarmTimer;
static otInstance * sInstance;

static void alarm_timer_handler(TimerHandle_t xTimer)
{
    sMsIsFired = true;
    otTaskletsSignalPending();
}

void rt58x_alarm_init()
{
    sAlarmTimer = xTimerCreate("ThAlarm_T", 1, false, NULL, alarm_timer_handler);
}

void rt58x_alarm_process(otInstance * aInstance)
{
    if (sMsIsFired)
    {
        sMsIsFired = false;
        otPlatAlarmMilliFired(aInstance);
    }
}

uint32_t otPlatTimeGetXtalAccuracy(void)
{
    return SystemCoreClock;
}

void otPlatAlarmMilliStartAt(otInstance * aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);

    int32_t remain       = 0;
    uint32_t now_counter = 0;

    now_counter = sys_now();
    sMsAlarm    = aT0 + aDt;
    remain      = (int32_t) (sMsAlarm - now_counter);

    sInstance = aInstance;

    if (xTimerIsTimerActive(sAlarmTimer))
    {
        xTimerStop(sAlarmTimer, 0);
    }

    if (remain <= 0)
    {
        sMsIsFired = true;
        otTaskletsSignalPending();
        // otPlatAlarmMilliFired(aInstance);
    }
    else
    {
        xTimerChangePeriod(sAlarmTimer, remain / portTICK_PERIOD_MS, 0);
    }
}
void otPlatAlarmMilliStop(otInstance * aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    if (xTimerIsTimerActive(sAlarmTimer))
    {
        xTimerStop(sAlarmTimer, 0);
    }
}

inline uint32_t otPlatAlarmMilliGetNow(void)
{
    return sys_now();
}
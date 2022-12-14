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

#include "openthread-system.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <openthread/config.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include "common/logging.hpp"

#include "utils/code_utils.h"

#include "cm3_mcu.h"

#include "util_log.h"

#define SYST_CSR (*((volatile uint32_t *)0xe000e010))
#define SYST_RVR (*((volatile uint32_t *)0xe000e014))
#define SYST_CVR (*((volatile uint32_t *)0xe000e018))
#define SYST_CALIB (*((volatile uint32_t *)0xe000e01c))

#define SYST_CSR_ENABLE_BIT (1UL << 0UL)
#define SYST_CSR_TICKINT_BIT (1UL << 1UL)
#define SYST_CSR_CLKSOURCE_BIT (1UL << 2UL)
#define SYST_CSR_COUNTFLAG_BIT (1UL << 16UL)

static uint32_t sMsAlarm = 0;
static bool sIsRunning = false;
static uint32_t sMsCounter = 0;

static uint32_t sUsCounter = 0;
static bool sUsIsRunning = false;
static uint32_t sUsAlarm = 0;

static inline void _timer_isr_handler(uint32_t timer_id)
{
    otSysEventSignalPending();
}

static inline void _timer_milli_handler(uint32_t timer_id)
{
    otSysEventSignalPending();
}

void rt58x_alarm_process(otInstance *aInstance)
{
    int32_t remaining;
    bool alarmMilliFired = false;
#if (OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE == 1)
    bool alarmMicroFired = false;
#endif

    if (sIsRunning)
    {
        sMsCounter = sys_now();
        remaining = (int32_t)(sMsAlarm - sMsCounter);
        // sUsCounter = rfb_port_rtc_time_read();
        // remaining = (int32_t)(sMsAlarm - (sUsCounter / 1000));
        if (remaining <= 0)
        {
            Timer_Stop(4);
            alarmMilliFired = true;
        }
    }

    if (alarmMilliFired)
    {
        sIsRunning = false;
        otPlatAlarmMilliFired(aInstance);
    }
#if (OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE == 1)
    if (sUsIsRunning)
    {
        sUsCounter = rfb_port_rtc_time_read();
        remaining = (int32_t)(sUsAlarm - sUsCounter);

        if (remaining <= 0)
        {
            sUsIsRunning = false;
            {
                otPlatAlarmMicroFired(aInstance);
            }
            Timer_Stop(0);
            alarmMicroFired = true;
        }
    }

    if (alarmMicroFired)
    {
        sUsIsRunning = false;
        otPlatAlarmMicroFired(aInstance);
    }
#endif
}
void SysTick_Handler(void)
{
    sUsCounter++;
}
void rt58x_alarm_init()
{
    timer_config_mode_t cfg;

    cfg.int_en = ENABLE;
    cfg.mode = TIMER_PERIODIC_MODE;
    cfg.prescale = TIMER_PRESCALE_32;

    Timer_Open(0, cfg, _timer_isr_handler);

    Timer_Int_Priority(0, 6);

    cfg.int_en = ENABLE;
    cfg.mode = TIMER_PERIODIC_MODE;
    cfg.prescale = TIMER_PRESCALE_1;

    Timer_Open(4, cfg, _timer_milli_handler);
    Timer_Int_Priority(4, 6);
}

uint32_t otPlatTimeGetXtalAccuracy(void)
{
    return SystemCoreClock;
}

#if (OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE == 1)
inline uint32_t otPlatAlarmMicroGetNow(void)
{
    sUsCounter = rfb_port_rtc_time_read();
    return sUsCounter;
}

void otPlatAlarmMicroStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);
    int32_t remain = 0;
    Timer_Stop(0);
    sUsCounter = rfb_port_rtc_time_read();

    sUsAlarm = aT0 + aDt;
    remain = (int32_t)(sUsAlarm - sUsCounter);
    sUsIsRunning = true;

    if (remain <= 0)
    {
        otSysEventSignalPending();
    }
    else
    {
        Timer_Start(0, (remain - 1));
    }
}

void otPlatAlarmMicroStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    sUsIsRunning = false;
    Timer_Stop(0);
}

#endif
void otPlatAlarmMilliStartAt(otInstance *aInstance, uint32_t aT0, uint32_t aDt)
{
    OT_UNUSED_VARIABLE(aInstance);
    int32_t remain = 0;
    Timer_Stop(4);
    sMsCounter = sys_now();
    sMsAlarm = aT0 + aDt;
    remain = (int32_t)(sMsAlarm - sMsCounter);
    sIsRunning = true;

    if (remain <= 0)
    {
        otSysEventSignalPending();
    }
    else
    {
        Timer_Start(4, (remain * 40) - 1);
    }
}
void otPlatAlarmMilliStop(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    sIsRunning = false;
    Timer_Stop(4);
}

inline uint32_t otPlatAlarmMilliGetNow(void)
{
    sMsCounter = sys_now();
    return sMsCounter;
}
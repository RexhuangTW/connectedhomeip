/**
 * @file entropy.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2022-04-26
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <openthread/platform/entropy.h>

#include <openthread/platform/radio.h>

#include "platform-rt58x.h"
#include "utils/code_utils.h"


extern uint32_t get_random_number(void);
extern uint32_t otPlatAlarmMilliGetNow(void);
static uint32_t sRandomSeed = 0;

static void generateRandom(uint8_t *aOutput, uint16_t aOutputLength)
{
    for (uint16_t index = 0; index < aOutputLength; index++)
    {
        aOutput[index] = 0;

        for (uint8_t offset = 0; offset < 8 * sizeof(uint8_t); offset++)
        {
            //aOutput[index] <<= 1;
            aOutput[index] = (otPlatAlarmMilliGetNow() + (sRandomSeed * 1103515245) + 12345678) % 0xFF ;
        }
    }
}
void random_number_init(void)
{
    sRandomSeed = get_random_number();
}
otError otPlatEntropyGet(uint8_t *aOutput, uint16_t aOutputLength)
{
    otError error     = OT_ERROR_NONE;

    otEXPECT_ACTION(aOutput, error = OT_ERROR_INVALID_ARGS);

    generateRandom(aOutput, aOutputLength);

exit:
    return error;
}

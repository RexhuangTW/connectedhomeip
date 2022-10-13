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
 *          Utilities for accessing persisted device configuration on
 *          platforms based on the Silicon Labs SDK.
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/RT582/RT582Config.h>

#include <lib/core/CHIPEncoding.h>
#include <platform/internal/testing/ConfigUnitTest.h>

#include "FreeRTOS.h"

#define RT582_SEM_TIMEOUT_ms 5
static SemaphoreHandle_t nvm_Sem;
static StaticSemaphore_t nvm_SemStruct;

void nvm3_lockBegin(void)
{
    VerifyOrDie(nvm_Sem != NULL);
    xSemaphoreTake(nvm_Sem, RT582_SEM_TIMEOUT_ms);
}

void nvm3_lockEnd(void)
{
    VerifyOrDie(nvm_Sem != NULL);
    xSemaphoreGive(nvm_Sem);
}

namespace chip {
namespace DeviceLayer {
namespace Internal {

// Matter NVM3 space is placed in the silabs default nvm3 section shared with other stack.
// 'kMatterNvm3KeyDomain' identify the matter nvm3 domain.
// The NVM3 default section is placed at end of Flash minus 1 page byt the linker file
// See examples/platform/efr32/ldscripts/efr32mgXX.ld

CHIP_ERROR RT582Config::Init()
{
    nvm_Sem = xSemaphoreCreateBinaryStatic(&nvm_SemStruct);

    if (nvm_Sem == NULL)
    {
        return CHIP_ERROR_NO_MEMORY;
    }

    return ChipError(0);
}

void RT582Config::DeInit()
{
    vSemaphoreDelete(nvm_Sem);
}

CHIP_ERROR RT582Config::ReadConfigValue(Key key, bool & val)
{
    CHIP_ERROR err;
    uint32_t objectType;
    size_t dataLen;
    bool tmpVal;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    // Get nvm3 object info.

    // Read nvm3 bytes into tmp.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::ReadConfigValue(Key key, uint32_t & val)
{
    CHIP_ERROR err;
    uint32_t objectType;
    size_t dataLen;
    uint32_t tmpVal;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    // Get nvm3 object info.


    // Read nvm3 bytes into tmp.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::ReadConfigValue(Key key, uint64_t & val)
{
    CHIP_ERROR err;
    uint32_t objectType;
    size_t dataLen;
    uint64_t tmpVal;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    // Get nvm3 object info.


    // Read nvm3 bytes into tmp.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    CHIP_ERROR err;
    uint32_t objectType;
    size_t dataLen;

    outLen = 0;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    // Get nvm3 object info.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    CHIP_ERROR err;
    uint32_t objectType;
    size_t dataLen;

    outLen = 0;
    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    // Get nvm3 object info.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen, size_t offset)
{
    CHIP_ERROR err;
    uint32_t objectType;
    size_t dataLen;

    outLen = 0;
    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    // Get nvm3 object info.
exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::ReadConfigValueCounter(uint8_t counterIdx, uint32_t & val)
{
    CHIP_ERROR err;
    uint32_t tmpVal;
    Key key = kMinConfigKey_MatterCounter + counterIdx;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    // Read bytes into tmp.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::WriteConfigValue(Key key, bool val)
{
    CHIP_ERROR err;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_ERROR_INVALID_ARGUMENT); // Verify key id.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::WriteConfigValue(Key key, uint32_t val)
{
    CHIP_ERROR err;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::WriteConfigValue(Key key, uint64_t val)
{
    CHIP_ERROR err;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::WriteConfigValueStr(Key key, const char * str)
{
    return WriteConfigValueStr(key, str, (str != NULL) ? strlen(str) : 0);
}

CHIP_ERROR RT582Config::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    CHIP_ERROR err = CHIP_ERROR_INVALID_ARGUMENT;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    if (str != NULL)
    {
        // Write the string to nvm3 without the terminator char (apart from
        // empty strings where only the terminator char is stored in nvm3).
    }

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    CHIP_ERROR err = CHIP_ERROR_INVALID_ARGUMENT;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

    // Only write NULL pointer if the given size is 0, since in that case, nothing is read at the pointer
    if ((data != NULL) || (dataLen == 0))
    {
        // Write the binary data to nvm3.
    }

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::WriteConfigValueCounter(uint8_t counterIdx, uint32_t val)
{
    CHIP_ERROR err;
    Key key = kMinConfigKey_MatterCounter + counterIdx;

    VerifyOrExit(ValidConfigKey(key), err = CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND); // Verify key id.

exit:
    return ChipError(0);
}

CHIP_ERROR RT582Config::ClearConfigValue(Key key)
{
    CHIP_ERROR err;

    // Delete the nvm3 object with the given key id.

exit:
    return ChipError(0);
}

bool RT582Config::ConfigValueExists(Key key)
{
    uint32_t objectType;
    size_t dataLen;

    // Find object with key id.
    return 0;
}

bool RT582Config::ConfigValueExists(Key key, size_t & dataLen)
{
    uint32_t objectType;
    size_t dLen;

    // Find object with key id.

    return 0;
}

CHIP_ERROR RT582Config::FactoryResetConfig(void)
{
    // Deletes all nvm3 'Config' type objects.
    // Note- 'Factory' and 'Counter' type nvm3 objects are NOT deleted.

    CHIP_ERROR err;

    // Iterate over all the CHIP Config nvm3 records and delete each one...

    return ChipError(0);
}

// CHIP_ERROR RT582Config::MapNvm3Error(Ecode_t nvm3Res)
// {
//     CHIP_ERROR err;

//     return ChipError(0);
// }

// CHIP_ERROR RT582Config::ForEachRecord(Key firstNvm3Key, Key lastNvm3Key, bool addNewRecord, ForEachRecordFunct funct)
// {
//     // Iterates through the specified range of nvm3 object key ids.
//     // Invokes the callers CB function when appropriate.

//     CHIP_ERROR err = CHIP_NO_ERROR;

//     return ChipError(0);
// }

bool RT582Config::ValidConfigKey(Key key)
{
    // Returns true if the key is in the Matter nvm3 reserved key range.
    // Additional check validates that the user consciously defined the expected key range
    // if ((key >= kMatterNvm3KeyLoLimit) && (key <= kMatterNvm3KeyHiLimit) && (key >= kMinConfigKey_MatterFactory) &&
    //     (key <= kMaxConfigKey_MatterKvs))
    // {
    //     return true;
    // }

    return false;
}

void RT582Config::RunConfigUnitTest()
{
    // Run common unit test.
    ::chip::DeviceLayer::Internal::RunConfigUnitTest<RT582Config>();
}

void RT582Config::RepackNvm3Flash(void)
{
    // Repack nvm3 flash if nvm3 space < headroom threshold.
    // Note- checking periodically during idle periods should prevent
    // forced repack events on any write operation.
}
CHIP_ERROR RT582Config::xx1233(uint32_t t123t)
{
    return ChipError(0);
}
} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

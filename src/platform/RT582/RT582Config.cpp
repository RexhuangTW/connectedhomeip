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

#include "util_log.h"
#include "cm3_mcu.h"

#define RT582CONFIG_BASE_ADDR       0xF4000
#define RT582CONFIG_MAX_ID          0x80
#define RT582CONFIG_ID_PER_SIZE     0x100
#define RT582CONFIG_SECTOR_SIZE     0x1000

namespace chip {
namespace DeviceLayer {
namespace Internal {

static uint8_t storage_backup[RT582CONFIG_SECTOR_SIZE];
static size_t storage_read(uint32_t id, size_t bufSize, uint8_t *buf)
{
    uint32_t sector_addr, id_addr, i, offset, chk, data_size;

    id_addr = RT582CONFIG_BASE_ADDR+(RT582CONFIG_ID_PER_SIZE*id);
    sector_addr = id_addr - (id_addr % RT582CONFIG_SECTOR_SIZE);

    for(i=0;i<16;i++)
    {
        flash_read_page_syncmode((uint32_t)&storage_backup[i*RT582CONFIG_ID_PER_SIZE], (sector_addr+(i*RT582CONFIG_ID_PER_SIZE)));
    }
    offset = (RT582CONFIG_ID_PER_SIZE*id) % 1000;

    if((bufSize < storage_backup[offset]) && (storage_backup[offset] != 0xFF))
    {
        ChipLogDetail(DeviceLayer, "Cfg  %s !!!!!!!!!!", __func__); 
        return 0xFFFFFFFF;
    }

    if(storage_backup[offset] == 0xFF)
        return 0;

    memcpy(buf, &storage_backup[offset+1], storage_backup[offset]);

    //if(chk == (bufSize-1))
    //    return 0;
        

    return storage_backup[offset];
}

size_t storage_write(uint32_t id, size_t dataLen, uint8_t *data)
{
    uint32_t sector_addr, id_addr, i, offset;

    id_addr = RT582CONFIG_BASE_ADDR+(RT582CONFIG_ID_PER_SIZE*id);
    sector_addr = id_addr - (id_addr % RT582CONFIG_SECTOR_SIZE);

    for(i=0;i<16;i++)
    {
        flash_read_page_syncmode((uint32_t)&storage_backup[i*RT582CONFIG_ID_PER_SIZE], (sector_addr+(i*RT582CONFIG_ID_PER_SIZE)));
    }

    flash_erase(FLASH_ERASE_SECTOR, sector_addr);

    offset = (RT582CONFIG_ID_PER_SIZE*id) % 1000;

    storage_backup[offset] = dataLen;
    memcpy(&storage_backup[offset+1], data, dataLen);

    for(i=0;i<16;i++)
    {
        flash_write_page(
            (uint32_t)&storage_backup[i*RT582CONFIG_ID_PER_SIZE], 
            (sector_addr+(i*RT582CONFIG_ID_PER_SIZE)));
    }

    for(i=0;i<16;i++)
    {
        flash_read_page_syncmode((uint32_t)&storage_backup[i*RT582CONFIG_ID_PER_SIZE], (sector_addr+(i*RT582CONFIG_ID_PER_SIZE)));
    }
    return dataLen;
}

void storage_erase(void)
{
    flash_erase(FLASH_ERASE_SECTOR, RT582CONFIG_BASE_ADDR);
    flash_erase(FLASH_ERASE_SECTOR, RT582CONFIG_BASE_ADDR + 0x1000);
    flash_erase(FLASH_ERASE_SECTOR, RT582CONFIG_BASE_ADDR + 0x2000);
    flash_erase(FLASH_ERASE_SECTOR, RT582CONFIG_BASE_ADDR + 0x3000);
}

CHIP_ERROR RT582Config::Init()
{
    //storage_erase();
    return ChipError(0);
}

template <typename T>
CHIP_ERROR RT582Config::ReadConfigValue(Key key, T & val)
{
    size_t read_count;
    ReturnErrorOnFailure(ReadConfigValueBin(key, &val, sizeof(val), read_count));
    VerifyOrReturnError(sizeof(val) == read_count, CHIP_ERROR_PERSISTED_STORAGE_FAILED);

    return CHIP_NO_ERROR;
}

CHIP_ERROR RT582Config::ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen)
{
    return ReadConfigValueBin(key, buf, bufSize, outLen);
}

CHIP_ERROR RT582Config::ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen)
{
    return ReadConfigValueBin(key, static_cast<void *>(buf), bufSize, outLen);
}

CHIP_ERROR RT582Config::ReadConfigValueBin(Key key, void * buf, size_t bufSize, size_t & outLen)
{
    //ChipLogDetail(DeviceLayer, "Cfg  %s", __func__); 
    outLen = storage_read(key, bufSize, (uint8_t *)buf);

    if(outLen == 0)
       return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;

    else if(outLen == 0xFFFFFFFF)
        return CHIP_ERROR_BUFFER_TOO_SMALL;

    return CHIP_NO_ERROR;
}
template CHIP_ERROR RT582Config::ReadConfigValue(Key key, bool & val);
template CHIP_ERROR RT582Config::ReadConfigValue(Key key, uint32_t & val);
template CHIP_ERROR RT582Config::ReadConfigValue(Key key, uint64_t & val);

template <typename T>
CHIP_ERROR RT582Config::WriteConfigValue(Key key, T val)
{
    return WriteConfigValueBin(key, &val, sizeof(val));
}


template CHIP_ERROR RT582Config::WriteConfigValue(Key key, bool val);
template CHIP_ERROR RT582Config::WriteConfigValue(Key key, uint32_t val);
template CHIP_ERROR RT582Config::WriteConfigValue(Key key, uint64_t val);

CHIP_ERROR RT582Config::WriteConfigValueStr(Key key, const char * str)
{
    return WriteConfigValueStr(key, str, (str != NULL) ? strlen(str) : 0);
}

CHIP_ERROR RT582Config::WriteConfigValueStr(Key key, const char * str, size_t strLen)
{
    return WriteConfigValueBin(key, str, strLen);
}

CHIP_ERROR RT582Config::WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen)
{
    return WriteConfigValueBin(key, static_cast<const void *>(data), dataLen);
}

CHIP_ERROR RT582Config::WriteConfigValueBin(Key key, const void * data, size_t dataLen)
{
    if (dataLen >= 0xFF)
    {
        err("write error, data length: %u\n", dataLen);
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    storage_write(key, dataLen, (uint8_t *)data);
    return CHIP_NO_ERROR;
}

CHIP_ERROR RT582Config::ClearConfigValue(Key key)
{
    storage_erase();
    return CHIP_NO_ERROR;
}

bool RT582Config::ConfigValueExists(Key key)
{
    uint8_t val;

    return ChipError::IsSuccess(ReadConfigValue(key, val));
}


CHIP_ERROR RT582Config::FactoryResetConfig(void)
{
    for (Key key = kMinConfigKey_MatterConfig; key <= kMaxConfigKey_MatterConfig; key++)
        ClearConfigValue(key);
    return CHIP_NO_ERROR;
}


void RT582Config::RunConfigUnitTest()
{
    // Run common unit test.
    //::chip::DeviceLayer::Internal::RunConfigUnitTest<RT582Config>();
}
} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

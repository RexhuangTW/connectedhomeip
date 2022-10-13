/*
 *
 *    Copyright (c) 2020-2022 Project CHIP Authors
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

#pragma once

#include <functional>

#include <platform/internal/CHIPDeviceLayerInternal.h>


#ifndef KVS_MAX_ENTRIES
#define KVS_MAX_ENTRIES 75 // Available key slot count for Kvs Key mapping.
#endif

// Delay before Key/Value is actually saved in NVM
#define RT582_KVS_SAVE_DELAY_SECONDS 5

static_assert((KVS_MAX_ENTRIES <= 255), "Implementation supports up to 255 Kvs entries");
static_assert((KVS_MAX_ENTRIES >= 30), "Mininimal Kvs entries requirement is not met");

namespace chip {
namespace DeviceLayer {
namespace Internal {


constexpr inline uint16_t RT582ConfigKey(uint8_t chipId, uint8_t pdmId)
{
    return static_cast<uint16_t>(chipId) << 8 | pdmId;
}

class RT582Config
{
public:
    // Definitions for Silicon Labs EFR32 NVM3 driver:-

    using Key = uint32_t;

    // NVM3 key base offsets used by the CHIP Device Layer.
    // ** Key base can range from 0x72 to 0x7F **
    // Persistent config values set at manufacturing time. Retained during factory reset.
    static constexpr uint8_t kMatterFactory_KeyBase = 0x72;
    // Persistent config values set at runtime. Cleared during factory reset.
    static constexpr uint8_t kMatterConfig_KeyBase = 0x73;
    // Persistent counter values set at runtime. Retained during factory reset.
    static constexpr uint8_t kMatterCounter_KeyBase = 0x74;
    // Persistent config values set at runtime. Cleared during factory reset.
    static constexpr uint8_t kMatterKvs_KeyBase = 0x75;

    // Key definitions for well-known configuration values.
    // Factory config keys
    static constexpr Key kConfigKey_SerialNum             = RT582ConfigKey(kMatterFactory_KeyBase, 0x00);
    static constexpr Key kConfigKey_MfrDeviceId           = RT582ConfigKey(kMatterFactory_KeyBase, 0x01);
    static constexpr Key kConfigKey_MfrDeviceCert         = RT582ConfigKey(kMatterFactory_KeyBase, 0x02);
    static constexpr Key kConfigKey_MfrDevicePrivateKey   = RT582ConfigKey(kMatterFactory_KeyBase, 0x03);
    static constexpr Key kConfigKey_ManufacturingDate     = RT582ConfigKey(kMatterFactory_KeyBase, 0x04);
    static constexpr Key kConfigKey_SetupPinCode          = RT582ConfigKey(kMatterFactory_KeyBase, 0x05);
    static constexpr Key kConfigKey_MfrDeviceICACerts     = RT582ConfigKey(kMatterFactory_KeyBase, 0x06);
    static constexpr Key kConfigKey_SetupDiscriminator    = RT582ConfigKey(kMatterFactory_KeyBase, 0x07);
    static constexpr Key kConfigKey_Spake2pIterationCount = RT582ConfigKey(kMatterFactory_KeyBase, 0x08);
    static constexpr Key kConfigKey_Spake2pSalt           = RT582ConfigKey(kMatterFactory_KeyBase, 0x09);
    static constexpr Key kConfigKey_Spake2pVerifier       = RT582ConfigKey(kMatterFactory_KeyBase, 0x0A);
    // Matter Config Keys
    static constexpr Key kConfigKey_ServiceConfig      = RT582ConfigKey(kMatterConfig_KeyBase, 0x01);
    static constexpr Key kConfigKey_PairedAccountId    = RT582ConfigKey(kMatterConfig_KeyBase, 0x02);
    static constexpr Key kConfigKey_ServiceId          = RT582ConfigKey(kMatterConfig_KeyBase, 0x03);
    static constexpr Key kConfigKey_LastUsedEpochKeyId = RT582ConfigKey(kMatterConfig_KeyBase, 0x05);
    static constexpr Key kConfigKey_FailSafeArmed      = RT582ConfigKey(kMatterConfig_KeyBase, 0x06);
    static constexpr Key kConfigKey_GroupKey           = RT582ConfigKey(kMatterConfig_KeyBase, 0x07);
    static constexpr Key kConfigKey_HardwareVersion    = RT582ConfigKey(kMatterConfig_KeyBase, 0x08);
    static constexpr Key kConfigKey_RegulatoryLocation = RT582ConfigKey(kMatterConfig_KeyBase, 0x09);
    static constexpr Key kConfigKey_CountryCode        = RT582ConfigKey(kMatterConfig_KeyBase, 0x0A);
    static constexpr Key kConfigKey_WiFiSSID           = RT582ConfigKey(kMatterConfig_KeyBase, 0x0C);
    static constexpr Key kConfigKey_WiFiPSK            = RT582ConfigKey(kMatterConfig_KeyBase, 0x0D);
    static constexpr Key kConfigKey_WiFiSEC            = RT582ConfigKey(kMatterConfig_KeyBase, 0x0E);
    static constexpr Key kConfigKey_GroupKeyBase       = RT582ConfigKey(kMatterConfig_KeyBase, 0x0F);
    static constexpr Key kConfigKey_LockUser           = RT582ConfigKey(kMatterConfig_KeyBase, 0x10);
    static constexpr Key kConfigKey_Credential         = RT582ConfigKey(kMatterConfig_KeyBase, 0x11);
    static constexpr Key kConfigKey_LockUserName       = RT582ConfigKey(kMatterConfig_KeyBase, 0x12);
    static constexpr Key kConfigKey_CredentialData     = RT582ConfigKey(kMatterConfig_KeyBase, 0x13);
    static constexpr Key kConfigKey_UserCredentials    = RT582ConfigKey(kMatterConfig_KeyBase, 0x14);
    static constexpr Key kConfigKey_WeekDaySchedules   = RT582ConfigKey(kMatterConfig_KeyBase, 0x15);
    static constexpr Key kConfigKey_YearDaySchedules   = RT582ConfigKey(kMatterConfig_KeyBase, 0x16);
    static constexpr Key kConfigKey_HolidaySchedules   = RT582ConfigKey(kMatterConfig_KeyBase, 0x17);

    static constexpr Key kConfigKey_GroupKeyMax =
        RT582ConfigKey(kMatterConfig_KeyBase, 0x1E); // Allows 16 Group Keys to be created.
    static constexpr Key kConfigKey_UniqueId = RT582ConfigKey(kMatterFactory_KeyBase, 0x1F);
    static constexpr Key kConfigKey_OpKeyMap = RT582ConfigKey(kMatterConfig_KeyBase, 0x20);

    // Matter Counter Keys
    static constexpr Key kConfigKey_BootCount             = RT582ConfigKey(kMatterCounter_KeyBase, 0x00);
    static constexpr Key kConfigKey_TotalOperationalHours = RT582ConfigKey(kMatterCounter_KeyBase, 0x01);
    static constexpr Key kConfigKey_LifeTimeCounter       = RT582ConfigKey(kMatterCounter_KeyBase, 0x02);

    // Matter KVS storage Keys
    static constexpr Key kConfigKey_KvsStringKeyMap = RT582ConfigKey(kMatterKvs_KeyBase, 0x00);
    static constexpr Key kConfigKey_KvsFirstKeySlot = RT582ConfigKey(kMatterKvs_KeyBase, 0x01);
    static constexpr Key kConfigKey_KvsLastKeySlot  = RT582ConfigKey(kMatterKvs_KeyBase, KVS_MAX_ENTRIES);

    // Set key id limits for each group.
    static constexpr Key kMinConfigKey_MatterFactory = RT582ConfigKey(kMatterFactory_KeyBase, 0x00);
    static constexpr Key kMaxConfigKey_MatterFactory = RT582ConfigKey(kMatterFactory_KeyBase, 0x0A);
    static constexpr Key kMinConfigKey_MatterConfig  = RT582ConfigKey(kMatterConfig_KeyBase, 0x00);
    static constexpr Key kMaxConfigKey_MatterConfig  = RT582ConfigKey(kMatterConfig_KeyBase, 0x20);

    // Allows 32 Counters to be created.
    static constexpr Key kMinConfigKey_MatterCounter = RT582ConfigKey(kMatterCounter_KeyBase, 0x00);
    static constexpr Key kMaxConfigKey_MatterCounter = RT582ConfigKey(kMatterCounter_KeyBase, 0x1F);

    static constexpr Key kMinConfigKey_MatterKvs = RT582ConfigKey(kMatterKvs_KeyBase, 0x00);
    static constexpr Key kMaxConfigKey_MatterKvs = RT582ConfigKey(kMatterKvs_KeyBase, 0xFF);

    static CHIP_ERROR Init(void);
    static void DeInit(void);

    // Configuration methods used by the GenericConfigurationManagerImpl<> template.
    static CHIP_ERROR ReadConfigValue(Key key, bool & val);
    static CHIP_ERROR ReadConfigValue(Key key, uint32_t & val);
    static CHIP_ERROR ReadConfigValue(Key key, uint64_t & val);
    static CHIP_ERROR ReadConfigValueStr(Key key, char * buf, size_t bufSize, size_t & outLen);
    static CHIP_ERROR ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen);
    static CHIP_ERROR ReadConfigValueBin(Key key, uint8_t * buf, size_t bufSize, size_t & outLen, size_t offset);
    static CHIP_ERROR ReadConfigValueCounter(uint8_t counterIdx, uint32_t & val);
    static CHIP_ERROR WriteConfigValue(Key key, bool val);
    static CHIP_ERROR WriteConfigValue(Key key, uint32_t val);
    static CHIP_ERROR WriteConfigValue(Key key, uint64_t val);
    static CHIP_ERROR WriteConfigValueStr(Key key, const char * str);
    static CHIP_ERROR WriteConfigValueStr(Key key, const char * str, size_t strLen);
    static CHIP_ERROR WriteConfigValueBin(Key key, const uint8_t * data, size_t dataLen);
    static CHIP_ERROR WriteConfigValueCounter(uint8_t counterIdx, uint32_t val);
    static CHIP_ERROR ClearConfigValue(Key key);
    static bool ConfigValueExists(Key key);
    static bool ConfigValueExists(Key key, size_t & dataLen);
    static CHIP_ERROR FactoryResetConfig(void);
    static bool ValidConfigKey(Key key);

    static void RunConfigUnitTest(void);
    static void RepackNvm3Flash(void);

protected:
    static constexpr uint8_t GetPDMId(uint32_t key);
    static constexpr uint8_t GetRecordKey(uint32_t key);

private:
    static CHIP_ERROR xx1233(uint32_t t123t);
};
/**
 * Extract a PDM id from a Key value.
 */
inline constexpr uint8_t RT582Config::GetPDMId(Key key)
{
    return static_cast<uint8_t>(key >> 8);
}

/**
 * Extract an NVM record key from a Key value.
 */
inline constexpr uint8_t RT582Config::GetRecordKey(Key key)
{
    return static_cast<uint8_t>(key);
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace chip

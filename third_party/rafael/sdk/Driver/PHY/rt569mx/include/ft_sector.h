/**************************************************************************//**
 * @file     ft_sector.h
 * @version
 * @brief    FT sector driver

 ******************************************************************************/
#ifndef _FT_SECTOR_H
#define _FT_SECTOR_H

/*******************************************************************************
*   INCLUDES
*******************************************************************************/
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)
#if ((RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_B))
#if (RF_MCU_FW_TARGET == RF_MCU_FW_TARGET_MAC)
#include "rt569mxb_init.h"
#elif (RF_MCU_FW_TARGET == RF_MCU_FW_TARGET_BLE)
#include "rt569mxb_ble_init.h"
#endif
#endif
#endif

/*******************************************************************************
*   CONSTANT AND DEFINE
*******************************************************************************/
#define FT_SECTOR_IC_VERSION_OFS            (0x00)
#define FT_SECTOR_FT_VERSION_OFS            (0x10)
#define FT_SECTOR_DCDC_OFS                  (0x20)
#define FT_SECTOR_LDOMV_OFS                 (0x30)
#define FT_SECTOR_LDOLV_OFS                 (0x40)
#define FT_SECTOR_SLDO_OFS                  (0x50)
#define FT_SECTOR_ADC_OFS                   (0x60)

#define FT_DCDC_TARGVOLT_LIGHT              (1350)
#define FT_DCDC_TARGVOLT_HEAVY_LOW_POWER    (1350)
#define FT_DCDC_TARGVOLT_HEAVY_HIGH_POWER   (1900)
#if (RF_TX_POWER_TYPE == RF_TX_POWER_0DBM)
#define FT_DCDC_TARGVOLT_HEAVY              FT_DCDC_TARGVOLT_HEAVY_LOW_POWER
#else
#define FT_DCDC_TARGVOLT_HEAVY              FT_DCDC_TARGVOLT_HEAVY_HIGH_POWER
#endif
#define FT_DCDC_TARGVOSEL_MIN               (0)
#define FT_DCDC_TARGVOSEL_MAX               (31)

#define FT_LDOMV_TARGVOLT_LIGHT             (1350)
#define FT_LDOMV_TARGVOLT_HEAVY_LOW_POWER   (1350)
#define FT_LDOMV_TARGVOLT_HEAVY_HIGH_POWER  (1900)
#if (RF_TX_POWER_TYPE == RF_TX_POWER_0DBM)
#define FT_LDOMV_TARGVOLT_HEAVY             FT_LDOMV_TARGVOLT_HEAVY_LOW_POWER
#else
#define FT_LDOMV_TARGVOLT_HEAVY             FT_LDOMV_TARGVOLT_HEAVY_HIGH_POWER
#endif
#define FT_LDOMV_TARGVOSEL_MIN              (0)
#define FT_LDOMV_TARGVOSEL_MAX              (31)

#define FT_LDOLV_TARGVOLT                   (1100)
#define FT_LDOLV_TARGVOSEL_MIN              (0)
#define FT_LDOLV_TARGVOSEL_MAX              (15)

#define FT_SLDO_TARGVOLT                    (800)
#define FT_SLDO_TARGVOSEL_MIN               (0)
#define FT_SLDO_TARGVOSEL_MAX               (15)
#define FT_SLDO_TARGVOSEL_USE_MIN           (5)


typedef enum
{
    FT_SECTOR_INVALID = 0,
    FT_SECTOR_VALID = 1,
} ft_sector_valid_t;

// Same as ePMU_MPK_STATUS in RF MCU
typedef enum
{
    PMU_MPK_NONE                = 0,
    PMU_MPK_XTAL                = BIT0,
    PMU_MPK_SLDO_VOSEL          = BIT1,
    PMU_MPK_LLDO_VOSEL          = BIT2,
    PMU_MPK_DCDC_VOSEL_H        = BIT3,
    PMU_MPK_LDOMV_VOSEL_H       = BIT4,
    PMU_MPK_DCDC_VOSEL_L        = BIT5,
    PMU_MPK_LDOMV_VOSEL_L       = BIT6,
    PMU_MPK_BG_OS               = BIT7,
    PMU_MPK_BG_OS_DIR           = BIT8,
} ft_sector_pmu_status_t;

typedef struct __attribute__((packed))
{
    uint8_t  flag;
    uint32_t ic_version;
}
ft_sector_ic_ver_t;

typedef struct __attribute__((packed))
{
    uint8_t  flag;
    uint32_t ft_version;
}
ft_sector_ft_ver_t;

typedef struct __attribute__((packed))
{
    uint8_t  flag;
    uint16_t voltage_0;
    uint8_t  vosel_0;
    uint16_t voltage_1;
    uint8_t  vosel_1;
}
ft_sector_volt_t;

typedef struct ft_volt_regulator
{
    ft_sector_volt_t    data;
    uint16_t            voltage_targ_0;
    uint16_t            voltage_targ_1;
    uint16_t            voltage_targ_2;
    uint8_t             vosel_targ_0;
    uint8_t             vosel_targ_1;
    uint8_t             vosel_targ_2;
} ft_volt_regulator_t;

typedef struct __attribute__((packed))
{
    uint8_t  flag;
    uint16_t adc_3v3;
    uint16_t adc_1v8;
    uint16_t adc_temp;
}
ft_sector_adc_t;

typedef struct ft_sector
{
    uint16_t                    status;
    ft_sector_ic_ver_t          ic_ver;                     // Offset: FT_SECTOR_IC_VERSION_OFS
    ft_sector_ft_ver_t          ft_ver;                     // Offset: FT_SECTOR_FT_VERSION_OFS
    ft_volt_regulator_t         dcdc;                       // Offset: FT_SECTOR_DCDC_OFS
    ft_volt_regulator_t         ldomv;                      // Offset: FT_SECTOR_LDOMV_OFS
    ft_volt_regulator_t         ldolv;                      // Offset: FT_SECTOR_LDOLV_OFS
    ft_volt_regulator_t         sldo;                       // Offset: FT_SECTOR_SLDO_OFS
    ft_sector_adc_t             adc;                        // Offset: FT_SECTOR_ADC_OFS
} ft_sector_t;

/******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************/
uint32_t Ft_FtInfoGet(ft_sector_t *p_ft_info, uint8_t dev_addr, uint16_t page_size, uint16_t max_size, uint8_t write_cycle,
                      uint32_t scl_pin, uint32_t sda_pin, uint32_t i2c_speed);

#endif  /* _FT_SECTOR_H */

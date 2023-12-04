/**************************************************************************//**
 * @file     ft_sector.c
 * @version
 * @brief    FT sector driver

 ******************************************************************************/

/*******************************************************************************
*   INCLUDES
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm3_mcu.h"
#include "eeprom_ctrl.h"
#include "ft_sector.h"
#include "Ruci.h"

/******************************************************************************
* DEFINES
******************************************************************************/
#define VOSEL_ROUND_TO_THE_NEAREST_VAL      0
#define VOSEL_ROUND_UP                      1
#define VOSEL_ROUND_DOWN                    2

/******************************************************************************
* GLOBAL PARAMETERS
******************************************************************************/
ft_sector_t *g_p_ft_info;

/******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************/
uint8_t Ft_PmuVoselCalc(ft_sector_volt_t *p_volt_info, uint16_t voltage_targ, uint8_t round_type)
{
    uint32_t vosel_targ;
    uint8_t value;

    switch (round_type)
    {
    default:
    case VOSEL_ROUND_TO_THE_NEAREST_VAL:
        value = 0x08;
        break;

    case VOSEL_ROUND_UP:
        value = 0x0f;
        break;

    case VOSEL_ROUND_DOWN:
        value = 0x00;
        break;
    }

    /* Need to round the result to the nearest integer */
    vosel_targ = (((uint32_t)p_volt_info->vosel_1 - p_volt_info->vosel_0) * ((uint32_t)voltage_targ - p_volt_info->voltage_0)) << 4;
    vosel_targ = ((int32_t)vosel_targ / (int32_t)((int32_t)p_volt_info->voltage_1 -  p_volt_info->voltage_0)) + ((uint32_t)p_volt_info->vosel_0 << 4);
    vosel_targ = (vosel_targ + value) >> 4;

    return (uint8_t)vosel_targ;
}

uint32_t Ft_PmuSettingCalc(ft_sector_t *p_ft_info)
{
    ft_volt_regulator_t *p_volt_tmp;
    uint32_t ret_status;

    ret_status = STATUS_ERROR;

    /* Check IC version fields */
    //    if (p_ft_info->ic_ver.flag != FT_SECTOR_VALID)
    //        goto DONE;

    /* Check FT version fields */
    //    if (p_ft_info->ft_ver.flag != FT_SECTOR_VALID)
    //        goto DONE;

    /* Check DCDC fields */
    p_volt_tmp = &p_ft_info->dcdc;
    if (p_volt_tmp->data.flag == FT_SECTOR_VALID)
    {
        /* Calculate DCDC vosel target value */
        p_volt_tmp->voltage_targ_0 = FT_DCDC_TARGVOLT_LIGHT;
        p_volt_tmp->vosel_targ_0 = Ft_PmuVoselCalc( &p_volt_tmp->data, p_volt_tmp->voltage_targ_0, VOSEL_ROUND_UP);
        if (p_volt_tmp->vosel_targ_0 <= FT_DCDC_TARGVOSEL_MAX)
        {
            p_ft_info->status |= PMU_MPK_DCDC_VOSEL_L;
        }

        p_volt_tmp->voltage_targ_1 = FT_DCDC_TARGVOLT_HEAVY;
        p_volt_tmp->vosel_targ_1 = Ft_PmuVoselCalc( &p_volt_tmp->data, p_volt_tmp->voltage_targ_1, VOSEL_ROUND_UP);
        if (p_volt_tmp->vosel_targ_1 <= FT_DCDC_TARGVOSEL_MAX)
        {
            p_ft_info->status |= PMU_MPK_DCDC_VOSEL_H;
        }
    }

    /* Check LDOMV fields */
    p_volt_tmp = &p_ft_info->ldomv;
    if (p_volt_tmp->data.flag == FT_SECTOR_VALID)
    {
        /* Calculate LDOMV vosel target value */
        p_volt_tmp->voltage_targ_0 = FT_LDOMV_TARGVOLT_LIGHT;
        p_volt_tmp->vosel_targ_0 = Ft_PmuVoselCalc( &p_volt_tmp->data, p_volt_tmp->voltage_targ_0, VOSEL_ROUND_UP);
        if (p_volt_tmp->vosel_targ_0 <= FT_LDOMV_TARGVOSEL_MAX)
        {
            p_ft_info->status |= PMU_MPK_LDOMV_VOSEL_L;
        }

        p_volt_tmp->voltage_targ_1 = FT_LDOMV_TARGVOLT_HEAVY;
        p_volt_tmp->vosel_targ_1 = Ft_PmuVoselCalc( &p_volt_tmp->data, p_volt_tmp->voltage_targ_1, VOSEL_ROUND_UP);
        if (p_volt_tmp->vosel_targ_1 <= FT_LDOMV_TARGVOSEL_MAX)
        {
            p_ft_info->status |= PMU_MPK_LDOMV_VOSEL_H;
        }
    }

    /* Check LDOLV fields */
    p_volt_tmp = &p_ft_info->ldolv;
    if (p_volt_tmp->data.flag == FT_SECTOR_VALID)
    {
        /* Calculate LDOLV vosel target value */
        p_volt_tmp->voltage_targ_0 = FT_LDOLV_TARGVOLT;
        p_volt_tmp->vosel_targ_0 = Ft_PmuVoselCalc( &p_volt_tmp->data, p_volt_tmp->voltage_targ_0, VOSEL_ROUND_UP);
        if (p_volt_tmp->vosel_targ_0 <= FT_LDOLV_TARGVOSEL_MAX)
        {
            p_ft_info->status |= PMU_MPK_LLDO_VOSEL;
        }
    }

    /* Check sLDO fields */
    p_volt_tmp = &p_ft_info->sldo;
    if (p_volt_tmp->data.flag == FT_SECTOR_VALID)
    {
        /* Calculate sLDO vosel target value */
        p_volt_tmp->voltage_targ_0 = FT_SLDO_TARGVOLT;
        p_volt_tmp->vosel_targ_0 = Ft_PmuVoselCalc( &p_volt_tmp->data, p_volt_tmp->voltage_targ_0, VOSEL_ROUND_TO_THE_NEAREST_VAL);
        if (p_volt_tmp->vosel_targ_0 <= FT_SLDO_TARGVOSEL_MAX)
        {
            if ( p_volt_tmp->vosel_targ_0 < FT_SLDO_TARGVOSEL_USE_MIN)
            {
                p_volt_tmp->vosel_targ_0 = FT_SLDO_TARGVOSEL_USE_MIN;
            }
            p_volt_tmp->vosel_targ_1 = p_volt_tmp->vosel_targ_0;
            p_volt_tmp->vosel_targ_2 = p_volt_tmp->vosel_targ_0;
            p_ft_info->status |= PMU_MPK_SLDO_VOSEL;
        }
    }

    ret_status = STATUS_SUCCESS;

    //DONE:
    return ret_status;
}

uint32_t Ft_FtInfoGet(ft_sector_t *p_ft_info, uint8_t dev_addr, uint16_t page_size, uint16_t max_size, uint8_t write_cycle,
                      uint32_t scl_pin, uint32_t sda_pin, uint32_t i2c_speed)
{
    eeprom_cfg_t cfg;
    uint32_t ret_status;

    /* EEPROM initialization */
    ret_status = eeprom_init( &cfg, dev_addr, page_size, max_size, write_cycle, scl_pin, sda_pin, i2c_speed);
    if (ret_status != STATUS_SUCCESS)
    {
        goto DONE;
    }

    /* Clear FT info buffer */
    memset(p_ft_info, 0, sizeof(ft_sector_t));

    /* Read FT info from EEPROM */
    eeprom_read( &cfg, FT_SECTOR_IC_VERSION_OFS, (uint8_t *)&p_ft_info->ic_ver, sizeof(ft_sector_ic_ver_t));
    eeprom_read( &cfg, FT_SECTOR_FT_VERSION_OFS, (uint8_t *)&p_ft_info->ft_ver, sizeof(ft_sector_ft_ver_t));
    eeprom_read( &cfg, FT_SECTOR_DCDC_OFS, (uint8_t *)&p_ft_info->dcdc.data, sizeof(ft_sector_volt_t));
    eeprom_read( &cfg, FT_SECTOR_LDOMV_OFS, (uint8_t *)&p_ft_info->ldomv.data, sizeof(ft_sector_volt_t));
    eeprom_read( &cfg, FT_SECTOR_LDOLV_OFS, (uint8_t *)&p_ft_info->ldolv.data, sizeof(ft_sector_volt_t));
    eeprom_read( &cfg, FT_SECTOR_SLDO_OFS, (uint8_t *)&p_ft_info->sldo.data, sizeof(ft_sector_volt_t));
    eeprom_read( &cfg, FT_SECTOR_ADC_OFS, (uint8_t *)&p_ft_info->adc, sizeof(ft_sector_adc_t));

    /* Calculate target values */
    ret_status = Ft_PmuSettingCalc( p_ft_info );
    if (ret_status != STATUS_SUCCESS)
    {
        goto DONE;
    }

    g_p_ft_info = p_ft_info;
    ret_status = STATUS_SUCCESS;

DONE:
    return ret_status;
}

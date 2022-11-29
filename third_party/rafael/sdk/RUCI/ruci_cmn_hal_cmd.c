/******************************************************************************
*
* @File         ruci_cmn_hal_cmd.c
* @Version
* $Revision:5065
* $Date: 2022-09-08
* @Brief
* @Note
* Copyright (C) 2019 Rafael Microelectronics Inc. All rights reserved.
*
*****************************************************************************/

/******************************************************************************
* INCLUDES
******************************************************************************/
#include "ruci_cmn_hal_cmd.h"

#if (RUCI_ENDIAN_INVERSE)
#if (RUCI_ENABLE_CMN)

/******************************************************************************
* GLOBAL PARAMETERS
******************************************************************************/
// RUCI: set_agc ---------------------------------------------------------------
const uint8_t ruci_elmt_type_set_agc[] =
{
    1, 1, 1, 1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_agc[] =
{
    1, 1, 1, 1, 1, 1, 1
};

// RUCI: set_calibration_enable ------------------------------------------------
const uint8_t ruci_elmt_type_set_calibration_enable[] =
{
    1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_calibration_enable[] =
{
    1, 1, 1, 1
};

// RUCI: set_calibration_setting -----------------------------------------------
const uint8_t ruci_elmt_type_set_calibration_setting[] =
{
    1, 1, 1, 1, 1, 1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_calibration_setting[] =
{
    1, 1, 1, 1, 1, 1, 2, 3, 4
};

// RUCI: set_tx_power ----------------------------------------------------------
const uint8_t ruci_elmt_type_set_tx_power[] =
{
    1, 1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_tx_power[] =
{
    1, 1, 1, 1, 1
};

// RUCI: get_temperature_rpt ---------------------------------------------------
const uint8_t ruci_elmt_type_get_temperature_rpt[] =
{
    1, 1, 1
};
const uint8_t ruci_elmt_num_get_temperature_rpt[] =
{
    1, 1, 1
};

// RUCI: get_voltage_rpt -------------------------------------------------------
const uint8_t ruci_elmt_type_get_voltage_rpt[] =
{
    1, 1, 1
};
const uint8_t ruci_elmt_num_get_voltage_rpt[] =
{
    1, 1, 1
};

// RUCI: set_rssi_offset -------------------------------------------------------
const uint8_t ruci_elmt_type_set_rssi_offset[] =
{
    1, 1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_rssi_offset[] =
{
    1, 1, 1, 1, 1
};

// RUCI: set_tx_power_compensation ---------------------------------------------
const uint8_t ruci_elmt_type_set_tx_power_compensation[] =
{
    1, 1, 1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_tx_power_compensation[] =
{
    1, 1, 1, 1, 1, 1
};

// RUCI: set_tx_power_channel_compensation -------------------------------------
const uint8_t ruci_elmt_type_set_tx_power_channel_compensation[] =
{
    1, 1, 1, 1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_tx_power_channel_compensation[] =
{
    1, 1, 1, 1, 1, 1, 1
};

// RUCI: set_tx_power_channel_segment ------------------------------------------
const uint8_t ruci_elmt_type_set_tx_power_channel_segment[] =
{
    1, 1, 1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_tx_power_channel_segment[] =
{
    1, 1, 1, 1, 1, 1
};

// RUCI: set_pmu_mpk_setting ---------------------------------------------------
const uint8_t ruci_elmt_type_set_pmu_mpk_setting[] =
{
    1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_pmu_mpk_setting[] =
{
    1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1
};

#endif /* RUCI_ENABLE_CMN */
#endif /* RUCI_ENDIAN_INVERSE */

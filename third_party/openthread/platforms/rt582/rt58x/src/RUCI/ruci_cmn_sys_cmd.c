/******************************************************************************
*
* @File         ruci_cmn_sys_cmd.c
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
#include "ruci_cmn_sys_cmd.h"

#if (RUCI_ENDIAN_INVERSE)
#if (RUCI_ENABLE_CMN)

/******************************************************************************
* GLOBAL PARAMETERS
******************************************************************************/
// RUCI: get_fw_ver ------------------------------------------------------------
const uint8_t ruci_elmt_type_get_fw_ver[] =
{
    1, 1, 1
};
const uint8_t ruci_elmt_num_get_fw_ver[] =
{
    1, 1, 1
};

#endif /* RUCI_ENABLE_CMN */
#endif /* RUCI_ENDIAN_INVERSE */

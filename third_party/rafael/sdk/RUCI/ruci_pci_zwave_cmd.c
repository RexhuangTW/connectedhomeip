/******************************************************************************
*
* @File         ruci_pci_zwave_cmd.c
* @Version
* $Revision:6351
* $Date: 2023-11-17
* @Brief
* @Note
* Copyright (C) 2023 Rafael Microelectronics Inc. All rights reserved.
*
*****************************************************************************/

/******************************************************************************
* INCLUDES
******************************************************************************/
#include "ruci_pci_zwave_cmd.h"

#if (RUCI_ENDIAN_INVERSE)
#if (RUCI_ENABLE_PCI)

/******************************************************************************
* GLOBAL PARAMETERS
******************************************************************************/
// RUCI: initiate_zwave --------------------------------------------------------
const uint8_t ruci_elmt_type_initiate_zwave[] =
{
    1, 1, 1, 1
};
const uint8_t ruci_elmt_num_initiate_zwave[] =
{
    1, 1, 1, 1
};

// RUCI: set_zwave_modem -------------------------------------------------------
const uint8_t ruci_elmt_type_set_zwave_modem[] =
{
    1, 1, 1, 1
};
const uint8_t ruci_elmt_num_set_zwave_modem[] =
{
    1, 1, 1, 1
};

#endif /* RUCI_ENABLE_PCI */
#endif /* RUCI_ENDIAN_INVERSE */

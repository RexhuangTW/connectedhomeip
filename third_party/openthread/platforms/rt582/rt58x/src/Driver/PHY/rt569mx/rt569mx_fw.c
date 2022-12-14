/**************************************************************************//**
* @file     rt569mx_fw.c
* @version
* @brief    RF MCU FW initialization

******************************************************************************/

#include "cm3_mcu.h"
#include "rf_mcu.h"
#include "rf_mcu_chip.h"
#include "rf_common_init.h"



/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)

#if ((RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_B) && (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFM0) && (RF_MCU_CHIP_BASE == BASE_RAM_TYPE))

#if (RF_FW_INCLUDE_PCI == TRUE)
#include "prg_pci_m0b_fpgam0_fw.h"
const uint32_t firmware_size_ruci = sizeof(firmware_program_ruci);
#else
const uint8_t firmware_program_ruci[] = {0};
const uint32_t firmware_size_ruci = 0;
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
#include "prg_ble_m0b_fpgam0_fw.h"
const uint32_t firmware_size_ble = sizeof(firmware_program_ble);
#else
const uint8_t firmware_program_ble[] = {0};
const uint32_t firmware_size_ruci = 0;
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
#include "prg_multi_m0b_fpgam0_fw.h"
const uint32_t firmware_size_multi = sizeof(firmware_program_multi);
#else
const uint8_t firmware_program_multi[] = {0};
const uint32_t firmware_size_multi = 0;
#endif

const uint8_t firmware_program_it[] = {0};
const uint32_t firmware_size_it = 0;
const uint8_t firmware_const_it[] = {0};
const uint32_t const_size_it = 0;

#elif ((RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_B) && (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFT3) && (RF_MCU_CHIP_BASE == BASE_RAM_TYPE))

#if (RF_FW_INCLUDE_PCI == TRUE)
#include "prg_pci_m0b_fpgat3_fw.h"
const uint32_t firmware_size_ruci = sizeof(firmware_program_ruci);
#else
const uint8_t firmware_program_ruci[] = {0};
const uint32_t firmware_size_ruci = 0;
#endif

#if (RF_FW_INCLUDE_BLE == TRUE)
#include "prg_ble_m0b_fpgat3_fw.h"
const uint32_t firmware_size_ble = sizeof(firmware_program_ble);
#else
const uint8_t firmware_program_ble[] = {0};
const uint32_t firmware_size_ruci = 0;
#endif

#if (RF_FW_INCLUDE_MULTI_2P4G == TRUE)
#include "prg_multi_m0b_fpgat3_fw.h"
const uint32_t firmware_size_multi = sizeof(firmware_program_multi);
#else
const uint8_t firmware_program_multi[] = {0};
const uint32_t firmware_size_multi = 0;
#endif

const uint8_t firmware_program_it[] = {0};
const uint32_t firmware_size_it = 0;
const uint8_t firmware_const_it[] = {0};
const uint32_t const_size_it = 0;

#elif (RF_MCU_CHIP_BASE == BASE_ROM_TYPE)

#if (RF_FW_INCLUDE_INTERNAL_TEST == TRUE)
#if (RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_A)
#include "prg_m0a_it_fw.h"
const uint32_t firmware_size_it = sizeof(firmware_program_it);
const uint32_t const_size_it = sizeof(firmware_const_it);
#else
const uint8_t firmware_program_it[] = {0};
const uint32_t firmware_size_it = 0;
const uint8_t firmware_const_it[] = {0};
const uint32_t const_size_it = 0;
#endif
#endif

#else
#error "Error: Invalid RF chip version!!"
#endif

#endif /*(RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0) */


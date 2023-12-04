/**************************************************************************//**
* @file     rt569mxa_init.c
* @version
* @brief    host layer phy related configure

******************************************************************************/

#include "cm3_mcu.h"
#include "rf_mcu.h"
#include "rf_mcu_chip.h"

#if ((RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0) && (RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_B) && (RF_MCU_FW_TARGET == RF_MCU_FW_TARGET_BLE))
#include "rt569mxb_ble_init.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define ALIGNED_BY(x, y)                    (((x) + (y) - 1) / (y) * (y))

#define PWR_MODE_DCDC                       (0)
#define PWR_MODE_LDO                        (1)
#define PWR_MODE_CONCURRENT                 (2)
#define RF_PMU_OP_MODE                      (PWR_MODE_DCDC)
#define RF_PMU_PARA_VER                     (24)
#define HAL_ZERO_IF_EN                      (1)

#define EXT_HAL_MEM_ADDR_OFFSET             (0x8000)
#define EXT_HAL_MEM_START_ADDR              (0x1000)
#define EXT_HAL_MEM_SIZE                    (0x1000)

#define EXT_HAL_MEM_ADDR_MEM_IND            ALIGNED_BY(EXT_HAL_MEM_START_ADDR, 4)
#define EXT_HAL_MEM_ADDR_FW_CFG             (EXT_HAL_MEM_ADDR_MEM_IND + ALIGNED_BY(sizeof(sHAL_PARA_IND), 4))
#define EXT_HAL_MEM_ADDR_HW_CFG             (EXT_HAL_MEM_ADDR_FW_CFG + ALIGNED_BY(sizeof(sHAL_FW_CFG), 4))
#define EXT_HAL_MEM_ADDR_DFT_TAB_MDM        (EXT_HAL_MEM_ADDR_HW_CFG + ALIGNED_BY(sizeof(sHAL_HW_CFG)*HAL_MAX, 4))
#define EXT_HAL_MEM_ADDR_DFT_TAB_RF         (EXT_HAL_MEM_ADDR_DFT_TAB_MDM + ALIGNED_BY(MDM_CFG_SIZE, 4))
#define EXT_HAL_MEM_ADDR_DFT_TAB_PMU        (EXT_HAL_MEM_ADDR_DFT_TAB_RF + ALIGNED_BY(RF_CFG_SIZE, 4))
#define EXT_HAL_MEM_ADDR_BMU_MEM_HWRXQ      (EXT_HAL_MEM_ADDR_DFT_TAB_PMU + ALIGNED_BY(PMU_CFG_SIZE, 4))
#define EXT_HAL_MEM_ADDR_BMU_MEM_HWSWRXQ    (EXT_HAL_MEM_ADDR_BMU_MEM_HWRXQ + ALIGNED_BY(MEM_MAX, 4))
#define EXT_HAL_MEM_ADDR_HW_PARA            (EXT_HAL_MEM_ADDR_BMU_MEM_HWSWRXQ + ALIGNED_BY(MEM_MAX, 4))
#define EXT_HAL_MEM_ADDR_HW_TAB             (EXT_HAL_MEM_ADDR_HW_PARA + ALIGNED_BY(sizeof(sHAL_HW_PARA_IND), 4))
#define EXT_HAL_MEM_ADDR_PRTCL_CFG          (EXT_HAL_MEM_ADDR_HW_TAB + ALIGNED_BY(sizeof(sHAL_HW_TAB_IND), 4))
#define EXT_HAL_MEM_ADDR_REG_PATCH          (EXT_HAL_MEM_ADDR_PRTCL_CFG + ALIGNED_BY(sizeof(sHAL_PRTCL_CFG), 4))

#define EXT_HAL_MEM_EN_HW_CFG_BLE                       (EXT_HAL_MEM_ADDR_HW_CFG)
#if (HAL_CFG & HAL_BLE_SUPPORT)
#define EXT_HAL_MEM_EN_HW_CFG_WISUN                     (EXT_HAL_MEM_EN_HW_CFG_BLE + sizeof(sHAL_HW_CFG))
#else
#define EXT_HAL_MEM_EN_HW_CFG_WISUN                     (EXT_HAL_MEM_EN_HW_CFG_BLE)
#endif
#if (HAL_CFG & HAL_WISUN_SUPPORT)
#define EXT_HAL_MEM_EN_HW_CFG_ZIGBEE                    (EXT_HAL_MEM_EN_HW_CFG_WISUN + sizeof(sHAL_HW_CFG))
#else
#define EXT_HAL_MEM_EN_HW_CFG_ZIGBEE                    (EXT_HAL_MEM_EN_HW_CFG_WISUN)
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
#define EXT_HAL_MEM_EN_HW_CFG_SLINK                     (EXT_HAL_MEM_EN_HW_CFG_ZIGBEE + sizeof(sHAL_HW_CFG))
#else
#define EXT_HAL_MEM_EN_HW_CFG_SLINK                     (EXT_HAL_MEM_EN_HW_CFG_ZIGBEE)
#endif

#define EXT_HAL_MEM_EN_NONE                 (0x0000UL)
#define EXT_HAL_MEM_EN_FW_CFG               (0x0001UL)
#define EXT_HAL_MEM_EN_HW_CFG               (0x0002UL)
#define EXT_HAL_MEM_EN_DFT_TAB_MDM          (0x0004UL)
#define EXT_HAL_MEM_EN_DFT_TAB_RF           (0x0008UL)
#define EXT_HAL_MEM_EN_DFT_TAB_PMU          (0x0010UL)
#define EXT_HAL_MEM_EN_BMU_MEM              (0x0020UL)
#define EXT_HAL_MEM_EN_HW_PARA              (0x0040UL)
#define EXT_HAL_MEM_EN_HW_TAB               (0x0080UL)
#define EXT_HAL_MEM_EN_PRTCL_CFG            (0x0100UL)
#define EXT_HAL_MEM_EN_REG_PATCH            (0x0200UL)
#define EXT_HAL_MEM_EN                      (EXT_HAL_MEM_EN_NONE)

#define HAL_MANUAL_NONE                     (0x0000)
#define HAL_MANUAL_MDM_DFT                  (0x0001)
#define HAL_MANUAL_RF_DFT                   (0x0002)
#define HAL_MANUAL_PMU_DFT                  (0x0004)
#define HAL_MANUAL_TX_BW                    (0x0008)
#define HAL_MANUAL_RX_BW                    (0x0010)
#define HAL_MANUAL_BMU_INIT                 (0x0020)
#define HAL_MANUAL_BMU_MEM                  (0x0040)
#define HAL_MANUAL_AGC_INIT                 (0x0080)
#define HAL_MANUAL_AGC                      (0x0100)
#define HAL_MANUAL_TCON_INIT                (0x0200)
#define HAL_MANUAL_TCON_TX                  (0x0400)
#define HAL_MANUAL_DIO                      (0x0800)
#define HAL_MANUAL_TEMP_MEAS                (0x1000)
#define HAL_MANUAL_VDD_MEAS                 (0x2000)
#define HAL_MANUAL_TX_EN                    (0x4000)
#define HAL_MANUAL_RX_EN                    (0x8000)
#define HAL_MANUAL_CFG                      (HAL_MANUAL_TEMP_MEAS|HAL_MANUAL_VDD_MEAS)

#define HAL_REG_PATCH_EN_NONE               (0x0000)
#define HAL_REG_PATCH_EN_HAL_INIT_BLE       (0x0001)
#define HAL_REG_PATCH_EN_HAL_INIT_WISUN     (0x0002)
#define HAL_REG_PATCH_EN_HAL_INIT_ZIGBEE    (0x0004)
#define HAL_REG_PATCH_EN_HAL_INIT_SLINK     (0x0008)
#define HAL_REG_PATCH_EN_TX_BW              (0x0010)
#define HAL_REG_PATCH_EN_RX_BW              (0x0020)
#define HAL_REG_PATCH_EN_BMU_INIT           (0x0040)
#define HAL_REG_PATCH_EN_BMU_MEM            (0x0080)
#define HAL_REG_PATCH_EN_AGC_INIT           (0x0100)
#define HAL_REG_PATCH_EN_AGC                (0x0200)
#define HAL_REG_PATCH_EN_TCON_INIT          (0x0400)
#define HAL_REG_PATCH_EN_TCON_TX            (0x0800)
#define HAL_REG_PATCH_EN_TCON_TX2M          (0x1000)
#define HAL_REG_PATCH_EN_REG_RESTORE        (0x2000)
#define HAL_REG_PATCH_EN_TX_EN              (0x4000)
#define HAL_REG_PATCH_EN_RX_EN              (0x8000)
#define HAL_REG_PATCH_EN                    (HAL_REG_PATCH_EN_NONE)


#define BYTE_INV_U32(a)                     ((((uint32_t)(a) & 0x000000FF) << 24) | (((uint32_t)(a) & 0x0000FF00) << 8) | (((uint32_t)(a) & 0x00FF0000) >> 8) | (((uint32_t)(a) & 0xFF000000) >> 24))
#define BYTE_INV_U16(a)                     ((((uint16_t)(a) & 0x000000FF) << 8) | (((uint16_t)(a) & 0x0000FF00) >> 8))
#define BYTE_INV_U8(a)                      (a)




#define HALRF_PLL_REF_CLK_32M                               (0)
#define HALRF_PLL_REF_CLK_16M                               (1)

#define PHY_TAB_BYPASS_FIR_15TAP_LEN                        (8)
#define PHY_TAB_BYPASS_FIR_31TAP_LEN                        (16)
#define PHY_TAB_RX_FIR_S2_LEN                               (16)
#define PHY_TAB_RX_FIR_S1_LEN                               (8)
#define PHY_TAB_RX_FIR_S0_LEN                               (8)
#define PHY_TAB_RX_RESAMP_FIR_LEN                           (8)
#define PHY_TAB_TX_FIR_LEN                                  (16)
#define PHY_TAB_TX_UFIR0_SLINK_LEN                          (16)
#define PHY_TAB_TX_UFIR1_SLINK_LEN                          (16)
#define PHY_TAB_TX_RESAMP_FIR_LEN                           (8)
#define PHY_TAB_SLINK_SF_LEN                                (8)
#define PHY_TAB_LNA_GAIN_LEN                                (16)
#define PHY_TAB_TIA_GAIN_LEN                                (16)
#define PHY_TAB_VGA_GAIN_LEN                                (16)
#define RF_TAB_FILT_CAP_INIT                                (13)
#define RF_TAB_TX_PWR_COMP_SEG_NUM                          (4)
#define RF_TAB_TX_PWR_COMP_SEG_0                            (13)
#define RF_TAB_TX_PWR_COMP_SEG_1                            (26)
#define RF_TAB_TX_PWR_COMP_SEG_2                            (38)

#define PHY_TAB_VAL_WISUN_PKT_SYNC_BIT_TH_SFD2              (28)
#define PHY_TAB_VAL_WISUN_PKT_SYNC_BIT_TH_SFD4              (38)
#define PHY_TAB_VAL_BLE_PKT_SYNC_BIT_TH_1M                  (29)
#define PHY_TAB_VAL_BLE_PKT_SYNC_BIT_TH_2M                  (29)
#define PHY_TAB_VAL_BLE_PKT_SYNC_BIT_TH_CODED_PHY           (32)
#define PHY_TAB_VAL_ZIGBEE_PKT_SYNC_BIT_TH                  (11)

#define PHY_TAB_VAL_WISUN_PKT_SYNC_CORR_TH_SFD2             (700)
#define PHY_TAB_VAL_WISUN_PKT_SYNC_CORR_TH_SFD4             (1400)
#define PHY_TAB_VAL_BLE_PKT_SYNC_CORR_TH_1M                 (1250)
#define PHY_TAB_VAL_BLE_PKT_SYNC_CORR_TH_2M                 (1200)
#define PHY_TAB_VAL_BLE_PKT_SYNC_CORR_TH_CODED_PHY          (800)
#define PHY_TAB_VAL_ZIGBEE_PKT_SYNC_CORR_TH                 (300)

#define PHY_TAB_VAL_WISUN_TRACK_BIT_TH_SFD2                 (20)
#define PHY_TAB_VAL_WISUN_TRACK_BIT_TH_SFD4                 (36)
#define PHY_TAB_VAL_BLE_TRACK_BIT_TH_1M                     (32)
#define PHY_TAB_VAL_BLE_TRACK_BIT_TH_2M                     (40)
#define PHY_TAB_VAL_BLE_TRACK_BIT_TH_CODED_PHY              (32)
#define PHY_TAB_VAL_ZIGBEE_TRACK_BIT_TH                     (31)

#define PHY_TAB_VAL_WISUN_TRACK_CORR_TH_SFD2                (600)
#define PHY_TAB_VAL_WISUN_TRACK_CORR_TH_SFD4                (800)
#define PHY_TAB_VAL_BLE_TRACK_CORR_TH_1M                    (800)
#define PHY_TAB_VAL_BLE_TRACK_CORR_TH_2M                    (1200)
#define PHY_TAB_VAL_BLE_TRACK_CORR_TH_CODED_PHY             (800)
#define PHY_TAB_VAL_ZIGBEE_TRACK_CORR_TH                    (1500)

#define PHY_TAB_VAL_WISUN_RX_NUM_PREAM_BASE                 (1)
#define PHY_TAB_VAL_BLE_RX_NUM_PREAM_BASE_1M                (0)
#define PHY_TAB_VAL_BLE_RX_NUM_PREAM_BASE_2M                (1)
#define PHY_TAB_VAL_BLE_RX_NUM_PREAM_BASE_CODED_PHY         (0)

#define PHY_TAB_VAL_PRE_CFO_EN_HIGH_BW                      (0)
#define PHY_TAB_VAL_PRE_CFO_EN_LOW_BW                       (1)

#define PHY_TAB_VAL_CFO_EST_ALPHA_IDX_CODED_PHY             (2)
#define PHY_TAB_VAL_CFO_EST_ALPHA_IDX_ZIGBEE                (2)

#define PHY_TAB_VAL_BLE_SYMB_SYNC_ALPHA_0                   (1)
#define PHY_TAB_VAL_BLE_SYMB_SYNC_ALPHA_1                   (2)
#define PHY_TAB_VAL_ZIGBEE_SYMB_SYNC_ALPHA_0                (2)
#define PHY_TAB_VAL_ZIGBEE_SYMB_SYNC_ALPHA_1                (2)
#define PHY_TAB_VAL_ZIGBEE_SUBG_SYMB_SYNC_ALPHA_1           (1)

#define PHY_TAB_VAL_WISUN_DETECT_OFFSET                     (7)
#define PHY_TAB_VAL_BLE_DETECT_OFFSET_1M                    (7)
#define PHY_TAB_VAL_BLE_DETECT_OFFSET_2M                    (7)
#define PHY_TAB_VAL_BLE_DETECT_OFFSET_CODED_PHY             (7)
#define PHY_TAB_VAL_ZIGBEE_DETECT_OFFSET                    (7)

#define PHY_TAB_VAL_CODED_PHY_TX_PREAMBLE_NUM               (10)
#define PHY_TAB_VAL_CODED_PHY_BIT_SCORE_TH                  (35)
#define PHY_TAB_VAL_CODED_PHY_SYNC_BIT_TH                   (28)
#define PHY_TAB_VAL_CODED_PHY_SYNC_CORR_TH                  (945)
#define PHY_TAB_VAL_CODED_PHY_TRACK_KP                      (5)
#define PHY_TAB_VAL_CODED_PHY_TRACK_KQ                      (4)

#define PHY_TAB_VAL_ZIGBEE_L_SFD_CHECK                      (15)
#define PHY_TAB_VAL_ZIGBEE_DESPREAD_SOFT_TH                 (0x100)
#define PHY_TAB_VAL_ZIGBEE_SUBG_DESPREAD_SOFT_TH            (0x0FA)
#define PHY_TAB_VAL_ZIGBEE_CHIP_VALUE_BOUND                 (0x16)
#define PHY_TAB_VAL_ZIGBEE_SUBG_CHIP_VALUE_BOUND            (0x1F)
#define PHY_TAB_VAL_ZIGBEE_BT_INDEX                         (1)
#define PHY_TAB_VAL_ZIGBEE_EXTRA_PREAMBLE_HB                (0)
#define PHY_TAB_VAL_ZIGBEE_EXTRA_PREAMBLE_LB                (8)
#define PHY_TAB_VAL_ZIGBEE_TRACK_KP                         (3)
#define PHY_TAB_VAL_ZIGBEE_TRACK_KQ                         (1)

#define HALPHY_RFK_PARA_RX_TIADC_TX_IF_BASE                 (0x200)
#define HALPHY_RFK_PARA_RX_TIADC_TX_IF_OFFSET               (0x0)
#define HALPHY_RFK_PARA_RX_TIADC_RX_IF                      (0x200)
#define HALPHY_RFK_PARA_RX_TIADC_RX_BW_SEL                  (0x01)
#define HALPHY_RFK_PARA_RX_TIADC_TX_BW_SEL                  (15)
#define HALPHY_RFK_PARA_RX_TIADC_IF_SEL                     (0)
#define HALPHY_RFK_PARA_RX_TIADC_RX_ST_VAL_I                (127)
#define HALPHY_RFK_PARA_RX_TIADC_RX_FILT_PW                 (0)
#define HALPHY_RFK_PARA_RX_TIADC_LNA_GAIN_B                 (15)
#define HALPHY_RFK_PARA_RX_TIADC_TIA_GAIN                   (15)
#define HALPHY_RFK_PARA_RX_TIADC_VGA_GAIN                   (15)
#define HALPHY_RFK_PARA_RX_TIADC_TX_BUF_GAIN_0_MAN          (0x09)
#define HALPHY_RFK_PARA_RX_TIADC_TX_BUF_GAIN_1_MAN          (0x0F)
#define HALPHY_RFK_PARA_RX_TIADC_TX_BUF_GAIN_2_MAN          (0x0F)
#define HALPHY_RFK_PARA_RX_TIADC_TX_BUF_GAIN_3_MAN          (0x0F)
#define HALPHY_RFK_PARA_RX_TIADC_TX_BUF_PW                  (7)
#define HALPHY_RFK_PARA_RX_TIADC_RX_FILT_CCOMP              (3)
#define HALPHY_RFK_PARA_RX_TIADC_PW_VCO                     (15)

#define HALPHY_RFK_PARA_RX_FILTER_TX_IF_BASE                (0x200)
#define HALPHY_RFK_PARA_RX_FILTER_TX_IF_OFFSET              (0x19A)
#define HALPHY_RFK_PARA_RX_FILTER_RX_IF                     (0x200)
#define HALPHY_RFK_PARA_RX_FILTER_RX_BW_SEL                 (1)
#define HALPHY_RFK_PARA_RX_FILTER_TX_BW_SEL                 (15)
#define HALPHY_RFK_PARA_RX_FILTER_IF_SEL                    (0)
#define HALPHY_RFK_PARA_RX_FILTER_RX_ST_VAL_I               (127)
#define HALPHY_RFK_PARA_RX_FILTER_RX_FILT_PW                (0)
#define HALPHY_RFK_PARA_RX_FILTER_LNA_GAIN_B                (10)
#define HALPHY_RFK_PARA_RX_FILTER_TIA_GAIN                  (3)
#define HALPHY_RFK_PARA_RX_FILTER_VGA_GAIN                  (3)
#if (RF_TX_POWER_TYPE == RF_TX_POWER_0DBM)
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_0_MAN         (0x00)
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_1_MAN         (0x0F)
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_2_MAN         (0x0F)
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_3_MAN         (0x0F)
#else
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_0_MAN         (0x00)
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_1_MAN         (0x00)
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_2_MAN         (0x00)
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_3_MAN         (0x00)
#endif
#define HALPHY_RFK_PARA_RX_FILTER_TX_BUF_PW                 (7)
#define HALPHY_RFK_PARA_RX_FILTER_RX_FILT_CCOMP             (3)
#define HALPHY_RFK_PARA_RX_FILTER_PW_VCO                    (15)

#define HALPHY_RFK_PARA_TX_SB_TX_IF_BASE                    (0x800)
#define HALPHY_RFK_PARA_TX_SB_TX_IF_OFFSET                  (0x0)
#define HALPHY_RFK_PARA_TX_SB_RX_IF                         (0x200)
#define HALPHY_RFK_PARA_TX_SB_RX_BW_SEL                     (0)
#define HALPHY_RFK_PARA_TX_SB_TX_BW_SEL                     (6)
#define HALPHY_RFK_PARA_TX_SB_IF_SEL                        (0)
#define HALPHY_RFK_PARA_TX_SB_RX_ST_VAL_I                   (127)
#define HALPHY_RFK_PARA_TX_SB_RX_FILT_PW                    (6)
#define HALPHY_RFK_PARA_TX_SB_LNA_GAIN_B                    (2)
#define HALPHY_RFK_PARA_TX_SB_TIA_GAIN                      (10)
#define HALPHY_RFK_PARA_TX_SB_VGA_GAIN                      (10)
#if (RF_TX_POWER_TYPE == RF_TX_POWER_0DBM)
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_0_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_1_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_2_MAN             (0x0F)
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_3_MAN             (0x0F)
#else
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_0_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_1_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_2_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_3_MAN             (0x0A)
#endif
#define HALPHY_RFK_PARA_TX_SB_TX_BUF_PW                     (7)
#define HALPHY_RFK_PARA_TX_SB_RX_FILT_CCOMP                 (0)
#define HALPHY_RFK_PARA_TX_SB_PW_VCO                        (15)

#define HALPHY_RFK_PARA_TX_LO_TX_IF_BASE                    (0x800)
#define HALPHY_RFK_PARA_TX_LO_TX_IF_OFFSET                  (0x0)
#define HALPHY_RFK_PARA_TX_LO_RX_IF                         (0x200)
#define HALPHY_RFK_PARA_TX_LO_RX_BW_SEL                     (0)
#define HALPHY_RFK_PARA_TX_LO_TX_BW_SEL                     (8)
#define HALPHY_RFK_PARA_TX_LO_IF_SEL                        (0)
#define HALPHY_RFK_PARA_TX_LO_RX_ST_VAL_I                   (127)
#define HALPHY_RFK_PARA_TX_LO_RX_FILT_PW                    (6)
#define HALPHY_RFK_PARA_TX_LO_LNA_GAIN_B                    (0)
#define HALPHY_RFK_PARA_TX_LO_TIA_GAIN                      (10)
#define HALPHY_RFK_PARA_TX_LO_VGA_GAIN                      (10)
#if (RF_TX_POWER_TYPE == RF_TX_POWER_0DBM)
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_0_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_1_MAN             (0x0F)
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_2_MAN             (0x0F)
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_3_MAN             (0x0F)
#else
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_0_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_1_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_2_MAN             (0x00)
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_3_MAN             (0x00)
#endif
#define HALPHY_RFK_PARA_TX_LO_TX_BUF_PW                     (7)
#define HALPHY_RFK_PARA_TX_LO_RX_FILT_CCOMP                 (0)
#define HALPHY_RFK_PARA_TX_LO_PW_VCO                        (15)

#define BMU_TAB_VAL_TXQ_DEPTH                               (0)         // 0: Single, 0x7f: Dual
#define BMU_TAB_VAL_RXQ_DEPTH                               (3)         // 0: Single, 1: Dual, ...
#define BMU_TAB_VAL_CTE_ROUND_MODE                          (2)
#define BMU_TAB_VAL_CTE_DOWN_SAMPLE_MASK                    (5)

#define BMU_TAB_VAL_RXQ_TH_HW_RXQ                           (6)
#define BMU_TAB_VAL_TXQ_TH_HW_RXQ                           (16)
#define BMU_TAB_VAL_EMPTY_TH_HW_RXQ                         (11)
#define BMU_TAB_VAL_RXQ_TH_HWSW_RXQ                         (1)
#define BMU_TAB_VAL_TXQ_TH_HWSW_RXQ                         (1)
#define BMU_TAB_VAL_EMPTY_TH_HWSW_RXQ                       (1)

#define HALTCON_PLL_LOCK_TIME                               (80)
#define HALTCON_TX_ON_TIME                                  (0)
#define HALTCON_TX_OFF_TIME                                 (0)
#define HALTCON_PA_ON_TIME                                  (1)
#define HALTCON_PA_OFF_TIME                                 (2)
#define HALTCON_RAMP_ON_TIME                                (10)
#define HALTCON_RAMP_OFF_TIME                               (5)
#define HALTCON_RAMP_ON_OFFSET                              (HALTCON_PA_ON_TIME)
#define HALTCON_RX_ON_TIME                                  (10)

#define RF_TAB_VAL_LNA_GAIN_2G                              (15)
#define RF_TAB_VAL_LNA_GAIN_SUBG                            (7)
#define RF_TAB_VAL_TIA_GAIN_2G                              (15)
#define RF_TAB_VAL_TIA_GAIN_SUBG                            (7)
#define RF_TAB_VAL_VGA_GAIN_2G                              (13)
#define RF_TAB_VAL_VGA_GAIN_SUBG                            (10)
#define RF_TAB_RSSI_OFFSET_DEFAULT                          (26)
#define RF_TAB_RSSI_OFFSET_DEFAULT_SUBG_915M                (0)
#define RF_TAB_RSSI_OFFSET_DEFAULT_SUBG_868M                (0)
#define RF_TAB_RSSI_OFFSET_DEFAULT_SUBG_433M                (0)
#define RF_TAB_RSSI_OFFSET_DEFAULT_SUBG_315M                (0)

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_FW_CFG)
const sHAL_FW_CFG extHalMemFwCfg =
{
    /* DIO control */
    BYTE_INV_U8(3),                                     // dioType
    BYTE_INV_U8(28),                                    // dioDbgType

    /* System Sleep Control */
    BYTE_INV_U8(0),                                     // sysSlpLevel 0:No_Sleep, 1:Wait-I, 2:Sleep
    BYTE_INV_U16(3000),                                 // sysSlpGurdTime

    /* Watchdog timer of RF mechanism */
    BYTE_INV_U32(1000000),                              // watchdogTimer, unit: us

    /* External FEM setting */
    BYTE_INV_U8(1),                                     // extFemEnable;
    BYTE_INV_U8(1),                                     // extFemTxPin;
    BYTE_INV_U8(2),                                     // extFemRxPin;
    BYTE_INV_U8(0),                                     // extFemTxActType;
    BYTE_INV_U8(0),                                     // extFemRxActType;
};
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_HW_CFG)
const sHAL_HW_CFG extHalMemHwCfg[] =
{
#if (HAL_CFG & HAL_BLE_SUPPORT)
    {
        /* SYSTEM */
        BYTE_INV_U16(0x0000),                           // HAL type, BLE, Zigbee, Wisun, or Slink
        //BYTE_INV_U16(0x0001),                           // RF band, Sub-1G or 2.4G
        BYTE_INV_U16(0x0000),                           // RFK band, 2.4G, SubG-915M, SubG-868M, SubG-433M, SubG-315M
        //BYTE_INV_U16(0x0001),                           // System clock, 16MHz or 32MHz
        BYTE_INV_U16(0x00FF),                           // RX maximum packet length in byte

        /* Preamble */
        BYTE_INV_U16(0x0000),                           // RX preamble base, 0 or 1
        BYTE_INV_U16(0x0000),                           // TX preamble base, 0 or 1

        /* SFD */
        BYTE_INV_U16(0x0000),                           // RX SFD type, 2 bytes or 4 bytes
        BYTE_INV_U16(0x0000),                           // TX SFD type, 2 bytes or 4 bytes
        {
            BYTE_INV_U32(0x71764129),
            BYTE_INV_U32(0x00000000),
        },

        /* PHR */
        BYTE_INV_U16(0x0001),                           // PHR type, enable or disable

        /* MHR */
        BYTE_INV_U8(0x01),                              // MHR report length in byte

        /* CRC */
        BYTE_INV_U16(0x0001),                           // CRC type, CRC-16, CRC-24, CRC-32 or disable CRC
        {
            BYTE_INV_U32(0x00000000),                   // CRC-16 default CRC initial value
            BYTE_INV_U32(0x00555555),                   // CRC-24 default CRC initial value
            BYTE_INV_U32(0x00000000),                   // CRC-32 default CRC initial value
        },

        /* Whitening */
        BYTE_INV_U16(0x0001),                           // Whitening type, enable or disable
        BYTE_INV_U8(0x25),                              // Default Whitening initial value

        /* BIT endian */
        BYTE_INV_U16(0x0000),                           // Payload and CRC bit endian for Wisun MAC

        /* Modem */
        BYTE_INV_U16(0x0000),                           // Modulation index, 0.5 or 1
        BYTE_INV_U16(0x0000),                           // Gaussian filter type

        /* BMU */
        BYTE_INV_U16(0x0000),                           // RXQ post-process method, automatic or manual
        BYTE_INV_U8(0x00),                              // RX control field length
        BYTE_INV_U16(0x0000),                           // TXQ post-process method, automatic or manual
        BYTE_INV_U8(0x00),                              // TX control field length
    },
#endif
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    {
        /* SYSTEM */
        BYTE_INV_U16(0x0001),                           // HAL type, BLE, Zigbee, Wisun, or Slink
        //BYTE_INV_U16(0x0000),                           // RF band, Sub-1G or 2.4G
        BYTE_INV_U16(0x0001),                           // RFK band, 2.4G, SubG-915M, SubG-868M, SubG-433M, SubG-315M
        //BYTE_INV_U16(0x0001),                           // System clock, 16MHz or 32MHz
        BYTE_INV_U16(0x07FF),                           // RX maximum packet length in byte

        /* Preamble */
        BYTE_INV_U16(0x0001),                           // RX preamble base, 0 or 1
        BYTE_INV_U16(0x0001),                           // TX preamble base, 0 or 1

        /* SFD */
        BYTE_INV_U16(0x0001),                           // RX SFD type, 2 bytes or 4 bytes
        BYTE_INV_U16(0x0001),                           // TX SFD type, 2 bytes or 4 bytes
        {
            BYTE_INV_U32(0x71764129),                   // SFD-4-byte default SFD content
            BYTE_INV_U32(0x00007209),                   // SFD-2-byte default SFD content
        },

        /* PHR */
        BYTE_INV_U16(0x0001),                           // PHR type, enable or disable

        /* MHR */
        BYTE_INV_U8(0x01),                              // MHR report length in byte

        /* CRC */
        BYTE_INV_U16(0x0000),                           // CRC type, CRC-16, CRC-24, CRC-32 or disable CRC
        {
            BYTE_INV_U32(0x00000000),                   // CRC-16 default CRC initial value
            BYTE_INV_U32(0x00000000),                   // CRC-24 default CRC initial value
            BYTE_INV_U32(0xFFFFFFFF),                   // CRC-32 default CRC initial value
        },

        /* Whitening */
        BYTE_INV_U16(0x0000),                           // Whitening type, enable or disable
        BYTE_INV_U8(0xFF),                              // Default Whitening initial value

        /* BIT endian */
        BYTE_INV_U16(0x0005),                           // Payload and CRC bit endian for Wisun MAC

        /* Modem */
        BYTE_INV_U16(0x0000),                           // Modulation index, 0.5 or 1
        BYTE_INV_U16(0x0001),                           // Gaussian filter type

        /* BMU */
        BYTE_INV_U16(0x0000),                           // RXQ post-process method, automatic or manual
        BYTE_INV_U8(0x00),                              // RX control field length
        BYTE_INV_U16(0x0000),                           // TXQ post-process method, automatic or manual
        BYTE_INV_U8(0x00),                              // TX control field length
    },
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    {
        /* SYSTEM */
        BYTE_INV_U16(0x0002),                           // HAL type, BLE, Zigbee, Wisun, or Slink
        //BYTE_INV_U16(0x0001),                           // RF band, Sub-1G or 2.4G
        BYTE_INV_U16(0x0000),                           // RFK band, 2.4G, SubG-915M, SubG-868M, SubG-433M, SubG-315M
        //BYTE_INV_U16(0x0001),                           // System clock, 16MHz or 32MHz
        BYTE_INV_U16(0x007F),                           // RX maximum packet length in byte

        /* Preamble */
        BYTE_INV_U16(0x0000),                           // RX preamble base, 0 or 1
        BYTE_INV_U16(0x0000),                           // TX preamble base, 0 or 1

        /* SFD */
        BYTE_INV_U16(0x0000),                           // RX SFD type, 2 bytes or 4 bytes
        BYTE_INV_U16(0x0000),                           // TX SFD type, 2 bytes or 4 bytes
        {
            BYTE_INV_U32(0x00000000),                   // SFD-4-byte default SFD content
            BYTE_INV_U32(0x00000000),                   // SFD-2-byte default SFD content
        },

        /* PHR */
        BYTE_INV_U16(0x0001),                           // PHR type, enable or disable

        /* MHR */
        BYTE_INV_U8(0x15),                              // MHR report length in byte

        /* CRC */
        BYTE_INV_U16(0x0000),                           // CRC type, CRC-16, CRC-24, CRC-32 or disable CRC
        {
            BYTE_INV_U32(0x00000000),                   // CRC-16 default CRC initial value
            BYTE_INV_U32(0x00000000),                   // CRC-24 default CRC initial value
            BYTE_INV_U32(0xFFFFFFFF),                   // CRC-32 default CRC initial value
        },

        /* Whitening */
        BYTE_INV_U16(0x0001),                           // Whitening type, enable or disable
        BYTE_INV_U8(0xFF),                              // Default Whitening initial value

        /* BIT endian */
        BYTE_INV_U16(0x0000),                           // Payload and CRC bit endian for Wisun MAC

        /* Modem */
        BYTE_INV_U16(0x0000),                           // Modulation index, 0.5 or 1
        BYTE_INV_U16(0x0001),                           // Gaussian filter type

        /* BMU */
        BYTE_INV_U16(0x0000),                           // RXQ post-process method, automatic or manual
        BYTE_INV_U8(0x00),                              // RX control field length
        BYTE_INV_U16(0x0000),                           // TXQ post-process method, automatic or manual
        BYTE_INV_U8(0x00),                              // TX control field length
    },
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    {
        /* SYSTEM */
        BYTE_INV_U16(0x0003),                           // HAL type, BLE, Zigbee, Wisun, or Slink
        //BYTE_INV_U16(0x0000),                           // RF band, Sub-1G or 2.4G
        BYTE_INV_U16(0x0001),                           // RFK band, 2.4G, SubG-915M, SubG-868M, SubG-433M, SubG-315M
        //BYTE_INV_U16(0x0001),                           // System clock, 16MHz or 32MHz
        BYTE_INV_U16(0x00FF),                           // RX maximum packet length in byte

        /* Preamble */
        BYTE_INV_U16(0x0000),                           // RX preamble base, 0 or 1
        BYTE_INV_U16(0x0000),                           // TX preamble base, 0 or 1

        /* SFD */
        BYTE_INV_U16(0x0000),                           // RX SFD type, 2 bytes or 4 bytes
        BYTE_INV_U16(0x0000),                           // TX SFD type, 2 bytes or 4 bytes
        {
            BYTE_INV_U32(0x00000000),                   // SFD-4-byte default SFD content
            BYTE_INV_U32(0x00000000),                   // SFD-2-byte default SFD content
        },

        /* PHR */
        BYTE_INV_U16(0x0001),                           // PHR type, enable or disable

        /* MHR */
        BYTE_INV_U8(0x01),                              // MHR report length in byte

        /* CRC */
        BYTE_INV_U16(0x0000),                           // CRC type, CRC-16, CRC-24, CRC-32 or disable CRC
        {
            BYTE_INV_U32(0x00000000),                   // CRC-16 default CRC initial value
            BYTE_INV_U32(0x00000000),                   // CRC-24 default CRC initial value
            BYTE_INV_U32(0x00000000),                   // CRC-32 default CRC initial value
        },

        /* Whitening */
        BYTE_INV_U16(0x0000),                           // Whitening type, enable or disable
        BYTE_INV_U8(0x00),                              // Default Whitening initial value

        /* BIT endian */
        BYTE_INV_U16(0x0000),                           // Payload and CRC bit endian for Wisun MAC

        /* Modem */
        BYTE_INV_U16(0x0000),                           // Modulation index, 0.5 or 1
        BYTE_INV_U16(0x0001),                           // Gaussian filter type

        /* BMU */
        BYTE_INV_U16(0x0000),                           // RXQ post-process method, automatic or manual
        BYTE_INV_U8(0x00),                              // RX control field length
        BYTE_INV_U16(0x0000),                           // TXQ post-process method, automatic or manual
        BYTE_INV_U8(0x00),                              // TX control field length
    },
#endif
};
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_DFT_TAB_MDM)
const uint8_t extHalMemDftTabMdm[MDM_CFG_SIZE] =
{
    /*  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F */
    0x00, 0x04, 0x00, 0x00, 0x40, 0x01, 0x14, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x00
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x7f, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x81, 0x00, // 0x10
    0x00, 0x04, 0x80, 0x00, 0x02, 0x20, 0xb0, 0x04, 0x29, 0x41, 0x76, 0x71, 0x02, 0x20, 0x54, 0x33, // 0x20
    0xb0, 0x04, 0x20, 0x00, 0x21, 0x43, 0x05, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x30
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, // 0x40
    0x40, 0x00, 0x00, 0x00, 0x15, 0x00, 0x58, 0x02, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x04, 0x00, // 0x50
    0x20, 0x03, 0x08, 0x23, 0x00, 0x01, 0x00, 0x00, 0x0f, 0x01, 0x05, 0x00, 0x05, 0x01, 0x02, 0x00, // 0x60
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x70
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x80
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x90
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xA0
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xB0
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xC0
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, // 0xD0
};
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_DFT_TAB_RF)
const uint8_t extHalMemDftTabRf[RF_CFG_SIZE] =
{
#if (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFT3)
    /*  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F */
    0x00, 0x80, 0xD8, 0x10, 0xA4, 0x0D, 0x01, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x0D, 0xA0, 0x10, 0x00, // 0x00
    0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0xEE, 0xE0, 0x23, 0x0F, 0x2D, 0x0F, 0xFF, 0x0C, 0xDF, 0xDD, // 0x10
    0x07, 0x15, 0x83, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x2A, 0x7C, 0xFE, // 0x20
    0xBF, 0x37, 0xFE, 0x7A, 0x6F, 0xFD, 0x70, 0x40, 0xE8, 0x0B, 0xAB, 0x7A, 0x02, 0x80, 0xC2, 0xB3, // 0x30
    0xBC, 0x88, 0x88, 0xD4, 0x30, 0x53, 0xB1, 0x6B, 0x2C, 0xCC, 0x4A, 0x00, 0xA3, 0x61, 0x80, 0xC0, // 0x40
    0xC3, 0x6C, 0x00, 0x20, 0x0F, 0x00, 0x00, 0x00, 0xC4, 0x08, 0x04, 0x00, 0x14, 0x0F, 0x0A, 0x05, // 0x50
    0x02, 0x10, 0x00, 0x00, 0x0F, 0x1A, 0x07, 0xFF, 0x14, 0x0F, 0x0A, 0x05, 0x02, 0x10, 0x00, 0x00, // 0x60
    0x2F, 0x20, 0x07, 0x0D, 0x3F, 0x00, 0xF0, 0xF0, 0x09, 0xF7, 0x00, 0x0A, 0x47, 0x44, 0x04, 0x08, // 0x70
    0xB3, 0x40, 0x50, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x80

#else
#if (RF_PMU_PARA_VER >= 24)     /* 20230221 */
#if (RF_TX_POWER_TYPE == RF_TX_POWER_0DBM)
    /*  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F */
    0x00, 0x00, 0x62, 0x90, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x72, 0x01, 0xbd, 0x10, 0x00, // 0x00
    0x00, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x50, 0x00, 0x3d, 0x08, 0x00, 0x0f, 0x0f, 0x0f, 0x03, // 0x10
    0x07, 0x15, 0x81, 0xf7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xca, 0x19, 0x76, // 0x20
    0x3a, 0xff, 0xfe, 0x00, 0xe7, 0x7d, 0x00, 0x00, 0xdf, 0x70, 0x07, 0xa3, 0x07, 0x44, 0x80, 0x5a, // 0x30
    0x28, 0xc0, 0x0b, 0x06, 0x8e, 0xc0, 0xf3, 0x20, 0x2c, 0x80, 0x72, 0x3c, 0x00, 0x90, 0x00, 0x00, // 0x40
    0xc3, 0xdc, 0x03, 0x20, 0x13, 0x20, 0x00, 0x80, 0x80, 0x0e, 0x00, 0x08, 0x14, 0x14, 0x14, 0x09, // 0x50
    0x22, 0x12, 0x00, 0x00, 0x08, 0x0a, 0x03, 0xff, 0x19, 0x0d, 0x04, 0x08, 0x13, 0x11, 0x00, 0x00, // 0x60
    0x10, 0x0e, 0x01, 0x0d, 0x70, 0x00, 0xf2, 0xf2, 0x0b, 0xf2, 0x00, 0x0a, 0x00, 0x00, 0x01, 0x00, // 0x70
    0x95, 0x04, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x80
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x31, // 0x90
    0x00, 0x41, 0xa0, 0x10, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x00, 0xf1, 0xff, 0x22, 0x22, // 0xA0
    0xee, 0xff, 0xff, 0x00, 0xf1, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x32, 0x14, 0x1f, // 0xB0
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x17, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xC0
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x17, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, // 0xD0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xE0
#else
    /*  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F */
    0x00, 0x00, 0x62, 0x90, 0xd1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x72, 0x00, 0xb0, 0x10, 0x00, // 0x00
    0x00, 0x00, 0x00, 0x00, 0x8f, 0x00, 0x00, 0x50, 0x00, 0x3d, 0x08, 0x00, 0x0f, 0x0f, 0x0f, 0x03, // 0x10
    0x07, 0x15, 0x81, 0xf7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0xca, 0x19, 0x76, // 0x20
    0x3a, 0xff, 0xfe, 0x00, 0xe7, 0x7d, 0x00, 0x00, 0x8e, 0xf1, 0x07, 0xa3, 0x07, 0x44, 0x80, 0x5a, // 0x30
    0x28, 0xc0, 0x0b, 0x02, 0x8e, 0xc0, 0xf3, 0x20, 0x2c, 0x80, 0x12, 0x3c, 0x02, 0x74, 0x80, 0x00, // 0x40
    0xc3, 0xdc, 0x03, 0x20, 0x13, 0x20, 0x00, 0x80, 0x80, 0x0e, 0x00, 0x08, 0x14, 0x14, 0x14, 0x09, // 0x50
    0x22, 0x12, 0x00, 0x00, 0x08, 0x0a, 0x03, 0xff, 0x19, 0x0d, 0x04, 0x08, 0x23, 0x11, 0x00, 0x00, // 0x60
    0x10, 0x0e, 0x01, 0x0d, 0x70, 0x00, 0xf2, 0xf2, 0x0b, 0xf2, 0x00, 0x0a, 0x00, 0x00, 0x01, 0x00, // 0x70
    0x95, 0x07, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x80
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x31, // 0x90
    0x00, 0x41, 0xa0, 0x10, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x00, 0xf1, 0xff, 0xee, 0x77, // 0xA0
    0xd9, 0xee, 0xfe, 0x00, 0xf0, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x32, 0x14, 0x1f, // 0xB0
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x17, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xC0
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x17, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, // 0xD0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xE0
#endif

#else
#error "Error: Not supported RF parameters!!!"
#endif
#endif
};
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_DFT_TAB_PMU)
const uint8_t extHalMemDftTabPmu[PMU_CFG_SIZE] =
{
#if (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFT3)
    /*  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F */
    // 0x00
    0x00, 0x7D, 0x0B, 0x84, 0x00, 0x00, 0x03, 0x00, // 0x10
    0x01, 0x00, 0x55, 0xF5, 0x3F, 0xF4, 0x00, 0x02, 0x29, 0x08, 0x24, 0xC0, 0xF9, 0x03, 0xFF, 0x03, // 0x20
    0x10, 0x32, 0x04, 0x00, 0xFF, 0x07, 0xDB, 0x00, 0xFF, 0x3F, 0x0C, 0x84, 0xE7, 0x20, 0x00, 0x00, // 0x30
    0xFF, 0x0B, 0xC2, 0xE0, 0xFF, 0x03, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x05, 0x20, // 0x40
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x50
    0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x60

#else
#if (RF_PMU_PARA_VER >= 24)     /* 20230221 */
#if (RF_TX_POWER_TYPE == RF_TX_POWER_0DBM)
    /*  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F */
    // 0x00
    0x00, 0x7D, 0x0F, 0xB4, 0x00, 0x00, 0x00, 0x00, // 0x10
    0x80, 0x00, 0x00, 0xF0, 0x2F, 0xF4, 0x19, 0x06, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x20
    0x10, 0x32, 0x04, 0x00, 0x00, 0x04, 0x14, 0x23, 0x00, 0x01, 0x00, 0x84, 0xF3, 0x38, 0x00, 0x00, // 0x30
    0x3F, 0xC8, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x06, 0x05, 0x20, // 0x40
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x50
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, // 0x60
    0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF9, 0x00, // 0x70
    0x77, 0x77, 0xF7, 0x00, 0x77, 0x77, 0x00, 0x00, 0x06, 0x60, 0x66, 0x00, 0x66, 0x66, 0x66, 0x66, // 0x80
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBB, 0x8B, 0x04, 0x04, 0x04, 0x04, 0x3F, 0x0F, // 0x90
    0x3A, 0xDD, 0x4D, 0x70, 0x3D, 0xBD, 0x69, 0x70, 0x00, 0xFF, 0x00, 0xFF, 0x7E, 0x3A, 0xD9, 0xC0, // 0xA0
    0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xB0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xC0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xD0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xE0
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x28, 0x00, 0x00, 0x10, 0x6A, 0x00, 0x00, // 0xF0
#else
    /*  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F */
    // 0x00
    0x00, 0x7D, 0x0F, 0xB4, 0x00, 0x00, 0x00, 0x00, // 0x10
    0x80, 0x00, 0x00, 0xF0, 0x2F, 0xF4, 0x19, 0x06, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x20
    0x10, 0x32, 0x04, 0x00, 0x00, 0x04, 0x14, 0x23, 0x00, 0x01, 0x00, 0x84, 0xF3, 0x38, 0x00, 0x00, // 0x30
    0x3F, 0xC8, 0xFF, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x06, 0x05, 0x20, // 0x40
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x64, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, // 0x50
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, // 0x60
    0x64, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF9, 0x00, // 0x70
    0x77, 0x77, 0xF7, 0x00, 0x77, 0x77, 0x00, 0x00, 0x06, 0x60, 0x66, 0x00, 0x66, 0x66, 0x66, 0x66, // 0x80
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBB, 0x8B, 0x10, 0x10, 0x10, 0x10, 0x3F, 0x0F, // 0x90
    0x3A, 0xDD, 0x4D, 0x70, 0x3D, 0xBD, 0x69, 0x70, 0x00, 0xFF, 0x00, 0xFF, 0x7E, 0x3A, 0xD9, 0xC0, // 0xA0
    0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xB0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xC0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xD0
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 0xE0
    0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x28, 0x00, 0x00, 0x10, 0x6A, 0x00, 0x00, // 0xF0
#endif

#else
#error "Error: Not supported PMU parameters!!!"
#endif
#endif
};
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_BMU_MEM)
const uint8_t extHalMemBmuMemHwRxq[MEM_MAX] =
{
    /*  Memory map for HW RX queue */
    /*  CTE,       Privacy,    H2M,        M2H,        LRX,        LTX,        LDQ,        FQ      */
    /*  0x4040,    0x4C00,     0x4D00,     0x4E40      0x4F80,     0x50C0,        ,        0x5200  */
    0x30,      0x04,       0x05,       0x05,       0x05,       0x05,       0x02,       0x33
};

const uint8_t extHalMemBmuMemHwSwRxq[MEM_MAX] =
{
    /* Memory map for HW RX queue with limited size and SW RX queue with full size */
    /*  CTE,       Privacy,    H2M,        M2H,        LRX,        LTX,        LDQ,        FQ      */
    /*  0x4040,    0x4C00,     0x4D00,     0x4E40      0x4F80,     0x50C0,        ,        0x5200  */
    0x00,      0x00,       0x02,       0x02,       0x5A,       0x01,       0x02,       0x1C
};
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_HW_PARA)
const sHAL_HW_PARA_IND  extHalMemHwPara =
{
    /* Sync. bit threshold */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_WISUN_PKT_SYNC_BIT_TH_SFD2),            // syncBitThWisunSfd2
    BYTE_INV_U8(PHY_TAB_VAL_WISUN_PKT_SYNC_BIT_TH_SFD4),            // syncBitThWisunSfd4
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_PKT_SYNC_BIT_TH_1M),                // syncBitThBle1M
    BYTE_INV_U8(PHY_TAB_VAL_BLE_PKT_SYNC_BIT_TH_2M),                // syncBitThBle2M
#if (HAL_CODED_PHY_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_PKT_SYNC_BIT_TH_CODED_PHY),         // syncBitThBleCodedPhy
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_PKT_SYNC_BIT_TH),                // syncBitThZigbee
#endif

    /* Sync. correlation threshold */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_WISUN_PKT_SYNC_CORR_TH_SFD2),          // syncCorrThWisunSfd2
    BYTE_INV_U16(PHY_TAB_VAL_WISUN_PKT_SYNC_CORR_TH_SFD4),          // syncCorrThWisunSfd4
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_BLE_PKT_SYNC_CORR_TH_1M),              // syncCorrThBle1M
    BYTE_INV_U16(PHY_TAB_VAL_BLE_PKT_SYNC_CORR_TH_2M),              // syncCorrThBle2M
#if (HAL_CODED_PHY_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_BLE_PKT_SYNC_CORR_TH_CODED_PHY),       // syncCorrThBleCodedPhy
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_ZIGBEE_PKT_SYNC_CORR_TH),              // syncCorrThZigbee
#endif

    /* Tracking bit threshold */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_WISUN_TRACK_BIT_TH_SFD2),               // trackBitThWisunSfd2
    BYTE_INV_U8(PHY_TAB_VAL_WISUN_TRACK_BIT_TH_SFD4),               // trackBitThWisunSfd4
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_TRACK_BIT_TH_1M),                   // trackBitThBle1M
    BYTE_INV_U8(PHY_TAB_VAL_BLE_TRACK_BIT_TH_2M),                   // trackBitThBle2M
#if (HAL_CODED_PHY_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_TRACK_BIT_TH_CODED_PHY),            // trackBitThBleCodedPhy
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_TRACK_BIT_TH),                   // trackBitThZigbee
#endif

    /* Tracking correlation threshold */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_WISUN_TRACK_CORR_TH_SFD2),             // trackCorrThWisunSfd2
    BYTE_INV_U16(PHY_TAB_VAL_WISUN_TRACK_CORR_TH_SFD4),             // trackCorrThWisunSfd4
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_BLE_TRACK_CORR_TH_1M),                 // trackCorrThBle1M
    BYTE_INV_U16(PHY_TAB_VAL_BLE_TRACK_CORR_TH_2M),                 // trackCorrThBle2M
#if (HAL_CODED_PHY_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_BLE_TRACK_CORR_TH_CODED_PHY),          // trackCorrThBleCodedPhy
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_ZIGBEE_TRACK_CORR_TH),                 // trackCorrThZigbee
#endif

    /* RX number of preamble base */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_WISUN_RX_NUM_PREAM_BASE),               // rxNumPreamBaseWisun
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_RX_NUM_PREAM_BASE_1M),              // rxNumPreamBaseBle1M
    BYTE_INV_U8(PHY_TAB_VAL_BLE_RX_NUM_PREAM_BASE_2M),              // rxNumPreamBaseBle2M
#if (HAL_CODED_PHY_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_RX_NUM_PREAM_BASE_CODED_PHY),       // rxNumPreamBaseBleCodedPhy
#endif
#endif

    /* Pre-CFO */
    BYTE_INV_U8(PHY_TAB_VAL_PRE_CFO_EN_HIGH_BW),                    // preCfoEnHighBw
    BYTE_INV_U8(PHY_TAB_VAL_PRE_CFO_EN_LOW_BW),                     // preCfoEnLowBw

    /* CFO tracking */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT))
    BYTE_INV_U8(PHY_TAB_VAL_CFO_EST_ALPHA_IDX_CODED_PHY),           // cfoEstAlphaIdxCodedPhy
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_CFO_EST_ALPHA_IDX_ZIGBEE),              // cfoEstAlphaIdxZigbee
#endif

    /* OQPSK symbol sync alpha index */
#if (HAL_CFG & HAL_BLE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_SYMB_SYNC_ALPHA_0),                 // symbSyncAlpha0Ble
    BYTE_INV_U8(PHY_TAB_VAL_BLE_SYMB_SYNC_ALPHA_1),                 // symbSyncAlpha1Ble
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_SYMB_SYNC_ALPHA_0),              // symbSyncAlpha0Zigbee
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_SYMB_SYNC_ALPHA_1),              // symbSyncAlpha1Zigbee
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_SUBG_SYMB_SYNC_ALPHA_1),         // symbSyncAlpha1ZigbeeSubg;
#endif
#endif

    /* Detection offset */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_WISUN_DETECT_OFFSET),                   // detectOffsetWisun
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_DETECT_OFFSET_1M),                  // detectOffsetBle1M
    BYTE_INV_U8(PHY_TAB_VAL_BLE_DETECT_OFFSET_2M),                  // detectOffsetBle2M
#if (HAL_CODED_PHY_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_BLE_DETECT_OFFSET_CODED_PHY),           // detectOffsetBleCodedPhy
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_DETECT_OFFSET),                  // detectOffsetZigbee
#endif

    /* Coded PHY related */
#if ((HAL_CFG & HAL_BLE_SUPPORT) && (HAL_CODED_PHY_SUPPORT))
    BYTE_INV_U8(PHY_TAB_VAL_CODED_PHY_TX_PREAMBLE_NUM),             // txPreamNumCodedPhy
    BYTE_INV_U8(PHY_TAB_VAL_CODED_PHY_BIT_SCORE_TH),                // bitScoreCodedPhy
    BYTE_INV_U16(PHY_TAB_VAL_CODED_PHY_SYNC_BIT_TH),                // syncBitThCodedPhy
    BYTE_INV_U16(PHY_TAB_VAL_CODED_PHY_SYNC_CORR_TH),               // syncCorrThCodedPhy
    BYTE_INV_U8(PHY_TAB_VAL_CODED_PHY_TRACK_KP),                    // trackKpCodedPhy;
    BYTE_INV_U8(PHY_TAB_VAL_CODED_PHY_TRACK_KQ),                    // trackKqCodedPhy;
#endif

    /* Zigbee related */
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_L_SFD_CHECK),                    // lSfdCheckZigbee
    BYTE_INV_U16(PHY_TAB_VAL_ZIGBEE_DESPREAD_SOFT_TH),              // despreadSoftThZigbee
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_ZIGBEE_SUBG_DESPREAD_SOFT_TH),         // despreadSoftThZigbeeSubg
#endif
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_CHIP_VALUE_BOUND),               // chipValueBoundZigbee
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_SUBG_CHIP_VALUE_BOUND),          // chipValueBoundZigbeeSubg
#endif
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_BT_INDEX),                       // btIdxZigbee
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_EXTRA_PREAMBLE_HB),              // extraPreamHbZigbee
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_EXTRA_PREAMBLE_LB),              // extraPreamLbZigbee
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_TRACK_KP),                       // trackKpZigbee;
    BYTE_INV_U8(PHY_TAB_VAL_ZIGBEE_TRACK_KQ),                       // trackKqZigbee;
#endif

    /* SLINK related */
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    BYTE_INV_U16(PHY_TAB_VAL_SLINK_TX_UP_SAMP_N0),                  // txUpSampN0Slink
    BYTE_INV_U16(PHY_TAB_VAL_SLINK_TX_UP_SAMP_N1),                  // txUpSampN1Slink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_TX_DOWN_SAMP_M0),                 // txDownSampM0Slink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_SYNC_CFO_TRACK_EN_G0),            // syncCfoTrackEnG0Slink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_SYNC_CFO_TRACK_EN_G1),            // syncCfoTrackEnG1Slink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_SYNC_CFO_TRACK_EN_G2),            // syncCfoTrackEnG2Slink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_ALPHA_EST_ERR_G0),                // alphaEstErrG0Slink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_ALPHA_EST_ERR_G1),                // alphaEstErrG1Slink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_ALPHA_EST_ERR_G2),                // alphaEstErrG2Slink
    BYTE_INV_U8(HALPHY_SLINK_UPSAMP_TYPE0),                         // upsampType0Slink
    BYTE_INV_U8(HALPHY_SLINK_UPSAMP_TYPE1),                         // upsampType1Slink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_CAD_EN),                          // cadEnSlink
    BYTE_INV_U8(PHY_TAB_VAL_SLINK_CAD_SNR_TH),
#endif

    /* AGC */
#if (HALRF_CFG & HALRF_2G_SUPPORT)
    BYTE_INV_U8(RF_TAB_VAL_LNA_GAIN_2G),                            // lnaGain2G
    BYTE_INV_U8(RF_TAB_VAL_TIA_GAIN_2G),                            // tiaGain2G
    BYTE_INV_U8(RF_TAB_VAL_VGA_GAIN_2G),                            // vgaGain2G
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    BYTE_INV_U8(RF_TAB_VAL_LNA_GAIN_SUBG),                          // lnaGainSubG
    BYTE_INV_U8(RF_TAB_VAL_TIA_GAIN_SUBG),                          // tiaGainSubG
    BYTE_INV_U8(RF_TAB_VAL_VGA_GAIN_SUBG),                          // vgaGainSubG
#endif
    {
        // rssiOffset[RFK_FREQ_MAX]
        BYTE_INV_U8(RF_TAB_RSSI_OFFSET_DEFAULT),
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
        BYTE_INV_U8(RF_TAB_RSSI_OFFSET_DEFAULT_SUBG_915M),
#if (0) // ( !HAL_SUBG_FREQ_BAND_MERGE_915M_868M )
        BYTE_INV_U8(RF_TAB_RSSI_OFFSET_DEFAULT_SUBG_868M),
#endif
        BYTE_INV_U8(RF_TAB_RSSI_OFFSET_DEFAULT_SUBG_433M),
        BYTE_INV_U8(RF_TAB_RSSI_OFFSET_DEFAULT_SUBG_315M),
#endif
    },

    /* BMU */
    BYTE_INV_U8(BMU_TAB_VAL_TXQ_DEPTH),                             // txqDepth
    BYTE_INV_U8(BMU_TAB_VAL_RXQ_DEPTH),                             // rxqDepth
    BYTE_INV_U8(BMU_TAB_VAL_CTE_ROUND_MODE),                        // cteRoundMode
    BYTE_INV_U8(BMU_TAB_VAL_CTE_DOWN_SAMPLE_MASK),                  // cteDownSampleMask
    BYTE_INV_U8(BMU_TAB_VAL_TXQ_TH_HW_RXQ),                         // txqThHwRxq
    BYTE_INV_U8(BMU_TAB_VAL_RXQ_TH_HW_RXQ),                         // rxqThHwRxq
    BYTE_INV_U8(BMU_TAB_VAL_EMPTY_TH_HW_RXQ),                       // emptyThHwRxq
    BYTE_INV_U8(BMU_TAB_VAL_TXQ_TH_HWSW_RXQ),                       // txqThHwSwRxq
    BYTE_INV_U8(BMU_TAB_VAL_RXQ_TH_HWSW_RXQ),                       // rxqThHwSwRxq
    BYTE_INV_U8(BMU_TAB_VAL_EMPTY_TH_HWSW_RXQ),                     // emptyThHwSwRxq

    /* PLL */
    BYTE_INV_U8(HALRF_PLL_REF_CLK_32M),                             // pllRefClock

    /* TCON */
    BYTE_INV_U16(HALTCON_PLL_LOCK_TIME),                            // pllLockTime
    BYTE_INV_U16(HALTCON_PA_ON_TIME),                               // paOnTime
    BYTE_INV_U16(HALTCON_RAMP_ON_TIME),                             // rampOnTime
    BYTE_INV_U16(HALTCON_TX_ON_TIME),                               // txOnTime
    BYTE_INV_U16(HALTCON_PA_OFF_TIME),                              // paOffTime
    BYTE_INV_U16(HALTCON_RAMP_OFF_TIME),                            // rampOffTime
    BYTE_INV_U16(HALTCON_TX_OFF_TIME),                              // txOffTime
    BYTE_INV_U16(HALTCON_RAMP_ON_OFFSET),                           // rampOnOffset
    BYTE_INV_U16(HALTCON_RX_ON_TIME),                               // rxOnTime

    /* TX power compensation */
    {
        // chSeg[3]
        BYTE_INV_U8(RF_TAB_TX_PWR_COMP_SEG_0),
        BYTE_INV_U8(RF_TAB_TX_PWR_COMP_SEG_1),
        BYTE_INV_U8(RF_TAB_TX_PWR_COMP_SEG_2),
    },

    /* 2.4GHz RF modualtion type */
    BYTE_INV_U8(HAL_ZERO_IF_EN),                                    // zeroIfEn

    /* PMU operation mode */
    BYTE_INV_U8(RF_PMU_OP_MODE),                                    // pmuOpMode

    /* RF Calibration */
    {
        {
            // rfkPara[RF_CAL_EXT_PARA_RX_TIADC]
            BYTE_INV_U16(HALPHY_RFK_PARA_RX_TIADC_TX_IF_BASE),          // txIf
            BYTE_INV_U16(HALPHY_RFK_PARA_RX_TIADC_TX_IF_OFFSET),        // txIfOffset
            BYTE_INV_U16(HALPHY_RFK_PARA_RX_TIADC_RX_IF),               // rxIf
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_RX_BW_SEL),            // rxBwSel
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_TX_BW_SEL),            // txBwSel
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_IF_SEL),               // ifSel
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_RX_ST_VAL_I),          // rxStValI
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_RX_FILT_PW),           // rxRxFiltPw
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_LNA_GAIN_B),           // lnaGainB
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_TIA_GAIN),             // tiaGain
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_VGA_GAIN),             // vgaGain
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_TX_BUF_GAIN_0_MAN),    // txBufGain0Man
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_TX_BUF_GAIN_1_MAN),    // txBufGain1Man
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_TX_BUF_GAIN_2_MAN),    // txBufGain2Man
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_TX_BUF_GAIN_3_MAN),    // txBufGain3Man
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_TX_BUF_PW),            // txBufPw
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_RX_FILT_CCOMP),        // rxFiltCcomp
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_TIADC_PW_VCO),               // pwVco
        },

        {
            // rfkPara[RF_CAL_EXT_PARA_RX_FILTER]
            BYTE_INV_U16(HALPHY_RFK_PARA_RX_FILTER_TX_IF_BASE),         // txIf
            BYTE_INV_U16(HALPHY_RFK_PARA_RX_FILTER_TX_IF_OFFSET),       // txIfOffset
            BYTE_INV_U16(HALPHY_RFK_PARA_RX_FILTER_RX_IF),              // rxIf
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_RX_BW_SEL),           // rxBwSel
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_TX_BW_SEL),           // txBwSel
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_IF_SEL),              // ifSel
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_RX_ST_VAL_I),         // rxStValI
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_RX_FILT_PW),          // rxRxFiltPw
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_LNA_GAIN_B),          // lnaGainB
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_TIA_GAIN),            // tiaGain
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_VGA_GAIN),            // vgaGain
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_0_MAN),   // txBufGain0Man
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_1_MAN),   // txBufGain1Man
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_2_MAN),   // txBufGain2Man
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_TX_BUF_GAIN_3_MAN),   // txBufGain3Man
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_TX_BUF_PW),           // txBufPw
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_RX_FILT_CCOMP),       // rxFiltCcomp
            BYTE_INV_U8(HALPHY_RFK_PARA_RX_FILTER_PW_VCO),              // pwVco
        },

        {
            // rfkPara[RF_CAL_EXT_PARA_TX_SB]
            BYTE_INV_U16(HALPHY_RFK_PARA_TX_SB_TX_IF_BASE),             // txIf
            BYTE_INV_U16(HALPHY_RFK_PARA_TX_SB_TX_IF_OFFSET),           // txIfOffset
            BYTE_INV_U16(HALPHY_RFK_PARA_TX_SB_RX_IF),                  // rxIf
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_RX_BW_SEL),               // rxBwSel
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_TX_BW_SEL),               // txBwSel
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_IF_SEL),                  // ifSel
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_RX_ST_VAL_I),             // rxStValI
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_RX_FILT_PW),              // rxRxFiltPw
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_LNA_GAIN_B),              // lnaGainB
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_TIA_GAIN),                // tiaGain
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_VGA_GAIN),                // vgaGain
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_0_MAN),       // txBufGain0Man
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_1_MAN),       // txBufGain1Man
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_2_MAN),       // txBufGain2Man
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_TX_BUF_GAIN_3_MAN),       // txBufGain3Man
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_TX_BUF_PW),               // txBufPw
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_RX_FILT_CCOMP),           // rxFiltCcomp
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_SB_PW_VCO),                  // pwVco
        },

        {
            // rfkPara[RF_CAL_EXT_PARA_TX_LO]
            BYTE_INV_U16(HALPHY_RFK_PARA_TX_LO_TX_IF_BASE),             // txIf
            BYTE_INV_U16(HALPHY_RFK_PARA_TX_LO_TX_IF_OFFSET),           // txIfOffset
            BYTE_INV_U16(HALPHY_RFK_PARA_TX_LO_RX_IF),                  // rxIf
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_RX_BW_SEL),               // rxBwSel
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_TX_BW_SEL),               // txBwSel
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_IF_SEL),                  // ifSel
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_RX_ST_VAL_I),             // rxStValI
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_RX_FILT_PW),              // rxRxFiltPw
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_LNA_GAIN_B),              // lnaGainB
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_TIA_GAIN),                // tiaGain
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_VGA_GAIN),                // vgaGain
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_0_MAN),       // txBufGain0Man
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_1_MAN),       // txBufGain1Man
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_2_MAN),       // txBufGain2Man
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_TX_BUF_GAIN_3_MAN),       // txBufGain3Man
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_TX_BUF_PW),               // txBufPw
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_RX_FILT_CCOMP),           // rxFiltCcomp
            BYTE_INV_U8(HALPHY_RFK_PARA_TX_LO_PW_VCO),                  // pwVco
        },
    },
};
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_HW_TAB)
const  sHAL_HW_TAB_IND extHalMemHwTab =
{
    /* RF TX\RX BW and IF */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
#if (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFT3)
    // rfTabTxBw
    {BYTE_INV_U8(0x0A),     BYTE_INV_U8(0x06)},
    // rfTabRxBw
    {BYTE_INV_U8(0x06),     BYTE_INV_U8(0x05)},
    // rfTabRxIf
    {BYTE_INV_U8(0x05),     BYTE_INV_U8(0x03)},
#else
    // rfTabTxBw
    {BYTE_INV_U8(0x0F),     BYTE_INV_U8(0x0D)},
    // rfTabRxBw
    {BYTE_INV_U8(0x02),     BYTE_INV_U8(0x01)},
    // rfTabRxIf
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x00)},
#endif
#if ((RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFM0) || (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0))
    // rfTabRxFiltOpComp
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x00)},
    // rfTabRxFiltPw
    {BYTE_INV_U8(0x06),     BYTE_INV_U8(0x06)},
    // rfTabRxFiltVcm
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x01)},
    // rfTabRxFiltCcomp
    {BYTE_INV_U8(0x04),     BYTE_INV_U8(0x03)},
    // rfTabVgaOpComp
    {BYTE_INV_U8(0x00),     BYTE_INV_U8(0x00)},
    // rfTabRxVgaPw
    {BYTE_INV_U8(0x04),     BYTE_INV_U8(0x02)},
#endif
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
#if (RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFT3)
    // rfTabTxBwDClk
    {BYTE_INV_U8(0x0C),     BYTE_INV_U8(0x0A),      BYTE_INV_U8(0x06)},
    // rfTabRxBwDClk
    {BYTE_INV_U8(0x07),     BYTE_INV_U8(0x06),      BYTE_INV_U8(0x05)},
    // rfTabRxIfDClk
    {BYTE_INV_U8(0x07),     BYTE_INV_U8(0x05),      BYTE_INV_U8(0x03)},
#else
    // rfTabTxBwDClk
    {BYTE_INV_U8(0x06),     BYTE_INV_U8(0x0F),      BYTE_INV_U8(0x0D)},
    // rfTabRxBwDClk
    {BYTE_INV_U8(0x03),     BYTE_INV_U8(0x02),      BYTE_INV_U8(0x01)},
    // rfTabRxIfDClk
    {BYTE_INV_U8(0x02),     BYTE_INV_U8(0x01),      BYTE_INV_U8(0x00)},
#endif
#if ((RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFM0) || (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0))
    // rfTabRxFiltOpCompDclk
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x01),      BYTE_INV_U8(0x00)},
    // rfTabRxFiltPwDclk
    {BYTE_INV_U8(0x06),     BYTE_INV_U8(0x06),      BYTE_INV_U8(0x06)},
    // rfTabRxFiltVcmDclk
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x01),      BYTE_INV_U8(0x01)},
    // rfTabRxFiltCcompDclk
    {BYTE_INV_U8(0x05),     BYTE_INV_U8(0x04),      BYTE_INV_U8(0x03)},
    // rfTabVgaOpCompDclk
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x00),      BYTE_INV_U8(0x00)},
    // rfTabRxVgaPwDclk
    {BYTE_INV_U8(0x06),     BYTE_INV_U8(0x04),      BYTE_INV_U8(0x02)},
#endif
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // rfTabSlinkTxBw
    // rfTabSlinkRxBw
    // rfTabSlinkRxIf
#endif

    /* One point modulation */
#if (0) // (HAL_ONE_POINT_EN)
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569MP)
    // rfTabOpPwCp
    // rfTabOpSelRz
    // rfTabOpSelCz
    // rfTabOpSelCp
    // rfTabSdmScale
    // phyTabModIdx
#endif
#endif

    /* TXP gain */
#if (0) // (HAL_TX_POWER_GAIN_TAB_SWITCH_SUPPORT)
    // rfTabTxpGain
#endif
    // rfTabTxpGainSubg
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    {
        // SubG-915M
        {0x22, 0x15, 0x22, 0x16, 0x1A, 0x15, 0x1A, 0x16, 0x7A, 0x14, 0x7A, 0x15, 0x72, 0x17, 0xFA, 0x17, 0xC2, 0x17, 0x40, 0x15},
#if (0) // ( !HAL_SUBG_FREQ_BAND_MERGE_915M_868M )
        // SubG-868M
        {0x22, 0x15, 0x22, 0x16, 0x1A, 0x15, 0x1A, 0x16, 0x7A, 0x14, 0x7A, 0x15, 0x72, 0x17, 0xFA, 0x17, 0xC2, 0x17, 0x40, 0x15},
#endif
        // SubG-433M
        {0x7A, 0x08, 0x6A, 0x08, 0x62, 0x09, 0x40, 0x0B, 0xC0, 0x0B, 0x7A, 0x0C, 0x7A, 0x14, 0x62, 0x14, 0xFA, 0x17, 0xC0, 0x17},
        // SubG-315M
        {0x3A, 0x08, 0x32, 0x08, 0x39, 0x08, 0x31, 0x09, 0x28, 0x09, 0x00, 0x09, 0x12, 0x0C, 0x1A, 0x15, 0x19, 0x0D, 0x41, 0x15},
    },
#endif

    /* SubG TXP */
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    // rfTabTxpSubg
#if (1) // (HAL_SUBG_FREQ_BAND_MERGE_915M_868M)
    {0x1E, 0x17, 0x1A},
#else
    {0x1E, 0x1E, 0x17, 0x1A},
#endif
#endif

    /* RX FIR S2 */
    // phyTabRxFirS2
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT))
    {0x02, 0x01, 0x00, 0xFE, 0xFD, 0xFC, 0xFC, 0xFD, 0xFF, 0x02, 0x06, 0x0A, 0x0E, 0x11, 0x13, 0x14},
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    // phyTabRxFirS2ModIdx1
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    // phyTabRxFirS2Zigbee
    {0x00, 0x01, 0x02, 0x02, 0x02, 0x00, 0xFE, 0xFC, 0xFB, 0xFC, 0x00, 0x06, 0x0C, 0x13, 0x17, 0x19},
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    // phyTabRxFirS2ZigbeeSubg
    {0x02, 0x01, 0x00, 0xFE, 0xFD, 0xFC, 0xFC, 0xFD, 0xFF, 0x02, 0x06, 0x0A, 0x0E, 0x11, 0x13, 0x14},
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabRxFirS2Slink
#endif

    /* RX FIR S1 */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    // phyTabRxFirS1[IDX_MAX][PHY_TAB_RX_FIR_S1_LEN]
    {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F},
        {0xFF, 0x02, 0x05, 0x08, 0x0B, 0x0E, 0x10, 0x11},
    },
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabRxFirS1Slink[SLINK_IDX_MAX][PHY_TAB_RX_FIR_S1_LEN]
#endif

    /* RX FIR S0 */
#if ((RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0) && (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT)))
    // phyTabRxFirS0[2][PHY_TAB_RX_FIR_S0_LEN];
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabRxFirS0Slink[SLINK_IDX_MAX][PHY_TAB_RX_FIR_S0_LEN];
#endif

    /* RX resample FIR */
#if(RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    // phyTabRxResampFir[PHY_TAB_RX_RESAMP_FIR_LEN]
#endif

    /* RX downsampling factor */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    // phyTabRxDownFact0[IDX_MAX]
#endif
    // phyTabRxDownFact1[IDX_MAX]
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x02)},
#if (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    // phyTabRxDownFact2[IDX_MAX]
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabRxDownFact0Slink[SLINK_IDX_MAX]
    // phyTabRxDownFact1Slink[SLINK_IDX_MAX]
    // phyTabRxDownFact2Slink[SLINK_IDX_MAX]
#endif

    /* TX FIR */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT))
    // phyTabTxFir[PHY_TAB_TX_FIR_LEN]
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F},
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    // phyTabTxFirZigbee[PHY_TAB_TX_FIR_LEN]
    {0x00, 0x01, 0x02, 0x02, 0x01, 0x00, 0xFE, 0xFC, 0xFB, 0xFD, 0x00, 0x06, 0x0C, 0x13, 0x17, 0x19},
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabTxFirSlink[PHY_TAB_TX_FIR_LEN]
#endif

    /* TX extra FIR for SLINK*/
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabTxUFir0Slink[PHY_TAB_TX_UFIR0_SLINK_LEN]
    // phyTabTxUFir1Slink[SLINK_IDX_MAX][PHY_TAB_TX_UFIR0_SLINK_LEN]
#endif

    /* TX resample FIR */
#if (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    // phyTabTxResampFir[2][PHY_TAB_TX_RESAMP_FIR_LEN]
#endif

    /* TX resampling factor */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    // phyTabTxUpsampN0[IDX_MAX]
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x01)},
    // phyTabTxDownsampM0[IDX_MAX]
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x01)},
    // phyTabTxUpsampN1[IDX_MAX]
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x02)},
#endif

    /*  */
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabTxUpFactorSlink[2][SLINK_IDX_MAX];
#endif

    /* PHY RX tracking parameter */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if ((HAL_CLK_CFG & HAL_16M_CLK_SUPPORT) && (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0))
    // phyTabRxPreSyncAlpha0[IDX_MAX]
    // phyTabRxPreSyncAlpha1[IDX_MAX]
#endif
#if ((HAL_CLK_CFG & HAL_32M_CLK_SUPPORT) && (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0))
    // phyTabRxPreSyncAlpha0Dclk[IDX_MAX_DCLK]
    // phyTabRxPreSyncAlpha1Dclk[IDX_MAX_DCLK]
#endif
    // phyTabRxTrackingKp[IDX_MAX]
    {BYTE_INV_U8(0x03),     BYTE_INV_U8(0x05)},
    // phyTabRxTrackingKq[IDX_MAX]
    {BYTE_INV_U8(0x01),     BYTE_INV_U8(0x04)},
    // phyTabRxCfoOffsetAlpha[IDX_MAX]
    {BYTE_INV_U8(0x02),     BYTE_INV_U8(0x01)},
#endif

    /* PHY IF */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    // phyTabIf[IDX_MAX]
    {BYTE_INV_U8(0x00),     BYTE_INV_U8(0x01)},
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    // phyTabIfDClk[IDX_MAX_DCLK];
    {BYTE_INV_U8(0x00),     BYTE_INV_U8(0x01),     BYTE_INV_U8(0x02)},
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabIfSlink[SLINK_IDX_MAX]
#endif

    /* TX preamble number */
#if (HAL_CFG & HAL_BLE_SUPPORT)
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    // phyTabTxPreamNumBLE[IDX_MAX]
    {BYTE_INV_U8(0x02),     BYTE_INV_U8(0x01)},
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    // phyTabTxPreamNumBLEDClk[IDX_MAX_DCLK]
    {BYTE_INV_U8(0x04),     BYTE_INV_U8(0x02),     BYTE_INV_U8(0x01)},
#endif
#endif
#if (HAL_CFG & HAL_WISUN_SUPPORT)
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    // phyTabTxPreamNumWisun[IDX_MAX]
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    // phyTabTxPreamNumWisunDClk[IDX_MAX_DCLK]
#endif
#endif
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569T3)
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    // phyTabTxDummyPreamble[IDX_MAX]
    // phyTabTxDummyPreambleTime[IDX_MAX]
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    // phyTabTxDummyPreambleDClk[IDX_MAX_DCLK]
    // phyTabTxDummyPreambleTimeDClk[IDX_MAX_DCLK]
#endif
#endif
#endif

    /* PHY data maping table */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    // phyTabBw[IDX_MAX]
    {BYTE_INV_U8(0x00),     BYTE_INV_U8(0x01)},
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    // phyTabBwDClk[IDX_MAX_DCLK]
    {BYTE_INV_U8(0x00),     BYTE_INV_U8(0x01),     BYTE_INV_U8(0x01)},
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabBwSlink[SLINK_IDX_MAX]
#endif

    /* Group delay (unit: us) */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    // phyTabTxDelay[IDX_MAX]
    {BYTE_INV_U8(0x03),     BYTE_INV_U8(0x04)},
    // phyTabTxTail[IDX_MAX]
    {BYTE_INV_U8(0x00),     BYTE_INV_U8(0x01)},
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT))
    // phyTabRxPathDelayFsk[IDX_MAX]
    {BYTE_INV_U8(0x07),     BYTE_INV_U8(0x09)},
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    // phyTabRxPathDelayCodedPhy[2]
    {BYTE_INV_U8(0x0E),     BYTE_INV_U8(0x0B)},
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    // phyTabRxPathDelayOqpsk[IDX_MAX]
    {BYTE_INV_U8(0x14),     BYTE_INV_U8(0x00),     BYTE_INV_U8(0x00)},
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabTxDelaySlink[SLINK_IDX_MAX]
    // phyTabTxTailSlink[SLINK_IDX_MAX]
    // phyTabRxPathDelaySlink[SLINK_IDX_MAX]
#endif

    /* SLINK SNR threshold */
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabSnrThSlink[PHY_TAB_SLINK_SF_LEN]
#endif

    /* Gain table */
    {
        // phyTabLNAGain[RFK_FREQ_MAX][PHY_TAB_LNA_GAIN_LEN]
#if (HALRF_CFG & HALRF_2G_SUPPORT)
        {0x00, 0x04, 0x09, 0x0C, 0x0E, 0x10, 0x14, 0x17, 0x19, 0x1A, 0x20, 0x25, 0x2A, 0x2D, 0x31, 0x31},
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
        {0x00, 0x06, 0x0D, 0x15, 0x1A, 0x20, 0x24, 0x2A, 0x30, 0x35, 0x3A, 0x3F, 0x45, 0x4B, 0x54, 0x53},
#if (0) // ( !HAL_SUBG_FREQ_BAND_MERGE_915M_868M )
        {0x00, 0x06, 0x0D, 0x15, 0x1A, 0x20, 0x24, 0x2A, 0x30, 0x35, 0x3A, 0x3F, 0x45, 0x4B, 0x54, 0x53},
#endif
        {0x00, 0x06, 0x0D, 0x15, 0x1A, 0x20, 0x24, 0x2A, 0x30, 0x35, 0x3A, 0x3F, 0x45, 0x4B, 0x54, 0x53},
        {0x00, 0x06, 0x0D, 0x15, 0x1A, 0x20, 0x24, 0x2A, 0x30, 0x35, 0x3A, 0x3F, 0x45, 0x4B, 0x54, 0x53},
#endif
    },
    // phyTabTIAGain[PHY_TAB_TIA_GAIN_LEN]
    {0x00, 0x05, 0x0C, 0x11, 0x18, 0x1F, 0x24, 0x29, 0x30, 0x36, 0x3D, 0x43, 0x49, 0x50, 0x56, 0x5D},
    // phyTabVGAGain[PHY_TAB_VGA_GAIN_LEN]
    {0x00, 0x06, 0x0C, 0x0C, 0x0C, 0x0C, 0x14, 0x18, 0x1E, 0x24, 0x2A, 0x30, 0x36, 0x3C, 0x3C, 0x3C},

    /* RSSI offset */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    // phyTabRssiOffset[IDX_MAX]
    {BYTE_INV_U8(0x00),     BYTE_INV_U8(0x00)},
#endif

    /* BW index to parameter index */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    // phyTabBwMap[BW_MAX]
    {
        BYTE_INV_U8(0x00),     BYTE_INV_U8(0x01),     BYTE_INV_U8(0x02),     BYTE_INV_U8(0x03),     BYTE_INV_U8(0x03),
        BYTE_INV_U8(0x03),     BYTE_INV_U8(0x03),     BYTE_INV_U8(0x03),     BYTE_INV_U8(0x03),
    },
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    // phyTabBwMapDclk[BW_MAX]
    {
        BYTE_INV_U8(0x00),     BYTE_INV_U8(0x01),     BYTE_INV_U8(0x02),     BYTE_INV_U8(0x03),     BYTE_INV_U8(0x03),
        BYTE_INV_U8(0x03),     BYTE_INV_U8(0x03),     BYTE_INV_U8(0x03),     BYTE_INV_U8(0x03),
    },
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    // phyTabSlinkBwMap[SLINK_BW_MAX]
#endif
};
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_PRTCL_CFG)
const sHAL_PRTCL_CFG extHalMemPrtclCfg =
{
    /* Common scheduler */
    BYTE_INV_U32(800),          // commDefInterval
    BYTE_INV_U8(2),             // scheGuardBase
    BYTE_INV_U8(40),            // schePickGuardBase
    BYTE_INV_U8(8),             // scheScanMinTimeBase
    BYTE_INV_U8(4),             // scheScanWinErlyAbrtBase
    BYTE_INV_U8(10),            // scheScanRxTOProtectBase
    BYTE_INV_U8(4),             // scheScanConnMDErlyAbrt1Base
    BYTE_INV_U8(2),             // scheScanConnMDErlyAbrt2Base
    BYTE_INV_U16(0xFFFF),       // scheMaxWeightVal
    BYTE_INV_U16(0x000F),       // scheScanFixedWeightVal
    BYTE_INV_U16(0x0028),       // scheInitFixedWeightVal

    /* BLE RX Proc Delay */
    BYTE_INV_U8(14),            // bleNextT2ROverhead
    BYTE_INV_U8(10),            // ble1mRxProcDelay
    BYTE_INV_U8(9),             // ble2mRxProcDelay
    BYTE_INV_U8(14),            // bleS2RxProcDelay
    BYTE_INV_U8(17),            // bleS8RxProcDelay
    BYTE_INV_U8(15),            // bleUnknownProcDelay

    BYTE_INV_U8(37),            // bleMasSlaCodedIfsS2OddLen
    BYTE_INV_U8(35),            // bleMasSlaCodedIfsS8OddLen
    BYTE_INV_U8(32),            // bleSlaCodedIfsS8OddLen

    BYTE_INV_U8(1),             // bleAsymCodedS2SpeedUpLenLevel
    BYTE_INV_U8(0),             // bleAsymCodedS8SpeedUpLenLevel

    /* BLE scheduler prepare offset */
    BYTE_INV_U16(625),          // bleAdvPrepareOffset;
    BYTE_INV_U16(625),          // bleScanPrepareOffset;
    BYTE_INV_U16(625),          // bleInitPrepareOffset;
    BYTE_INV_U16(1050),         // bleConnMasPrepareOffset;
    BYTE_INV_U16(900),          // bleConnSlaPrepareOffset;

    /* HTRP parameter */
    BYTE_INV_U8(0),             // htrpLongPeriodSleepEn;
};
#endif

/* REG_PATCH_HAL_INIT_BLE */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_START   EXT_HAL_MEM_ADDR_REG_PATCH + ALIGNED_BY(sizeof(sHAL_REG_PATCH), 4)
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_HAL_INIT_BLE)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem0[] =
{
    // {BYTE_INV_U16(0x0120),  BYTE_INV_U8(0x1),   BYTE_INV_U8(0x2),   BYTE_INV_U8(0x03)},
    // {BYTE_INV_U16(0x0121),  BYTE_INV_U8(0x6),   BYTE_INV_U8(0x7),   BYTE_INV_U8(0x03)},
    // {BYTE_INV_U16(0x0122),  BYTE_INV_U8(0x3),   BYTE_INV_U8(0x5),   BYTE_INV_U8(0x07)},
    // {BYTE_INV_U16(0x0122),  BYTE_INV_U8(0x3),   BYTE_INV_U8(0x5),   BYTE_INV_U8(0x07)},
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_START + sizeof(extHalMemRegPatchMem0)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_START
#endif

/* REG_PATCH_HAL_INIT_WISUN */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_HAL_INIT_WISUN)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem1[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_START + sizeof(extHalMemRegPatchMem1)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_START
#endif

/* REG_PATCH_HAL_INIT_ZIGBEE */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_HAL_INIT_ZIGBEE)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem2[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_START + sizeof(extHalMemRegPatchMem2)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_START
#endif

/* REG_PATCH_HAL_INIT_SLINK */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_HAL_INIT_SLINK)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem3[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_START + sizeof(extHalMemRegPatchMem3)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_START
#endif

/* REG_PATCH_TX_BW */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TX_BW)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem4[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_START + sizeof(extHalMemRegPatchMem4)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_START
#endif

/* REG_PATCH_RX_BW */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_RX_BW)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem5[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_START + sizeof(extHalMemRegPatchMem5)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_START
#endif

/* REG_PATCH_BMU_INIT */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_BMU_INIT)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem6[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_START + sizeof(extHalMemRegPatchMem6)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_START
#endif

/* REG_PATCH_BMU_MEM */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_BMU_MEM)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem7[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_START + sizeof(extHalMemRegPatchMem7)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_START
#endif

/* REG_PATCH_AGC_INIT */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_AGC_INIT)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem8[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_START + sizeof(extHalMemRegPatchMem8)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_START
#endif

/* REG_PATCH_AGC */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_START   EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_AGC)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem9[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_START + sizeof(extHalMemRegPatchMem9)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_END     EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_START
#endif

/* REG_PATCH_TCON_INIT */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_START  EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TCON_INIT)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem10[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_START + sizeof(extHalMemRegPatchMem10)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_START
#endif

/* REG_PATCH_TCON_TX */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_START  EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TCON_TX)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem11[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_START + sizeof(extHalMemRegPatchMem11)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_START
#endif

/* REG_PATCH_TCON_TX2M */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_START  EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TCON_TX2M)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem12[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_START + sizeof(extHalMemRegPatchMem12)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_START
#endif

/* REG_PATCH_REG_RESTORE */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_START  EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_REG_RESTORE)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem13[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_START + sizeof(extHalMemRegPatchMem13)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_START
#endif

/* REG_PATCH_EN_TX_EN */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_START  EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TX_EN)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem14[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_START + sizeof(extHalMemRegPatchMem14)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_START
#endif

/* REG_PATCH_EN_RX_EN */
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_START  EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_END
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_RX_EN)
const sHAL_REG_PATCH_DATA extHalMemRegPatchMem15[] =
{
    {BYTE_INV_U16(0x0000),  BYTE_INV_U8(0x0),   BYTE_INV_U8(0x0),   BYTE_INV_U8(0x00)},
};
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_START + sizeof(extHalMemRegPatchMem15)
#else
#define EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_END    EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_START
#endif

#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_REG_PATCH)
const sHAL_REG_PATCH extHalMemRegPatch =
{
    /* Patch enable */
    BYTE_INV_U16(HAL_REG_PATCH_EN),                     // enable

    /* Patch memory info. */
    {
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_END)},     // mem[0]:  REG_PATCH_HAL_INIT_BLE
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_END)},     // mem[1]:  REG_PATCH_HAL_INIT_WISUN
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_END)},     // mem[2]:  REG_PATCH_HAL_INIT_ZIGBEE
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_END)},     // mem[3]:  REG_PATCH_HAL_INIT_SLINK
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_END)},     // mem[4]:  REG_PATCH_TX_BW
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_END)},     // mem[5]:  REG_PATCH_RX_BW
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_END)},     // mem[6]:  REG_PATCH_BMU_INIT
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_END)},     // mem[7]:  REG_PATCH_BMU_MEM
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_END)},     // mem[8]:  REG_PATCH_AGC_INIT
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_START),   BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_END)},     // mem[9]:  REG_PATCH_AGC
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_START),  BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_END)},    // mem[10]: REG_PATCH_TCON_INIT
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_START),  BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_END)},    // mem[11]: REG_PATCH_TCON_TX
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_START),  BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_END)},    // mem[12]: REG_PATCH_TCON_TX2M
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_START),  BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_END)},    // mem[13]: REG_PATCH_REG_RESTORE
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_START),  BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_END)},    // mem[14]: REG_PATCH_TX_EN
        {BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_START),  BYTE_INV_U16(EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_END)},    // mem[15]: REG_PATCH_RX_EN
    }
};

#endif

const sHAL_PARA_IND extHalMemInd =
{
    /* Parameter source */
    BYTE_INV_U16(EXT_HAL_MEM_EN),                       // paraSrcCtrl. Using internal memory as default setting.

    /* Function manual mode */
    BYTE_INV_U16(HAL_MANUAL_CFG),                       // funcManualCtrl. Enabling Temp. sensor and voltage sensor manual mode as default setting.

    /* HAL FW configuration */
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_FW_CFG        + EXT_HAL_MEM_ADDR_OFFSET),                         // pFwCfg;

    /* HAL HW configuration */
    {
        // pHwCfg[HAL_MAX]
        BYTE_INV_U32(EXT_HAL_MEM_EN_HW_CFG_BLE + EXT_HAL_MEM_ADDR_OFFSET),
        BYTE_INV_U32(EXT_HAL_MEM_EN_HW_CFG_WISUN + EXT_HAL_MEM_ADDR_OFFSET),
        BYTE_INV_U32(EXT_HAL_MEM_EN_HW_CFG_ZIGBEE + EXT_HAL_MEM_ADDR_OFFSET),
        BYTE_INV_U32(EXT_HAL_MEM_EN_HW_CFG_SLINK + EXT_HAL_MEM_ADDR_OFFSET),
    },

    /* Default register setting */
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_DFT_TAB_MDM   + EXT_HAL_MEM_ADDR_OFFSET),                         // pMdmDefaultCfg;
#if (HALRF_CFG & HALRF_2G_SUPPORT)
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_DFT_TAB_RF    + EXT_HAL_MEM_ADDR_OFFSET),                         // pRfDefaultCfg2G;
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    // pRfDefaultCfgSubG
#endif
#if (HALRF_CFG & HALRF_2G_SUPPORT)
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_DFT_TAB_PMU   + EXT_HAL_MEM_ADDR_OFFSET),                         // pPmuDefaultCfg2G;
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    // pPmuDefaultCfgSubG;
#endif

    /* BMU memory size */
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_BMU_MEM_HWRXQ + EXT_HAL_MEM_ADDR_OFFSET),                         // pBmuMemSizeHwRxq;
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_BMU_MEM_HWSWRXQ + EXT_HAL_MEM_ADDR_OFFSET),                       // pBmuMemSizeHwSwRxq;

    /* HAL HW parameter indicater */
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_HW_PARA       + EXT_HAL_MEM_ADDR_OFFSET),                         // pHwPara;

    /* HAL HW table indicater */
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_HW_TAB        + EXT_HAL_MEM_ADDR_OFFSET),                         // pHwTab;

    /* BLE LL configuration */
#if (HAL_CFG & HAL_BLE_SUPPORT)
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_PRTCL_CFG     + EXT_HAL_MEM_ADDR_OFFSET),                         // pPrtclCfg;
#endif

    /* HAL register patch */
    BYTE_INV_U32(EXT_HAL_MEM_ADDR_REG_PATCH     + EXT_HAL_MEM_ADDR_OFFSET),                         // pRegPatch;
};


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
#if (RF_MCU_PATCH_SUPPORTED)
const patch_cfg_ctrl_t PATCH_CFG_LIST[] =
{
};

const uint32_t PATCH_CFG_NO = sizeof(PATCH_CFG_LIST) / sizeof(PATCH_CFG_LIST[0]);
#endif


/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
void RfMcu_PhyExtMemInit(void)
{
    if (EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_END > (EXT_HAL_MEM_START_ADDR + EXT_HAL_MEM_SIZE))
    {
        ASSERT();
    }

    /* Configure memory */
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_MEM_IND,
                    ((const uint8_t *)(&extHalMemInd)), ((uint16_t)(sizeof(extHalMemInd))));
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_FW_CFG)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_FW_CFG,
                    ((const uint8_t *)(&extHalMemFwCfg)), ((uint16_t)(sizeof(extHalMemFwCfg))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_HW_CFG)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_HW_CFG,
                    ((const uint8_t *)(&extHalMemHwCfg[0])), ((uint16_t)(sizeof(extHalMemHwCfg))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_DFT_TAB_MDM)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_DFT_TAB_MDM,
                    ((const uint8_t *)(extHalMemDftTabMdm)), ((uint16_t)(sizeof(extHalMemDftTabMdm))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_DFT_TAB_RF)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_DFT_TAB_RF,
                    ((const uint8_t *)(extHalMemDftTabRf)), ((uint16_t)(sizeof(extHalMemDftTabRf))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_DFT_TAB_PMU)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_DFT_TAB_PMU,
                    ((const uint8_t *)(extHalMemDftTabPmu)), ((uint16_t)(sizeof(extHalMemDftTabPmu))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_BMU_MEM)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_BMU_MEM_HWRXQ,
                    ((const uint8_t *)(extHalMemBmuMemHwRxq)), ((uint16_t)(sizeof(extHalMemBmuMemHwRxq))));
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_BMU_MEM_HWSWRXQ,
                    ((const uint8_t *)(extHalMemBmuMemHwSwRxq)), ((uint16_t)(sizeof(extHalMemBmuMemHwSwRxq))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_HW_PARA)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_HW_PARA,
                    ((const uint8_t *)(&extHalMemHwPara)), ((uint16_t)(sizeof(extHalMemHwPara))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_HW_TAB)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_HW_TAB,
                    ((const uint8_t *)(&extHalMemHwTab)), ((uint16_t)(sizeof(extHalMemHwTab))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_PRTCL_CFG)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_PRTCL_CFG,
                    ((const uint8_t *)(&extHalMemPrtclCfg)), ((uint16_t)(sizeof(extHalMemPrtclCfg))));
#endif
#if (EXT_HAL_MEM_EN & EXT_HAL_MEM_EN_REG_PATCH)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH,
                    ((const uint8_t *)(&extHalMemRegPatch)), ((uint16_t)(sizeof(extHalMemRegPatch))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_HAL_INIT_BLE)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM0_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem0)), ((uint16_t)(sizeof(extHalMemRegPatchMem0))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_HAL_INIT_WISUN)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM1_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem1)), ((uint16_t)(sizeof(extHalMemRegPatchMem1))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_HAL_INIT_ZIGBEE)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM2_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem2)), ((uint16_t)(sizeof(extHalMemRegPatchMem2))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_HAL_INIT_SLINK)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM3_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem3)), ((uint16_t)(sizeof(extHalMemRegPatchMem3))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TX_BW)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM4_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem4)), ((uint16_t)(sizeof(extHalMemRegPatchMem4))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_RX_BW)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM5_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem5)), ((uint16_t)(sizeof(extHalMemRegPatchMem5))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_BMU_INIT)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM6_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem6)), ((uint16_t)(sizeof(extHalMemRegPatchMem6))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_BMU_MEM)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM7_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem7)), ((uint16_t)(sizeof(extHalMemRegPatchMem7))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_AGC_INIT)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM8_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem8)), ((uint16_t)(sizeof(extHalMemRegPatchMem8))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_AGC)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM9_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem9)), ((uint16_t)(sizeof(extHalMemRegPatchMem9))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TCON_INIT)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM10_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem10)), ((uint16_t)(sizeof(extHalMemRegPatchMem10))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TCON_TX)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM11_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem11)), ((uint16_t)(sizeof(extHalMemRegPatchMem11))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TCON_TX2M)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM12_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem12)), ((uint16_t)(sizeof(extHalMemRegPatchMem12))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_REG_RESTORE)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM13_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem13)), ((uint16_t)(sizeof(extHalMemRegPatchMem13))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_TX_EN)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM14_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem14)), ((uint16_t)(sizeof(extHalMemRegPatchMem14))));
#endif
#if (HAL_REG_PATCH_EN & HAL_REG_PATCH_EN_RX_EN)
    RfMcu_MemorySet((uint16_t)EXT_HAL_MEM_ADDR_REG_PATCH_MEM15_START,
                    ((const uint8_t *)(&extHalMemRegPatchMem15)), ((uint16_t)(sizeof(extHalMemRegPatchMem15))));
#endif
}
#endif


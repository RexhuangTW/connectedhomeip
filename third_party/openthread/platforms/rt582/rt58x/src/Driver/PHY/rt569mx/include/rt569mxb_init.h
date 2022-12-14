/**************************************************************************//**
 * @file     rt569mxb_init.h
 * @version
 * @brief    host layer phy related configure

 ******************************************************************************/

#ifndef __RT569MXB_INIT_H__
#define __RT569MXB_INIT_H__

#include "rf_mcu_chip.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
*   CONSTANT AND DEFINE
*******************************************************************************/
#if ((RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0) && (RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_B))

/* HAL operation mode */
#define HAL_BLE_SUPPORT             (0x01)
#define HAL_WISUN_SUPPORT           (0x02)
#define HAL_ZIGBEE_SUPPORT          (0x04)
#define HAL_SLINK_SUPPORT           (0x08)
#define HAL_CFG                     (HAL_BLE_SUPPORT|HAL_ZIGBEE_SUPPORT)

/* RF band */
#define HALRF_2G_SUPPORT            (0x01)
#define HALRF_SUBG_SUPPORT          (0x02)
#define HALRF_CFG                   (HALRF_2G_SUPPORT)

/* System clock */
#define HAL_16M_CLK_SUPPORT         (0x01)
#define HAL_32M_CLK_SUPPORT         (0x02)
#define HAL_CLK_CFG                 (HAL_16M_CLK_SUPPORT|HAL_32M_CLK_SUPPORT)

/* Data BW */
#define HAL_BW_2M_SUPPORT           (0x0001)
#define HAL_BW_1M_SUPPORT           (0x0002)
#define HAL_BW_500K_SUPPORT         (0x0004)
#define HAL_BW_200K_SUPPORT         (0x0008)
#define HAL_BW_100K_SUPPORT         (0x0010)
#define HAL_BW_50K_SUPPORT          (0x0020)
#define HAL_BW_300K_SUPPORT         (0x0040)
#define HAL_BW_150K_SUPPORT         (0x0080)
#define HAL_BW_75K_SUPPORT          (0x0100)
#define HAL_SLINK_BW_500K_SUPPORT   (0x0001)
#define HAL_SLINK_BW_250K_SUPPORT   (0x0002)
#define HAL_SLINK_BW_125K_SUPPORT   (0x0004)
#define HAL_SLINK_BW_62P5K_SUPPORT  (0x0008)
#define HAL_BW_DCLK_CFG             (HAL_BW_2M_SUPPORT|HAL_BW_1M_SUPPORT|HAL_BW_500K_SUPPORT)
#define HAL_BW_NCLK_CFG             (HAL_BW_2M_SUPPORT|HAL_BW_1M_SUPPORT)
#define HAL_SLINK_BW_CFG            (0)

/* HAL setting determined by chip.h and project pre-definition */
#if (HAL_CLK_CFG == HAL_32M_CLK_SUPPORT)
#define HAL_BW_CFG                  (HAL_BW_DCLK_CFG)
#elif (HAL_CLK_CFG == HAL_16M_CLK_SUPPORT)
#define HAL_BW_CFG                  (HAL_BW_NCLK_CFG)
#else
#define HAL_BW_CFG                  (HAL_BW_NCLK_CFG|HAL_BW_DCLK_CFG)
#endif

/* BLE Coded PHY support */
#if ((HAL_CFG & HAL_BLE_SUPPORT) && (HAL_BW_CFG & HAL_BW_1M_SUPPORT))
#define HAL_CODED_PHY_SUPPORT       (TRUE)
#else
#define HAL_CODED_PHY_SUPPORT       (FALSE)
#endif

#define IDX_MAX                     (3)
#define IDX_MAX_DCLK                (3)
#define BW_MAX                      (9)
#define PHY_TAB_RX_FIR_S2_LEN       (16)
#define PHY_TAB_RX_FIR_S1_LEN       (8)
#define PHY_TAB_TX_FIR_LEN          (16)
#define PHY_TAB_LNA_GAIN_LEN        (16)
#define PHY_TAB_TIA_GAIN_LEN        (16)
#define PHY_TAB_VGA_GAIN_LEN        (16)

#pragma pack(push)
#pragma pack(1)

typedef struct HAL_FW_CFG
{
    /* Parameter source */
    uint16_t    paraSrcCtrl;

    /* Function manual mode */
    uint16_t    funcManualCtrl;

    /* DIO control */
    uint8_t     dioType;
    uint8_t     dioDbgType;

    /* System Sleep Control */
    uint8_t     sysSlpLevel;
    uint16_t    sysSlpGurdTime;

    /* Watchdog timer of RF mechanism */
    uint32_t    watchdogTimer;

    /* External FEM setting */
    uint8_t     extFemEnable;
    uint8_t     extFemTxPin;
    uint8_t     extFemRxPin;
    uint8_t     extFemTxActType;
    uint8_t     extFemRxActType;

} sHAL_FW_CFG, *pHAL_FW_CFG;

typedef struct HAL_HW_CFG
{
    /* SYSTEM */
    uint16_t    halType;                        // HAL type, BLE, Zigbee, Wisun, or Slink
    uint16_t    rfBand;                         // RF band, Sub-1G or 2.4G
    uint16_t    clkMode;                        // System clock, 16MHz or 32MHz
    uint16_t    rxMaxPktLen;                    // RX maximum packet length in byte

    /* Preamble */
    uint16_t    rxPreambleBase;                 // RX preamble base, 0 or 1
    uint16_t    txPreambleBase;                 // TX preamble base, 0 or 1

    /* SFD */
    uint16_t    rxSfdType;                      // RX SFD type, 2 bytes or 4 bytes
    uint16_t    txSfdType;                      // TX SFD type, 2 bytes or 4 bytes
    uint32_t    sfdData[2];                     // Default SFD content, index 0 for SFD-4-byte, index 1 for SFD-2-byte

    /* PHR */
    uint16_t    phrType;                        // PHR type, enable or disable

    /* MHR */
    uint8_t     mhrRptLen;                      // MHR report length in byte

    /* CRC */
    uint16_t    crcType;                        // CRC type, CRC-16, CRC-24, CRC-32 or disable CRC
    uint32_t    crcInit[3];                     // Default CRC initial value, index 0 for CRC-16, ...

    /* Whitening */
    uint16_t    whitenType;                     // Whitening type, enable or disable
    uint8_t     whitenInit;                     // Default Whitening initial value

    /* BIT endian */
    uint16_t    bitEndian;                      // Payload and CRC bit endian for Wisun MAC

    /* Modem */
    uint16_t    modIdx;                         // Modulation index, 0.5 or 1
    uint16_t    gaussianFiltType;               // Gaussian filter type

    /* BMU */
    uint16_t    rxqCtrl;                        // RXQ post-process method, automatic or manual
    uint8_t     rxCtrlLen;                      // RX control field length
    uint16_t    txqCtrl;                        // TXQ post-process method, automatic or manual
    uint8_t     txCtrlLen;                      // TX control field length
} sHAL_HW_CFG, *pHAL_HW_CFG;

/* HAL HW parameter indicater */
typedef struct HAL_HW_RFK_PARA_IND
{
    uint16_t    txIf;
    uint16_t    txIfOffset;
    uint16_t    rxIf;
    uint8_t     rxBwSel;
    uint8_t     txBwSel;
    uint8_t     ifSel;
    uint8_t     rxStValI;
    uint8_t     rxRxFiltPw;
    uint8_t     lnaGainB;
    uint8_t     tiaGain;
    uint8_t     vgaGain;
    uint8_t     txBufGain0Man;
    uint8_t     txBufGain1Man;
    uint8_t     txBufGain2Man;
    uint8_t     txBufGain3Man;
    uint8_t     txBufPw;
    uint8_t     rxFiltCcomp;
    uint8_t     pwVco;
} sHAL_HW_RFK_PARA_IND;

typedef struct HAL_HW_PARA_IND
{
    /* Sync. bit threshold */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    uint8_t     syncBitThWisunSfd2;
    uint8_t     syncBitThWisunSfd4;
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint8_t     syncBitThBle1M;
    uint8_t     syncBitThBle2M;
#if (HAL_CODED_PHY_SUPPORT)
    uint8_t     syncBitThBleCodedPhy;
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     syncBitThZigbee;
#endif

    /* Sync. correlation threshold */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    uint16_t    syncCorrThWisunSfd2;
    uint16_t    syncCorrThWisunSfd4;
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint16_t    syncCorrThBle1M;
    uint16_t    syncCorrThBle2M;
#if (HAL_CODED_PHY_SUPPORT)
    uint16_t    syncCorrThBleCodedPhy;
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint16_t    syncCorrThZigbee;
#endif

    /* Tracking bit threshold */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    uint8_t     trackBitThWisunSfd2;
    uint8_t     trackBitThWisunSfd4;
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint8_t     trackBitThBle1M;
    uint8_t     trackBitThBle2M;
#if (HAL_CODED_PHY_SUPPORT)
    uint8_t     trackBitThBleCodedPhy;
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     trackBitThZigbee;
#endif

    /* Tracking correlation threshold */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    uint16_t    trackCorrThWisunSfd2;
    uint16_t    trackCorrThWisunSfd4;
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint16_t    trackCorrThBle1M;
    uint16_t    trackCorrThBle2M;
#if (HAL_CODED_PHY_SUPPORT)
    uint16_t    trackCorrThBleCodedPhy;
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint16_t    trackCorrThZigbee;
#endif

    /* RX number of preamble base */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    uint8_t     rxNumPreamBaseWisun;
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint8_t     rxNumPreamBaseBle1M;
    uint8_t     rxNumPreamBaseBle2M;
#if (HAL_CODED_PHY_SUPPORT)
    uint8_t     rxNumPreamBaseBleCodedPhy;
#endif
#endif

    /* Pre-CFO */
    uint8_t     preCfoEnHighBw;
    uint8_t     preCfoEnLowBw;

    /* CFO estimation */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT))
    uint8_t     cfoEstAlphaIdxCodedPhy;
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     cfoEstAlphaIdxZigbee;
#endif

    /* OQPSK symbol sync alpha index */
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint8_t     symbSyncAlpha0Ble;
    uint8_t     symbSyncAlpha1Ble;
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     symbSyncAlpha0Zigbee;
    uint8_t     symbSyncAlpha1Zigbee;
#endif

    /* Detection offset */
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    uint8_t     detectOffsetWisun;
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint8_t     detectOffsetBle1M;
    uint8_t     detectOffsetBle2M;
#if (HAL_CODED_PHY_SUPPORT)
    uint8_t     detectOffsetBleCodedPhy;
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     detectOffsetZigbee;
#endif

    /* Coded PHY related */
#if ((HAL_CFG & HAL_BLE_SUPPORT) && (HAL_CODED_PHY_SUPPORT))
    uint8_t     txPreamNumCodedPhy;
    uint8_t     bitScoreCodedPhy;
    uint16_t    syncBitThCodedPhy;
    uint16_t    syncCorrThCodedPhy;
    uint8_t     trackKpCodedPhy;
    uint8_t     trackKqCodedPhy;
#endif

    /* Zigbee related */
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     lSfdCheckZigbee;
    uint16_t    despreadSoftThZigbee;
    uint8_t     chipValueBoundZigbee;
    uint8_t     btIdxZigbee;
    uint8_t     extraPreamHbZigbee;
    uint8_t     extraPreamLbZigbee;
    uint8_t     trackKpZigbee;
    uint8_t     trackKqZigbee;
#endif

    /* SLINK related */
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint16_t    txUpSampN0Slink;
    uint16_t    txUpSampN1Slink;
    uint8_t     txDownSampM0Slink;
    uint8_t     syncCfoTrackEnG0Slink;
    uint8_t     syncCfoTrackEnG1Slink;
    uint8_t     syncCfoTrackEnG2Slink;
    uint8_t     alphaEstErrG0Slink;
    uint8_t     alphaEstErrG1Slink;
    uint8_t     alphaEstErrG2Slink;
    uint8_t     upsampType0Slink;
    uint8_t     upsampType1Slink;
    uint8_t     cadEnSlink;
    uint8_t     cadSnrThSlink;
#endif

    /* AGC */
#if (HALRF_CFG & HALRF_2G_SUPPORT)
    uint8_t     lnaGain2G;
    uint8_t     tiaGain2G;
    uint8_t     vgaGain2G;
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    uint8_t     lnaGainSubG;
    uint8_t     tiaGainSubG;
    uint8_t     vgaGainSubG;
#endif
    uint8_t     rssiOffset;

    /* BMU */
    uint8_t     txqDepth;
    uint8_t     rxqDepth;
    uint8_t     cteRoundMode;
    uint8_t     cteDownSampleMask;
    uint8_t     txqThHwRxq;
    uint8_t     rxqThHwRxq;
    uint8_t     emptyThHwRxq;
    uint8_t     txqThHwSwRxq;
    uint8_t     rxqThHwSwRxq;
    uint8_t     emptyThHwSwRxq;

    /* PLL */
    uint8_t     pllRefClock;

    /* TCON */
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0)
    uint16_t    pllLockTime;
    uint16_t    paOnTime;
    uint16_t    rampOnTime;
    uint16_t    txOnTime;
    uint16_t    paOffTime;
    uint16_t    rampOffTime;
    uint16_t    txOffTime;
    uint16_t    rampOnOffset;
    uint16_t    rxOnTime;
#elif (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569T3)
#if (HALRF_CFG & HALRF_2G_SUPPORT)
    uint16_t    pllLockTime;
    uint16_t    paOnTime;
    uint16_t    txOnTime;
    uint16_t    paOffTime;
    uint16_t    rxOnTime;
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    uint16_t    pllLockTimeSubg;
    uint16_t    paOnTimeSubg;
    uint16_t    txOnTimeSubg;
    uint16_t    paOffTimeSubg;
    uint16_t    rxOnTimeSubg;
#endif
#endif

    /* TX power compensation */
    uint8_t     chSeg[3];

    /* 2.4GHz RF modulation type */
    uint8_t     zeroIfEn;

    /* PMU operation mode */
    uint8_t     pmuOpMode;

    /* RF Calibration */
    sHAL_HW_RFK_PARA_IND rfkPara[4];
} sHAL_HW_PARA_IND, *pHAL_HW_PARA_IND;

/* HAL HW table indicater */
typedef struct HAL_HW_TAB_IND
{
    /* RF TX\RX BW and IF */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    uint8_t     rfTabTxBw[IDX_MAX];
    uint8_t     rfTabRxBw[IDX_MAX];
    uint8_t     rfTabRxIf[IDX_MAX];
#if ((RF_MCU_CHIP_TYPE == RF_MCU_TYPE_FPGA_RFM0) || (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0))
    uint8_t     rfTabRxFiltOpComp[IDX_MAX];
    uint8_t     rfTabRxFiltPw[IDX_MAX];
    uint8_t     rfTabRxFiltVcm[IDX_MAX];
    uint8_t     rfTabRxFiltCcomp[IDX_MAX];
    uint8_t     rfTabVgaOpComp[IDX_MAX];
    uint8_t     rfTabRxVgaPw[IDX_MAX];
#endif
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    uint8_t     rfTabTxBwDClk[IDX_MAX_DCLK];
    uint8_t     rfTabRxBwDClk[IDX_MAX_DCLK];
    uint8_t     rfTabRxIfDClk[IDX_MAX_DCLK];
#if ((CHIP_TYPE == TYPE_FPGA_RFM0) || (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0))
    uint8_t     rfTabRxFiltOpCompDclk[IDX_MAX_DCLK];
    uint8_t     rfTabRxFiltPwDclk[IDX_MAX_DCLK];
    uint8_t     rfTabRxFiltVcmDclk[IDX_MAX_DCLK];
    uint8_t     rfTabRxFiltCcompDclk[IDX_MAX_DCLK];
    uint8_t     rfTabVgaOpCompDclk[IDX_MAX_DCLK];
    uint8_t     rfTabRxVgaPwDclk[IDX_MAX_DCLK];
#endif
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     rfTabSlinkTxBw[SLINK_IDX_MAX];
    uint8_t     rfTabSlinkRxBw[SLINK_IDX_MAX];
    uint8_t     rfTabSlinkRxIf[SLINK_IDX_MAX];
#endif

    /* One point modulation */
#if (0) // (HAL_ONE_POINT_EN)
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569MP)
    uint8_t     rfTabOpPwCp[IDX_MAX];
    uint8_t     rfTabOpSelRz[IDX_MAX];
    uint8_t     rfTabOpSelCz[IDX_MAX];
    uint8_t     rfTabOpSelCp[IDX_MAX];
    uint8_t     rfTabSdmScale[RF_FREQ_MAX][MOD_MAX][IDX_MAX];
    uint16_t    phyTabModIdx[RF_FREQ_MAX][MOD_MAX][IDX_MAX];
#endif
#endif

    /* RX FIR S2 */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT))
    uint8_t     phyTabRxFirS2[PHY_TAB_RX_FIR_S2_LEN];
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    uint8_t     phyTabRxFirS2ModIdx1[PHY_TAB_RX_FIR_S2_LEN];
#endif
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     phyTabRxFirS2Zigbee[PHY_TAB_RX_FIR_S2_LEN];
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabRxFirS2Slink[SLINK_IDX_MAX][PHY_TAB_RX_FIR_S2_LEN] ;
#endif

    /* RX FIR S1 */
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569T3)
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    uint8_t     phyTabRxFirS1[3][PHY_TAB_RX_FIR_S1_LEN];
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabRxFirS1Slink[SLINK_IDX_MAX][PHY_TAB_RX_FIR_S1_LEN];
#endif
#else
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    uint8_t     phyTabRxFirS1[IDX_MAX][PHY_TAB_RX_FIR_S1_LEN];
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabRxFirS1Slink[SLINK_IDX_MAX][PHY_TAB_RX_FIR_S1_LEN];
#endif
#endif

    /* RX FIR S0 */
#if ((RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0) && (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT)))
    uint8_t     phyTabRxFirS0[2][PHY_TAB_RX_FIR_S0_LEN];
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabRxFirS0Slink[SLINK_IDX_MAX][PHY_TAB_RX_FIR_S0_LEN];
#endif

    /* RX resample FIR */
#if(RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    uint8_t     phyTabRxResampFir[PHY_TAB_RX_RESAMP_FIR_LEN];
#endif

    /* RX downsampling factor */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    uint8_t     phyTabRxDownFact0[IDX_MAX];
#endif
    uint8_t     phyTabRxDownFact1[IDX_MAX];
#if (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    uint8_t     phyTabRxDownFact2[IDX_MAX];
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabRxDownFact0Slink[SLINK_IDX_MAX];
    uint8_t     phyTabRxDownFact1Slink[SLINK_IDX_MAX];
    uint8_t     phyTabRxDownFact2Slink[SLINK_IDX_MAX];
#endif

    /* TX FIR */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT))
    uint8_t     phyTabTxFir[PHY_TAB_TX_FIR_LEN];
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     phyTabTxFirZigbee[PHY_TAB_TX_FIR_LEN];
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabTxFirSlink[PHY_TAB_TX_FIR_LEN];
#endif

    /* TX extra FIR for SLINK*/
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabTxUFir0Slink[PHY_TAB_TX_UFIR0_SLINK_LEN];
    uint8_t     phyTabTxUFir1Slink[SLINK_IDX_MAX][PHY_TAB_TX_UFIR0_SLINK_LEN];
#endif

    /* TX resample FIR */
#if (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0)
    uint8_t     phyTabTxResampFir[2][PHY_TAB_TX_RESAMP_FIR_LEN];
#endif

    /* TX resampling factor */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    uint8_t     phyTabTxUpsampN0[IDX_MAX];
    uint8_t     phyTabTxDownsampM0[IDX_MAX];
    uint8_t     phyTabTxUpsampN1[IDX_MAX];
#endif

    /*  */
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabTxUpFactorSlink[2][SLINK_IDX_MAX];
#endif

    /* PHY RX tracking parameter */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if ((HAL_CLK_CFG & HAL_16M_CLK_SUPPORT) && (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0))
    uint8_t     phyTabRxPreSyncAlpha0[IDX_MAX];
    uint8_t     phyTabRxPreSyncAlpha1[IDX_MAX];
#endif
#if ((HAL_CLK_CFG & HAL_32M_CLK_SUPPORT) && (RF_MCU_CHIP_MODEL != RF_MCU_CHIP_569M0))
    uint8_t     phyTabRxPreSyncAlpha0Dclk[IDX_MAX_DCLK];
    uint8_t     phyTabRxPreSyncAlpha1Dclk[IDX_MAX_DCLK];
#endif
    uint8_t     phyTabRxTrackingKp[IDX_MAX];
    uint8_t     phyTabRxTrackingKq[IDX_MAX];
    uint8_t     phyTabRxCfoOffsetAlpha[IDX_MAX];
#endif

    /* PHY IF */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    uint8_t     phyTabIf[IDX_MAX];
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    uint8_t     phyTabIfDClk[IDX_MAX_DCLK];
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabIfSlink[SLINK_IDX_MAX];
#endif

    /* TX preamble number */
#if (HAL_CFG & HAL_BLE_SUPPORT)
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    uint8_t     phyTabTxPreamNumBLE[IDX_MAX];
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    uint8_t     phyTabTxPreamNumBLEDClk[IDX_MAX_DCLK];
#endif
#endif
#if (HAL_CFG & HAL_WISUN_SUPPORT)
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    uint8_t     phyTabTxPreamNumWisun[IDX_MAX];
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    uint8_t     phyTabTxPreamNumWisunDClk[IDX_MAX_DCLK];
#endif
#endif
#if (RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569T3)
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
#if (HAL_CLK_CFG & HAL_16M_CLK_SUPPORT)
    uint8_t     phyTabTxDummyPreamble[IDX_MAX];
    uint8_t     phyTabTxDummyPreambleTime[IDX_MAX];
#endif
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    uint8_t     phyTabTxDummyPreambleDClk[IDX_MAX_DCLK];
    uint8_t     phyTabTxDummyPreambleTimeDClk[IDX_MAX_DCLK];
#endif
#endif
#endif

    /* PHY data maping table */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    uint8_t     phyTabBw[IDX_MAX];
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    uint8_t     phyTabBwDClk[IDX_MAX_DCLK];
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabBwSlink[SLINK_IDX_MAX];
#endif

    /* Group delay (unit: us) */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    uint8_t     phyTabTxDelay[IDX_MAX];
    uint8_t     phyTabTxTail[IDX_MAX];
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT))
    uint8_t     phyTabRxPathDelayFsk[IDX_MAX];
#endif
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint8_t     phyTabRxPathDelayCodedPhy[2];
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint8_t     phyTabRxPathDelayOqpsk[IDX_MAX];
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabTxDelaySlink[SLINK_IDX_MAX];
    uint8_t     phyTabTxTailSlink[SLINK_IDX_MAX];
    uint8_t     phyTabRxPathDelaySlink[SLINK_IDX_MAX];
#endif

    /* SLINK SNR threshold */
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabSnrThSlink[PHY_TAB_SLINK_SF_LEN];
#endif

    /* Gain table */
#if (HALRF_CFG & HALRF_2G_SUPPORT)
    uint8_t     phyTabLNAGain2G[PHY_TAB_LNA_GAIN_LEN];
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    uint8_t     phyTabLNAGainSubG[PHY_TAB_LNA_GAIN_LEN];
#endif
    uint8_t     phyTabTIAGain[PHY_TAB_TIA_GAIN_LEN];
    uint8_t     phyTabVGAGain[PHY_TAB_VGA_GAIN_LEN];

    /* RSSI offset */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    uint8_t     phyTabRssiOffset[IDX_MAX];
#endif

    /* BW index to parameter index */
#if (HAL_CFG & (HAL_BLE_SUPPORT|HAL_WISUN_SUPPORT|HAL_ZIGBEE_SUPPORT))
    uint8_t     phyTabBwMap[BW_MAX];
#if (HAL_CLK_CFG & HAL_32M_CLK_SUPPORT)
    uint8_t     phyTabBwMapDclk[BW_MAX];
#endif
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint8_t     phyTabSlinkBwMap[SLINK_BW_MAX];
#endif
} sHAL_HW_TAB_IND, *pHAL_HW_TAB_IND;

typedef struct HAL_PRTCL_CFG
{
    /* Common scheduler */
    uint32_t    commDefInterval;                        //
    uint8_t     scheGuardBase;                          // LL_PARA_SCHE_GUARD_VAL
    uint8_t     schePickGuardBase;                      // LL_PARA_SCHE_PICK_GUARD_TIME
    uint8_t     scheScanMinTimeBase;                    // LL_PARA_SCHE_SCAN_MINIMUM_TIME
    uint8_t     scheScanWinErlyAbrtBase;                // LL_PARA_SCAN_WINDOW_EARLY_ABORT
    uint8_t     scheScanRxTOProtectBase;                // LL_PARA_SCAN_RX_TIMEOUT_PROTECT
    uint8_t     scheScanConnMDErlyAbrt1Base;            // LL_PARA_SCHE_CONN_MD_EARLY_ABORT
    uint8_t     scheScanConnMDErlyAbrt2Base;            // LL_PARA_SCHE_CONN_MD_EARLY_ABORT
    uint16_t    scheMaxWeightVal;                       // LL_PARA_SCHE_MAX_WEIGHT_VAL
    uint16_t    scheScanFixedWeightVal;                 // LL_PARA_SCHE_SCAN_FIXED_WEIGHT
    uint16_t    scheInitFixedWeightVal;                 // LL_PARA_SCHE_INIT_FIXED_WEIGHT

    /* BLE RX Proc Delay */
    uint8_t     bleNextT2ROverhead;                     // TX to RX overhead to adjust TX to RX timing
    uint8_t     ble1mRxProcDelay;                       // 1M RX to TX delay
    uint8_t     ble2mRxProcDelay;                       // 2M RX to TX delay
    uint8_t     bleS2RxProcDelay;                       // Coded Phy S2 RX to TX delay
    uint8_t     bleS8RxProcDelay;                       // Coded Phy S8 RX to TX delay
    uint8_t     bleUnknownProcDelay;                    // Coded Unknown RX tp TX delay
    uint8_t     bleMasSlaCodedIfsS2OddLen;              // Coded Phy master S2 Odd Length
    uint8_t     bleMasSlaCodedIfsS8OddLen;              // Coded Phy master S8 Odd Length
    uint8_t     bleSlaCodedIfsS8OddLen;                 // Coded Phy slave S8 Odd Length

    uint8_t     bleAsymCodedS2SpeedUpLenLevel;          // Asymetric coded phy S2 len level to speed up
    uint8_t     bleAsymCodedS8SpeedUpLenLevel;          // Asymetric coded phy S8 len level to speed up

    /* BLE scheduler prepare offset */
    uint16_t    bleAdvPrepareOffset;                    //
    uint16_t    bleScanPrepareOffset;                   //
    uint16_t    bleInitPrepareOffset;                   //
    uint16_t    bleConnMasPrepareOffset;                //
    uint16_t    bleConnSlaPrepareOffset;                //

    /* HTRP parameter */
    uint8_t     htrpLongPeriodSleepEn;                  // Enable sleep mode in HTRP long period

} sHAL_PRTCL_CFG, *pHAL_PRTCL_CFG;

typedef struct HAL_REG_PATCH_DATA
{
    uint16_t    addr;
    uint8_t     bitStart: 4;
    uint8_t     bitEnd: 4;
    uint8_t     value;
} sHAL_REG_PATCH_DATA, *pHAL_REG_PATCH_DATA;

typedef struct HAL_REG_PATCH_MEM
{
    uint16_t    start;
    uint16_t    end;
} sHAL_REG_PATCH_MEM, *pHAL_REG_PATCH_MEM;

typedef struct HAL_REG_PATCH
{
    /* Patch enable */
    uint16_t    enable;

    /* Patch memory info. */
    sHAL_REG_PATCH_MEM  mem[15];
} sHAL_REG_PATCH, *pHAL_REG_PATCH;

/* HAL memory indicater */
typedef struct HAL_PARA_IND
{
    /* HAL FW configuration */
    uint32_t    pFwCfg;

    /* HAL HW configuration */
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint32_t    pHwCfgBLE;
#endif
#if (HAL_CFG & HAL_WISUN_SUPPORT)
    uint32_t    pHwCfgWisun;
#endif
#if (HAL_CFG & HAL_ZIGBEE_SUPPORT)
    uint32_t    pHwCfgZigbee;
#endif
#if (HAL_CFG & HAL_SLINK_SUPPORT)
    uint32_t    pHwCfgSlink;
#endif

    /* Default register setting */
    uint32_t    pMdmDefaultCfg;
#if (HALRF_CFG & HALRF_2G_SUPPORT)
    uint32_t    pRfDefaultCfg2G;
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    uint32_t    pRfDefaultCfgSubG;
#endif
#if (HALRF_CFG & HALRF_2G_SUPPORT)
    uint32_t    pPmuDefaultCfg2G;
#endif
#if (HALRF_CFG & HALRF_SUBG_SUPPORT)
    uint32_t    pPmuDefaultCfgSubG;
#endif

    /* BMU memory size */
    uint32_t    pBmuMemSizeHwRxq;
    uint32_t    pBmuMemSizeHwSwRxq;

    /* HAL HW parameter indicater */
    uint32_t    pHwPara;

    /* HAL HW table indicater */
    uint32_t    pHwTab;

    /* BLE LL configuration */
#if (HAL_CFG & HAL_BLE_SUPPORT)
    uint32_t    pPrtclCfg;
#endif

    /* HAL register patch */
    uint32_t    pRegPatch;

} sHAL_PARA_IND, *pHAL_PARA_IND;

#pragma pack(pop)
#endif

/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
#if ((RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0) && (RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_B))
void RfMcu_PhyExtMemInit(void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* __RT569MXB_INIT_H__ */


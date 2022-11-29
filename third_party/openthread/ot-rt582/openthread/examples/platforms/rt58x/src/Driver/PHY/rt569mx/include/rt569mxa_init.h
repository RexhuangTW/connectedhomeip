/**************************************************************************//**
 * @file     rt569mxa_init.h
 * @version
 * @brief    host layer phy related configure

 ******************************************************************************/

#ifndef __RT569MXA_INIT_H__
#define __RT569MXA_INIT_H__

#include "rf_mcu_chip.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
*   CONSTANT AND DEFINE
*******************************************************************************/
#if ((RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0) && (RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_A))
#define IDX_MAX                                     (2)
#define BW_MAX                                      (9)
#define PHY_TAB_RX_FIR_S2_LEN                       (16)
#define PHY_TAB_RX_FIR_S1_LEN                       (8)
#define PHY_TAB_TX_FIR_LEN                          (16)
#define PHY_TAB_LNA_GAIN_LEN                        (16)
#define PHY_TAB_TIA_GAIN_LEN                        (16)
#define PHY_TAB_VGA_GAIN_LEN                        (16)

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

    /* BMU */
    uint16_t    rxqCtrl;                        // RXQ post-process method, automatic or manual
    uint8_t     rxCtrlLen;                      // RX control field length
    uint16_t    txqCtrl;                        // TXQ post-process method, automatic or manual
    uint8_t     txCtrlLen;                      // TX control field length
} sHAL_HW_CFG, *pHAL_HW_CFG;

/* HAL HW parameter indicater */
typedef struct HAL_HW_PARA_IND
{
    /* Sync. bit threshold */
    uint8_t     syncBitThBle1M;
    uint8_t     syncBitThBle2M;
    uint8_t     syncBitThBleCodedPhy;
    uint8_t     syncBitThZigbee;

    /* Sync. correlation threshold */
    uint16_t    syncCorrThBle1M;
    uint16_t    syncCorrThBle2M;
    uint16_t    syncCorrThBleCodedPhy;
    uint16_t    syncCorrThZigbee;

    /* Tracking bit threshold */
    uint8_t     trackBitThBle1M;
    uint8_t     trackBitThBle2M;
    uint8_t     trackBitThBleCodedPhy;
    uint8_t     trackBitThZigbee;

    /* Tracking correlation threshold */
    uint16_t    trackCorrThBle1M;
    uint16_t    trackCorrThBle2M;
    uint16_t    trackCorrThBleCodedPhy;
    uint16_t    trackCorrThZigbee;

    /* RX number of preamble base */
    uint8_t     rxNumPreamBaseBle1M;
    uint8_t     rxNumPreamBaseBle2M;
    uint8_t     rxNumPreamBaseBleCodedPhy;

    /* Pre-CFO */
    uint8_t     preCfoEnHighBw;
    uint8_t     preCfoEnLowBw;

    /* CFO estimation */
    uint8_t     cfoEstAlphaIdxCodedPhy;
    uint8_t     cfoEstAlphaIdxZigbee;

    /* OQPSK symbol sync alpha index */
    uint8_t     symbSyncAlpha0Ble;
    uint8_t     symbSyncAlpha1Ble;
    uint8_t     symbSyncAlpha0Zigbee;
    uint8_t     symbSyncAlpha1Zigbee;

    /* Detection offset */
    uint8_t     detectOffsetBle1M;
    uint8_t     detectOffsetBle2M;
    uint8_t     detectOffsetBleCodedPhy;
    uint8_t     detectOffsetZigbee;

    /* Coded PHY related */
    uint8_t     txPreamNumCodedPhy;
    uint8_t     bitScoreCodedPhy;
    uint16_t    syncBitThCodedPhy;
    uint16_t    syncCorrThCodedPhy;
    uint8_t     trackKpCodedPhy;
    uint8_t     trackKqCodedPhy;

    /* Zigbee related */
    uint8_t     lSfdCheckZigbee;
    uint16_t    despreadSoftThZigbee;
    uint8_t     chipValueBoundZigbee;
    uint8_t     btIdxZigbee;
    uint8_t     extraPreamHbZigbee;
    uint8_t     extraPreamLbZigbee;
    uint8_t     trackKpZigbee;
    uint8_t     trackKqZigbee;

    /* AGC */
    uint8_t     lnaGain2G;
    uint8_t     tiaGain2G;
    uint8_t     vgaGain2G;

    /* BMU */
    uint8_t     txqDepth;
    uint8_t     rxqDepth;
    uint8_t     cteRoundMode;
    uint8_t     cteDownSampleMask;
    uint8_t     txqTh;
    uint8_t     rxqTh;
    uint8_t     emptyTh;

    /* PLL */
    uint8_t     pllRefClock;

    /* TCON */
    uint16_t    pllLockTime;
    uint16_t    paOnTime;
    uint16_t    rampOnTime;
    uint16_t    txOnTime;
    uint16_t    paOffTime;
    uint16_t    rampOffTime;
    uint16_t    txOffTime;
    uint16_t    rampOnOffset;
    uint16_t    rxOnTime;
} sHAL_HW_PARA_IND, *pHAL_HW_PARA_IND;

/* HAL HW table indicater */
typedef struct HAL_HW_TAB_IND
{
    /* RF TX\RX BW and IF */
    uint8_t     rfTabTxBw[IDX_MAX];
    uint8_t     rfTabRxBw[IDX_MAX];
    uint8_t     rfTabRxIf[IDX_MAX];
    uint8_t     rfTabTxLpfPw[IDX_MAX];
    uint8_t     rfTabTxLpfBiasR[IDX_MAX];
    uint8_t     rfTabRxFiltPw[IDX_MAX];
    uint8_t     rfTabRxFiltBiasR[IDX_MAX];
    uint8_t     rfTabRxVgaPw[IDX_MAX];
    uint8_t     rfTabRxVgaBiasR[IDX_MAX];

    /* RX FIR S2 */
    uint8_t     phyTabRxFirS2[PHY_TAB_RX_FIR_S2_LEN];
    uint8_t     phyTabRxFirS2Zigbee[PHY_TAB_RX_FIR_S2_LEN];

    /* RX FIR S1 */
    uint8_t     phyTabRxFirS1[IDX_MAX][PHY_TAB_RX_FIR_S1_LEN];

    // /* RX FIR S0 */
    // uint8_t     phyTabRxFirS0[2][PHY_TAB_RX_FIR_S0_LEN];

    // /* RX resample FIR */
    // uint8_t     phyTabRxResampFir[PHY_TAB_RX_RESAMP_FIR_LEN];

    /* RX downsampling factor */
    // uint8_t     phyTabRxDownFact0[IDX_MAX];
    uint8_t     phyTabRxDownFact1[IDX_MAX];
    // uint8_t     phyTabRxDownFact2[IDX_MAX];

    /* TX FIR */
    uint8_t     phyTabTxFir[PHY_TAB_TX_FIR_LEN];
    uint8_t     phyTabTxFirZigbee[PHY_TAB_TX_FIR_LEN];

    // /* TX resample FIR */
    // uint8_t     phyTabTxResampFir[2][PHY_TAB_TX_RESAMP_FIR_LEN];

    /* TX resampling factor */
    uint8_t     phyTabTxUpsampN0[IDX_MAX];
    uint8_t     phyTabTxDownsampM0[IDX_MAX];
    uint8_t     phyTabTxUpsampN1[IDX_MAX];

    /* PHY RX tracking parameter */
    // uint8_t     phyTabRxPreSyncAlpha0[IDX_MAX];
    // uint8_t     phyTabRxPreSyncAlpha1[IDX_MAX];
    uint8_t     phyTabRxTrackingKp[IDX_MAX];
    uint8_t     phyTabRxTrackingKq[IDX_MAX];
    uint8_t     phyTabRxCfoOffsetAlpha[IDX_MAX];

    /* PHY IF */
    uint16_t     phyTabIf[IDX_MAX];

    /* TX preamble number */
    uint8_t     phyTabTxPreamNumBLE[IDX_MAX];

    /* PHY data maping table */
    uint8_t     phyTabBw[IDX_MAX];

    /* Group delay (unit: us) */
    uint8_t     phyTabTxDelay[IDX_MAX];
    uint8_t     phyTabTxTail[IDX_MAX];

    /* Gain table */
    uint8_t     phyTabLNAGain2G[PHY_TAB_LNA_GAIN_LEN];
    uint8_t     phyTabTIAGain[PHY_TAB_TIA_GAIN_LEN];
    uint8_t     phyTabVGAGain[PHY_TAB_VGA_GAIN_LEN];

    /* RSSI offset */
    uint8_t     phyTabRssiOffset[IDX_MAX];

    /* BW index to parameter index */
    uint8_t     phyTabBwMap[BW_MAX];
} sHAL_HW_TAB_IND, *pHAL_HW_TAB_IND;

typedef struct HAL_PRTCL_CFG
{
    /* Common scheduler */
    uint32_t    commDefInterval;                    //
    uint8_t     scheGuardBase;                      // LL_PARA_SCHE_GUARD_VAL
    uint8_t     schePickGuardBase;                  // LL_PARA_SCHE_PICK_GUARD_TIME
    uint8_t     scheScanMinTimeBase;                // LL_PARA_SCHE_SCAN_MINIMUM_TIME
    uint8_t     scheScanWinErlyAbrtBase;            // LL_PARA_SCAN_WINDOW_EARLY_ABORT
    uint8_t     scheScanRxTOProtectBase;            // LL_PARA_SCAN_RX_TIMEOUT_PROTECT
    uint8_t     scheScanConnMDErlyAbrt1Base;        // LL_PARA_SCHE_CONN_MD_EARLY_ABORT
    uint8_t     scheScanConnMDErlyAbrt2Base;        // LL_PARA_SCHE_CONN_MD_EARLY_ABORT
    uint16_t    scheMaxWeightVal;                   // LL_PARA_SCHE_MAX_WEIGHT_VAL
    uint16_t    scheScanFixedWeightVal;             // LL_PARA_SCHE_SCAN_FIXED_WEIGHT
    uint16_t    scheInitFixedWeightVal;             // LL_PARA_SCHE_INIT_FIXED_WEIGHT

    /* BLE RX Proc Delay */
    uint8_t     bleNextT2ROverhead;                 //
    uint8_t     ble1mRxProcDelay;                   //
    uint8_t     ble2mRxProcDelay;                   //
    uint8_t     bleS2RxProcDelay;                   //
    uint8_t     bleS8RxProcDelay;                   //

    uint8_t     bleUnknownProcDelay;                   //BLE_CODED_UNKNOWN_RX_PROC_DLEAY
    uint8_t     bleMasSlaCodedIfsS2OddLen;
    uint8_t     bleMasSlaCodedIfsS8OddLen;
    uint8_t     bleSlaCodedIfsS8OddLen;

    uint8_t     bleAsymCodedS2SpeedUpLenLevel;
    uint8_t     bleAsymCodedS8SpeedUpLenLevel;

    /* BLE scheduler prepare offset */
    uint16_t    bleAdvPrepareOffset;                   //
    uint16_t    bleScanPrepareOffset;                  //
    uint16_t    bleInitPrepareOffset;                  //
    uint16_t    bleConnMasPrepareOffset;               //
    uint16_t    bleConnSlaPrepareOffset;               //

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
    sHAL_REG_PATCH_MEM  mem[14];
} sHAL_REG_PATCH, *pHAL_REG_PATCH;

/* HAL memory indicater */
typedef struct HAL_PARA_IND
{
    /* HAL FW configuration */
    uint32_t pFwCfg;

    /* HAL HW configuration */
    uint32_t pHwCfgBLE;
    uint32_t pHwCfgZigbee;

    /* Default register setting */
    uint32_t pMdmDefaultCfg;
    uint32_t pRfDefaultCfg2G;
    uint32_t pPmuDefaultCfg2G;

    /* BMU memory size */
    uint32_t pBmuMemSize;

    /* HAL HW parameter indicater */
    uint32_t pHwPara;

    /* HAL HW table indicater */
    uint32_t pHwTab;

    uint32_t pPrtclCfg;

    /* HAL register patch */
    uint32_t pRegPatch;

} sHAL_PARA_IND, *pHAL_PARA_IND;

#pragma pack(pop)
#endif

/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
#if ((RF_MCU_CHIP_MODEL == RF_MCU_CHIP_569M0) && (RF_MCU_CHIP_VER == RF_MCU_CHIP_VER_A))
void RfMcu_PhyExtMemInit(void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* __RT569MXA_INIT_H__ */


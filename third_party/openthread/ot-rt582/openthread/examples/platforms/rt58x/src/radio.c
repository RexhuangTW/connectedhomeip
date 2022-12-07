/**
 * @file radio.c
 * @author Rex Huang (rex.huang@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2022-04-07
 *
 * @copyright Copyright (c) 2022
 *
 */
//=============================================================================
//                Include
//=============================================================================
#include "OpenThreadConfig.h"

#include "openthread-system.h"
#include <assert.h>
#include <openthread/config.h>
#include <openthread/link.h>
#include <openthread/platform/alarm-micro.h>
#include <openthread/platform/alarm-milli.h>
#include <openthread/platform/diag.h>
#include <openthread/platform/radio.h>
#include <openthread/platform/time.h>

#include "common/logging.hpp"
#include "utils/code_utils.h"
#include "utils/mac_frame.h"

#include "utils/soft_source_match_table.h"

#include "project_config.h"

/* Rafael RFB */
#include "rfb.h"
#include "rfb_comm_15p4Mac.h"

#include "mac_frame_gen.h"

#include "task_hci.h"
#include "util_list.h"
#include "util_log.h"

#include "sys_arch.h"

#include "mem_mgmt.h"

//=============================================================================
//                Private Definitions of const value
//=============================================================================
#define CCA_THRESHOLD_UNINIT (127)
#define CCA_THRESHOLD_DEFAULT (75) //(75) // dBm  - default for 2.4GHz 802.15.4
#define RAFAEL_RECEIVE_SENSITIVITY (100)

#define RX_CONTROL_FIELD_LENGTH (7)
#define RX_STATUS_LENGTH (5)
#define PHR_LENGTH (1)
#define CRC16_LENGTH (2)
#define RX_HEADER_LENGTH (RX_CONTROL_FIELD_LENGTH + PHR_LENGTH)
#define RX_APPEND_LENGTH (RX_STATUS_LENGTH + CRC16_LENGTH)
#define RX_LENGTH (MAX_RF_LEN + RX_HEADER_LENGTH + RX_APPEND_LENGTH)
#define FCF_LENGTH (2)

#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#define PHY_PIB_TURNAROUND_TIMER 192
#define PHY_PIB_CCA_DETECTED_TIME 128 // 8 symbols
#define PHY_PIB_CCA_DETECT_MODE ENERGY_DETECTION
#define PHY_PIB_CCA_THRESHOLD CCA_THRESHOLD_DEFAULT
#define MAC_PIB_UNIT_BACKOFF_PERIOD 320
#define MAC_PIB_MAC_ACK_WAIT_DURATION 544 // non-beacon mode; 864 for beacon mode
#define MAC_PIB_MAC_MAX_BE 5
#define MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME 16416
#define MAC_PIB_MAC_MAX_FRAME_RETRIES 4
#define MAC_PIB_MAC_MAX_CSMACA_BACKOFFS 10
#define MAC_PIB_MAC_MIN_BE 2
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define PHY_PIB_TURNAROUND_TIMER (692 / 4)  // 192
#define PHY_PIB_CCA_DETECTED_TIME (692 / 4) // 128 // 8 symbols
#define PHY_PIB_CCA_DETECT_MODE ENERGY_DETECTION
#define PHY_PIB_CCA_THRESHOLD CCA_THRESHOLD_DEFAULT
#define MAC_PIB_UNIT_BACKOFF_PERIOD (320 / 4)
#define MAC_PIB_MAC_ACK_WAIT_DURATION (2000 / 4) // 544 // non-beacon mode; 864 for beacon mode
#define MAC_PIB_MAC_MAX_BE 3
#define MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME 16416
#define MAC_PIB_MAC_MAX_FRAME_RETRIES 4
#define MAC_PIB_MAC_MAX_CSMACA_BACKOFFS 3
#define MAC_PIB_MAC_MIN_BE 1
#endif

#define MAC_RX_BUFFERS 20
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
#define FREQ (2405)
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
#define FREQ (915000)
#endif

static void radioSendMessage(otInstance * aInstance);
static void _mac_rx_buffer_free(uint8_t * pbuf);
static bool _mac_rx_buffer_chk_acl(uint8_t * pbuf);
static bool hasFramePending(const otRadioFrame * aFrame);
//=============================================================================
//                Private ENUM
//=============================================================================
enum
{
    kMinChannel = 11,
    kMaxChannel = 26,
};

enum
{
    IEEE802154_MIN_LENGTH      = 5,
    IEEE802154_MAX_LENGTH      = 2047,
    IEEE802154_ACK_LENGTH      = 5,
    IEEE802154_FRAME_TYPE_MASK = 0x7,
    IEEE802154_FRAME_TYPE_ACK  = 0x2,
    IEEE802154_FRAME_PENDING   = 1 << 4,
    IEEE802154_ACK_REQUEST     = 1 << 5,
    IEEE802154_DSN_OFFSET      = 2,
};
//=============================================================================
//                Private Struct
//=============================================================================
typedef struct RadioMessage
{
    uint8_t mChannel;
    uint8_t mPsdu[OT_RADIO_FRAME_MAX_SIZE];
    uint16_t mLength;
    uint8_t mDsn;
} rf_tx_msg_t;

typedef struct
{
    uint8_t data[268];
    bool free;
    bool acl;
} rf_rx_buffer_t;

typedef struct
{
    uint16_t msg_tag; /**< message tag. */
    uint16_t msg_len; /**< message length. */
    uint8_t * p_msg;  /**< message payload. */
} rf_fw_rx_ctrl_msg_t;

//=============================================================================
//                Private Global Variables
//=============================================================================
static rfb_interrupt_event_t sRFBInterruptEvt;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
static rfb_zb_ctrl_t * spRFBCtrl;
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
static rfb_wisun_ctrl_t * spRFBCtrl;
#endif

static bool sIsSrcMatchEnabled = false;
static int8_t sCcaThresholdDbm = CCA_THRESHOLD_DEFAULT;
static otExtAddress sExtAddress;

static uint8_t sIEEE_EUI64Addr[8] = { 0xAA, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

static uint16_t sTurnaroundTime = PHY_PIB_TURNAROUND_TIMER;
static uint8_t sCCAMode         = PHY_PIB_CCA_DETECT_MODE;
static uint8_t sCCAThreshold    = PHY_PIB_CCA_THRESHOLD;
static uint16_t sCCADuration    = PHY_PIB_CCA_DETECTED_TIME;

static uint16_t sMacAckWaitTime = MAC_PIB_MAC_ACK_WAIT_DURATION;
static uint8_t sMacFrameRetris  = MAC_PIB_MAC_MAX_FRAME_RETRIES;

static uint8_t sPhyTxPower    = TX_POWER_20dBm;
static uint8_t sPhyDataRate   = FSK_300K;
static uint8_t sPhyModulation = MOD_1;

static bool sDisable = true;

static uint8_t sPromiscuous    = false;
static uint16_t sShortAddress  = 0xFFFF;
static uint32_t sExtendAddr_0  = 0x01020304;
static uint32_t sExtendAddr_1  = 0x05060709;
static uint16_t sPANID         = 0xFFFF;
static uint8_t sCoordinator    = 0;
static uint8_t sCurrentChannel = kMinChannel;

static bool sTxWait = false;
static bool sTxDone = false;
static bool sIsAck  = false;
static otError sTransmitError;
static otError sReceiveError = OT_ERROR_NONE;
static otRadioState sState   = OT_RADIO_STATE_DISABLED;

static rf_tx_msg_t sTransmitMessage;
static otRadioFrame sTransmitFrame;
static otRadioFrame sAckFrame;
static struct RadioMessage sAckMessage;

static rf_rx_buffer_t sMacRxBuffer[MAC_RX_BUFFERS];
static otRadioFrame sReceiveFrame[MAC_RX_BUFFERS];
static uint32_t sMacFrameCounter;
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

static uint8_t sKeyId;
static otMacKeyMaterial sPrevKey;
static otMacKeyMaterial sCurrKey;
static otMacKeyMaterial sNextKey;
static otRadioKeyType sKeyType;
#endif

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
static uint32_t sCslSampleTime;
static uint32_t sCslPeriod;
#endif

static uint32_t otRFBInit = 0;
extern sys_queue_t g_rx_common_queue_handle;

static otInstance * maInstance;
//=============================================================================
//                Functions
//=============================================================================
static void ReverseExtAddress(otExtAddress * aReversed, const otExtAddress * aOrigin)
{
    for (size_t i = 0; i < sizeof(*aReversed); i++)
    {
        aReversed->m8[i] = aOrigin->m8[sizeof(*aOrigin) - 1 - i];
    }
}

bool platformRadioIsTransmitPending(void)
{
    return !sTxWait;
}

/* Radio Configuration */
/**
 * @brief                               Get the bus speed in bits/second between the host and radio chip
 *
 * @param aInstance                     A pointer to an OpenThread instance
 * @return uint32_t                     The bus speed in bits/second between the host and the radio chip.
 *                                      Return 0 when the MAC and above layer and Radio layer resides
 *                                      on the same chip.
 */
uint32_t otPlatRadioGetBusSpeed(otInstance * aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return 0;
}

/**
 * @brief                               Get the radio capabilities.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otRadioCaps                  The radio capability bit vector (see OT_RADIO_* definitions)
 */
otRadioCaps otPlatRadioGetCaps(otInstance * aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    /* clang-format off */
    otRadioCaps capabilities = (OT_RADIO_CAPS_ACK_TIMEOUT |
                               OT_RADIO_CAPS_TRANSMIT_RETRIES |
//#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
//                                OT_RADIO_CAPS_TRANSMIT_SEC      |
//#endif
                                OT_RADIO_CAPS_CSMA_BACKOFF
                               );

    return capabilities;
}

/**
 * @brief                               Get the radio's CCA ED threshold in dBm measured at antenna connector
 *                                      per IEEE 802.15.4 - 2015 section 10.1.4
 * @param aInstance                     The OpenThread instance structure.
 * @param aThreshold                    The CCA ED threshold in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully retrieved the CCA ED threshold.
 *                                      OT_ERROR_INVALID_ARGS       aThreasold was NULL.
 *                                      OT_ERROR_NOT_IMPLEMENTED    CCA ED threshold configuration via dBm is
 *                                                                  not implemented.
 */
otError otPlatRadioGetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t *aThreshold)
{

    return OT_ERROR_NOT_IMPLEMENTED;
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;
    otEXPECT_ACTION(aThreshold != NULL, error = OT_ERROR_INVALID_ARGS);

    *aThreshold = -(sCcaThresholdDbm);

exit:
    return error;
}

/**
 * @brief                               Get the external FEM's Rx LNA gain in dBm.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aGain                         The external FEM's Rx LNA gain in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully retrieved the external FEM's LNA gain.
 *                                      OT_ERROR_INVALID_ARGS       aGain was NULL.
 *                                      OT_ERROR_NOT_IMPLEMENTED    External FEM's LNA setting is
 *                                                                  not implemented.
 */
otError otPlatRadioGetFemLnaGain(otInstance *aInstance, int8_t *aGain)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief                               Get the factory-assigned IEEE EUI-64 for this interface.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aIeeeEui64                    A pointer to the factory-assigned IEEE EUI-64.
 */
void otPlatRadioGetIeeeEui64(otInstance *aInstance, uint8_t *aIeeeEui64)
{
    OT_UNUSED_VARIABLE(aInstance);

    memcpy(aIeeeEui64, sIEEE_EUI64Addr, 8);
}

/**
 * @brief                               Get the current estimated time(in microseconds) of the radio chip.
 *
 *                                      This microsecond timer must be a free-running timer. The timer must
 *                                      continue to advance with microsecond presision even when the radio is
 *                                      in the sleep state.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return uint64_t                     The current time in microseconds. UINT64_MAX when platform does not
 *                                      support or radio time is not ready.
 */
uint64_t otPlatRadioGetNow(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    //return  otPlatAlarmMicroGetNow();
    //return  rfb_port_rtc_time_read();
    return 0;
}

/**
 * @brief                               Get the status of promiscuous mode.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return true                         Promiscuous mode is enabled.
 * @return false                        Promiscuous mode is disable.
 */
bool otPlatRadioGetPromiscuous(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return sPromiscuous;
}

/**
 * @brief                               Get the radio receive sensitivity value.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return int8_t                       The radio receive sensitivity value in dBm.
 */
int8_t otPlatRadioGetReceiveSensitivity(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return -(RAFAEL_RECEIVE_SENSITIVITY);
}

/**
 * @brief                               Get the radio's transmit power in dBm.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aPower                        The transmit power in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully retrieved the transmit power.
 *                                      OT_ERROR_INVALID_ARGS       aPower was NULL.
 *                                      OT_ERROR_NOT_IMPLEMENTED    Transmit power configuration via
 *                                                                  dBm is not implemented.
 */
otError otPlatRadioGetTransmitPower(otInstance *aInstance, int8_t *aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    otError error = OT_ERROR_NONE;

    error = OT_ERROR_NOT_IMPLEMENTED;

    otEXPECT_ACTION(aPower != NULL, error = OT_ERROR_INVALID_ARGS);

exit:
    return error;
}

/**
 * @brief                               Set the radio's CCA ED threshold in dBm measured at antenna
 *                                      connector per IEEE 802.15.4-2015 section 10.1.4.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aThreshold                    The CCA ED threshold in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully retrieved the CCA ED threshold.
 *                                      OT_ERROR_INVALID_ARGS       Given threshold is out of range.
 *                                      OT_ERROR_NOT_IMPLEMENTED    CCA ED threshold configuration via
 *                                                                  dBm is not implemented.
 */
otError otPlatRadioSetCcaEnergyDetectThreshold(otInstance *aInstance, int8_t aThreshold)
{
    //info("OT %s\n", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NOT_IMPLEMENTED;
    sCCAThreshold = aThreshold;

    if(otRFBInit)
    spRFBCtrl->phy_pib_set(sTurnaroundTime,
                        sCCAMode,
                        sCCAThreshold,
                        sCCADuration);

    return OT_ERROR_NONE;
}

/**
 * @brief                               Set the Extended Address for address filtering.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aAddress                      A pointer to the IEEE 802.15.4 Extended Address stored in
 *                                      little-endian byte order.
 */
void otPlatRadioSetExtendedAddress(otInstance *aInstance, const otExtAddress *aAddress)
{
    //info("OT %s\n", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    ReverseExtAddress(&sExtAddress, aAddress);


    sExtendAddr_0 = (aAddress->m8[0] | (aAddress->m8[1] << 8) | (aAddress->m8[2] << 16) | (aAddress->m8[3] << 24));
    sExtendAddr_1 = (aAddress->m8[4] | (aAddress->m8[5] << 8) | (aAddress->m8[6] << 16) | (aAddress->m8[7] << 24));

    if(otRFBInit)
    spRFBCtrl->address_filter_set(sPromiscuous,
                                  sShortAddress,
                                  sExtendAddr_0,
                                  sExtendAddr_1,
                                  sPANID,
                                  sCoordinator);                                
}

/**
 * @brief                               Set the external FEM's Rx LNA gain in dBm.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aGain                         The external FEM's RX LNA gain in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully set the external FEM's RX LNA gain.
 *                                      OT_ERROR_INVALID_ARGS       Given gain is out of range.
 *                                      OT_ERROR_NOT_IMPLEMENTED    External FEM's Rx LNA gain
 *                                                                  setting is not implemented.
 */
otError otPlatRadioSetFemLnaGain(otInstance *aInstance, int8_t aGain)
{
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    return OT_ERROR_NOT_IMPLEMENTED;
}

/**
 * @brief                               Set the PAN ID for address filitering.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aPanId                        The IEEE 802.15.4 PAN ID.
 */
void otPlatRadioSetPanId(otInstance *aInstance, uint16_t aPanId)
{
    //info("OT %s\n", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    sPANID = aPanId;

    utilsSoftSrcMatchSetPanId(aPanId);
    if(otRFBInit)
    spRFBCtrl->address_filter_set(sPromiscuous,
                                  sShortAddress,
                                  sExtendAddr_0,
                                  sExtendAddr_1,
                                  sPANID,
                                  sCoordinator);
}

/**
 * @brief                               Enable or disable promiscuous mode.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aEnable                       TURE to enable or FALSE to disable promiscuous mode.
 */
void otPlatRadioSetPromiscuous(otInstance *aInstance, bool aEnable)
{
    //info("OT %s\n", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    sPromiscuous = aEnable;
    if(otRFBInit)
    spRFBCtrl->address_filter_set(sPromiscuous,
                                  sShortAddress,
                                  sExtendAddr_0,
                                  sExtendAddr_1,
                                  sPANID,
                                  sCoordinator);

}

/**
 * @brief                               Set the Short Address for address filitering.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aAddress                      The IEEE 802.15.4 Short Address.
 */
void otPlatRadioSetShortAddress(otInstance *aInstance, uint16_t aAddress)
{
    //info("OT %s\n", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    sShortAddress = aAddress;
    if(otRFBInit)
    spRFBCtrl->address_filter_set(sPromiscuous,
                                  sShortAddress,
                                  sExtendAddr_0,
                                  sExtendAddr_1,
                                  sPANID,
                                  sCoordinator);
}

/**
 * @brief                               Set the radio's transmit power in dBm.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aPower                        The transimit power in dBm.
 * @return otError                      OT_ERROR_NONE               Successfully set the transmit power.
 *                                      OT_ERROR_INVALID_ARGS       aPower is out of range.
 *                                      OT_ERROR_NOT_IMPLEMENTED    Transmit power configuration via
 *                                                                  dBm is not implemented.
 */
otError otPlatRadioSetTransmitPower(otInstance *aInstance, int8_t aPower)
{
    OT_UNUSED_VARIABLE(aInstance);

    return OT_ERROR_NOT_IMPLEMENTED;
}

/* Radio Operation */

/**
 * @brief                               Disable the radio.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to Disable.
 *                                      OT_ERROR_INVALID_STATE      The radio was not in sleep state.
 */
otError otPlatRadioDisable(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    sDisable = true;
    sState = OT_RADIO_STATE_SLEEP;

    return OT_ERROR_NONE;
}

/**
 * @brief                               Enable the radio.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to enable.
 *                                      OT_ERROR_INVALID_FAILED     The radio could not be enabled.
 */
otError otPlatRadioEnable(otInstance *aInstance)
{

    otError error;
    OT_UNUSED_VARIABLE(aInstance);

    if (sDisable)
    {
        sDisable = false;
        sState = OT_RADIO_STATE_SLEEP;
        error =  OT_ERROR_NONE;
    }
    else
    {
        error = OT_ERROR_NONE;
    }
    return error;
}

/**
 * @brief                               Enable/Disable source address match feature.
 *
 *                                      The source address match feature controls how the radio layer descides
 *                                      the "frame pending" bit for acks sent in response to data request commands
 *                                      from children.
 *
 *                                      If disable, the radio layer must set the "frame panding" on all acks to
 *                                      data request commands.
 *
 *                                      If enable, the radio layer uses the source address match table to determine
 *                                      whether to set or clear the "frame pending" bit in an ack to data request command.
 *
 *                                      The source address match table provide the list of children for which there is a
 *                                      pending frame. Either a short address or an extended/long address can be added
 *                                      to the source address match table.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aEnable                       Enable/disable source address match feature.
 */
void otPlatRadioEnableSrcMatch(otInstance *aInstance, bool aEnable)
{
    //info("OT %s\n", __func__);
    OT_UNUSED_VARIABLE(aInstance);
    // set Frame Pending bit for all outgoing ACKs if aEnable is false
    sIsSrcMatchEnabled = aEnable;
    if(otRFBInit)
    spRFBCtrl->src_addr_match_ctrl(sIsSrcMatchEnabled);
}

/**
 * @brief                               Biging the energy scan sequence on the radio.
 *                                      The function is used when radio provides OT_RADIO_CAPS_ENERGY_SCAN capability.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aScanChannel                  The channel to perform the energy scan on.
 * @param aScanDuration                 The duration, in milliseconds, for the channel to be scanned.
 * @return otError                      OT_ERROR_NONE               Successfully started scanning the channel.
 *                                      OT_ERROR_NOT_IMPLEMENTED    The radio doesn't support energy scanning.
 */
otError otPlatRadioEnergyScan(otInstance *aInstance, uint8_t aScanChannel, uint16_t aScanDuration)
{
    //info("OT %s\n", __func__);
    //OT_UNUSED_VARIABLE(aInstance);
    maInstance = aInstance;
    return OT_ERROR_NOT_IMPLEMENTED;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    uint32_t ChannelFrequency = FREQ + 5 * (aScanChannel - 11);
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    uint32_t ChannelFrequency = FREQ + 200 * (aScanChannel - 1);
#endif
    int8_t rssi_value;

    spRFBCtrl->frequency_set(ChannelFrequency);

    rssi_value = spRFBCtrl->rssi_read(RFB_MODEM_ZIGBEE);
    if(otRFBInit)
    otPlatRadioEnergyScanDone(aInstance, -rssi_value);

    sCurrentChannel        = aScanChannel;

    return OT_ERROR_NONE;
}

/**
 * @brief                               Get the most recent RSSI measurement.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return int8_t                       The RSSI in dBm when it is valid. 127 when RSSI is invalid.
 */
int8_t otPlatRadioGetRssi(otInstance *aInstance)
{
    //info("OT %s\n", __func__);
    otError  error;
    int8_t   rssi = OT_RADIO_RSSI_INVALID;

    OT_UNUSED_VARIABLE(aInstance);
    if(otRFBInit)
    rssi = spRFBCtrl->rssi_read(RFB_MODEM_ZIGBEE);

    return -rssi;
}


/**
 * @brief                               Get the radio transmit frame buffer.
 *
 *                                      OpenThread forms the IEEE 802.15.4 frame in this buffer then calls
 *                                      otPlatRadioTransmit() to request transmission.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otRadioFrame*                A pointer ti the transimit frame buffer.
 */
otRadioFrame *otPlatRadioGetTransmitBuffer(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return &sTransmitFrame;
}

/**
 * @brief                               Check whether radio is enabled or not.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return true                         If the radio is enabled.
 * @return false                        If the radio is not enabled.
 */
bool otPlatRadioIsEnabled(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    return (!sDisable);
}

/**
 * @brief                               Transition the radio from Sleep to Receive(turn on the radio)
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aChannel                      The channel to use for receiving.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to Receive.
 *                                      OT_ERROR_INVALID_STATE      The radio was disabled or transmiting.
 */
otError otPlatRadioReceive(otInstance *aInstance, uint8_t aChannel)
{
    //info("OT %s\n", __func__);
    OT_UNUSED_VARIABLE(aInstance);

    assert(aInstance != NULL);

    otError error = OT_ERROR_INVALID_STATE;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    uint32_t ChannelFrequency = FREQ + 5 * (aChannel - 11);
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    uint32_t ChannelFrequency = FREQ + 200 * (aChannel - 1) ;
#endif

    if (sState != OT_RADIO_STATE_DISABLED)
    {
        error                  = OT_ERROR_NONE;
        sState                 = OT_RADIO_STATE_RECEIVE;
        //sReceiveFrame.mChannel = aChannel;
        sCurrentChannel        = aChannel;

        /* Enable RX and leave SLEEP */
        if(otRFBInit)
        spRFBCtrl->frequency_set(ChannelFrequency);
        if(otRFBInit)
        spRFBCtrl->auto_state_set(true);
    }

    return error;
}


/**
 * @brief                               Transition the radio from Receive to Sleep (turn off the radio)
 *
 * @param aInstance                     The OpenThread instance structure.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to Sleep.
 *                                      OT_ERROR_BUSY               The radio was transmiting.
 *                                      OT_ERROR_INVALID_STATE      The radio was disabled.
 */
otError otPlatRadioSleep(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    otError error = OT_ERROR_NONE;

    /* Disable RX, and enter sleep*/
    //spRFBCtrl->auto_state_set(false);
    sState = OT_RADIO_STATE_SLEEP;
exit:
    return error;
}

/**
 * @brief                               Begin the transmit sequence on the radio.
 *
 *                                      The caller must from the IEEE 802.15.4 frame in the buffer provided by
 *                                      otPlatRadioTransmitBuffer() before requesting transmission. The channel
 *                                      and transmit power are also include in otRadioFrame structure.
 *
 *                                      The transmit sequence consists of:
 *                                          1. Transitioning the radio to Transmit from on of following state:
 *                                              * Receive if Rx is on when the device idle or OT_RADIO_CAPS_SLEEP_TO_TX
 *                                                is not supported.
 *                                              * Sleep if Rx is off when the device is idle or OT_RADIO_CAPS_SLEEP_TO_TX
 *                                                is supported.
 *                                          2. Transmits the psdu on the given channel and at the given transmit power.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aFrame                        A pointer to the frame to be transmitted.
 * @return otError                      OT_ERROR_NONE               Successfully transitioned to Transmit.
 *                                      OT_ERROR_INVALID_STATE      The radio was not in Receive state.
 */
otError otPlatRadioTransmit(otInstance *aInstance, otRadioFrame *aFrame)
{
    //info("OT %s\n", __func__);
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aFrame);

    assert(aInstance != NULL);
    assert(aFrame != NULL);
    
    otError error = OT_ERROR_INVALID_STATE;
    if(otRFBInit==0)
        return error;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    uint32_t ChannelFrequency = FREQ + 5 * (sCurrentChannel - 11);
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    uint32_t ChannelFrequency = FREQ + 200 * (sCurrentChannel - 1) ;
#endif
    //if (sState == OT_RADIO_STATE_RECEIVE)
    {
        //while (!platformRadioIsTransmitPending());
        if (platformRadioIsTransmitPending())
        {
            //gpio_pin_write(20, 0);
            error           = OT_ERROR_NONE;
            sState          = OT_RADIO_STATE_TRANSMIT;
            if(sCurrentChannel != aFrame->mChannel)
            {
                sCurrentChannel = aFrame->mChannel;
                spRFBCtrl->frequency_set(ChannelFrequency);
            }
            radioSendMessage(aInstance);
        }

    }
    return error;
}
#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
otError otPlatRadioEnableCsl(otInstance         *aInstance,
                             uint32_t            aCslPeriod,
                             otShortAddress      aShortAddr,
                             const otExtAddress *aExtAddr)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aShortAddr);
    OT_UNUSED_VARIABLE(aExtAddr);

    otError error = OT_ERROR_NONE;

    sCslPeriod = aCslPeriod;
    if (sCslPeriod > 0)
    {
        spRFBCtrl->csl_receiver_ctrl(1, sCslPeriod);
    }
    return error;
}

void otPlatRadioUpdateCslSampleTime(otInstance *aInstance, uint32_t aCslSampleTime)
{
    OT_UNUSED_VARIABLE(aInstance);

    sCslSampleTime = aCslSampleTime;
    spRFBCtrl->csl_sample_time_update(sCslSampleTime);
}



#endif // OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
uint8_t otPlatRadioGetCslAccuracy(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);

    //return spRFBCtrl->csl_accuracy_get();
    return 0;
}

uint8_t otPlatRadioGetCslUncertainty(otInstance *aInstance)
{
    OT_UNUSED_VARIABLE(aInstance);
    //return spRFBCtrl->csl_uncertainty_get();
    return 0;
}
#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
void otPlatRadioSetMacKey(otInstance             *aInstance,
                          uint8_t                 aKeyIdMode,
                          uint8_t                 aKeyId,
                          const otMacKeyMaterial *aPrevKey,
                          const otMacKeyMaterial *aCurrKey,
                          const otMacKeyMaterial *aNextKey,
                          otRadioKeyType          aKeyType)
{
    OT_UNUSED_VARIABLE(aInstance);
    OT_UNUSED_VARIABLE(aKeyIdMode);

    otEXPECT(aPrevKey != NULL && aCurrKey != NULL && aNextKey != NULL);

    sKeyId   = aKeyId;
    sKeyType = aKeyType;
    memcpy(&sPrevKey, aPrevKey, sizeof(otMacKeyMaterial));
    memcpy(&sCurrKey, aCurrKey, sizeof(otMacKeyMaterial));
    memcpy(&sNextKey, aNextKey, sizeof(otMacKeyMaterial));
    //spRFBCtrl->key_set(sCurrKey.mKeyMaterial.mKey.m8);
exit:
    return;
}

/**
 * @brief                               The method sets the current MAC frame counter value.
 *
 * @param aInstance                     The OpenThread instance structure.
 * @param aMacFrameCounter              The MAC frame counter value.
 */
void otPlatRadioSetMacFrameCounter(otInstance *aInstance, uint32_t aMacFrameCounter)
{
    OT_UNUSED_VARIABLE(aInstance);

    sMacFrameCounter = aMacFrameCounter;
}
#endif

/**
 * @brief                               The method get the cca value.
 */
otError otPlatRadioGetCca(otInstance *aInstance, int8_t *aThreshold, uint16_t *turnaroundtime, uint16_t *duration)
{
    *aThreshold = -(sCCAThreshold);
    *turnaroundtime = sTurnaroundTime;
    *duration = sCCADuration;
    return OT_ERROR_NONE;
}

/**
 * @brief                               The method sets the cca value.
 * @return otError                                          OT_ERROR_NONE               Successfully set the transmit power.
 *                                      OT_ERROR_INVALID_ARGS       cca value is error.
 */
otError otPlatRadioSetCca(otInstance *aInstance, int8_t aThreshold, uint16_t turnaroundtime, uint16_t duration)
{
    //info("OT %s\n", __func__);
    if (aThreshold <= 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    if (turnaroundtime <= 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    if (duration <= 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    sCCAThreshold = aThreshold;
    sTurnaroundTime = turnaroundtime;
    sCCADuration = duration;

    spRFBCtrl->phy_pib_set(sTurnaroundTime,
                           sCCAMode,
                           sCCAThreshold,
                           sCCADuration);
    return OT_ERROR_NONE;
}

/**
 * @brief                              The method gets the mac config value.
 */
otError otPlatRadioGetMacConfig(otInstance *aInstance, uint16_t *ack_wait_time, uint8_t *mac_try)
{
    *ack_wait_time = sMacAckWaitTime;
    *mac_try = sMacFrameRetris;

    return OT_ERROR_NONE;
}

/**
 * @brief                               The method sets the mac config value.
 * @return otError                                          OT_ERROR_NONE               Successfully set the transmit power.
 *                                      OT_ERROR_INVALID_ARGS       mac value is error.
 */
otError otPlatRadioSetMacConfig(otInstance *aInstance, uint16_t ack_wait_time, uint8_t mac_try)
{
    //info("OT %s\n", __func__);
    if (ack_wait_time <= 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    if (mac_try <= 0)
    {
        return OT_ERROR_INVALID_ARGS;
    }

    sMacAckWaitTime = ack_wait_time;
    sMacFrameRetris = mac_try;

    /* MAC PIB */
    spRFBCtrl->mac_pib_set(MAC_PIB_UNIT_BACKOFF_PERIOD,
                           sMacAckWaitTime,
                           MAC_PIB_MAC_MAX_BE,
                           MAC_PIB_MAC_MAX_CSMACA_BACKOFFS,
                           MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME,
                           sMacFrameRetris,
                           MAC_PIB_MAC_MIN_BE);
    return OT_ERROR_NONE;
}

/**
 * @brief                              The method gets the phy config value.
 * @param TX power [0: TX_POWER_20dBm , 1:TX_POWER_14dBm, 2:TX_POWER_0dBm]
 * @param Modulation index(fdev) [MOD_0P5 = 0, MOD_1 = 1] (FSK only)
 * @param Data rate
                  FSK [FSK_2M = 0, FSK_1M = 1, FSK_500K = 2, FSK_200K = 3, FSK_100K = 4,
                    FSK_50K = 5, FSK_300K = 6, FSK_150K = 7, FSK_75K = 8]
 */
otError otPlatRadioGetPhyConfig(otInstance *aInstance, uint8_t *tx_power, uint8_t *modulation, uint8_t *data_rate)
{
    *tx_power = sPhyTxPower;
    *modulation = sPhyModulation;
    *data_rate = sPhyDataRate;
    return OT_ERROR_NONE;
}

/**
 * @brief                              The method sets the phy config value.
 * @param TX power [0: TX_POWER_20dBm , 1:TX_POWER_14dBm, 2:TX_POWER_0dBm]
 * @param Modulation index(fdev) [MOD_0P5 = 0, MOD_1 = 1] (FSK only)
 * @param Data rate
                  FSK [FSK_2M = 0, FSK_1M = 1, FSK_500K = 2, FSK_200K = 3, FSK_100K = 4,
                    FSK_50K = 5, FSK_300K = 6, FSK_150K = 7, FSK_75K = 8]
 * @return otError                                          OT_ERROR_NONE               Successfully set the transmit power.
 *                                      OT_ERROR_INVALID_ARGS       phy config value is error.
 */
otError otPlatRadioSetPhyConfig(otInstance *aInstance, uint8_t tx_power, uint8_t modulation, uint8_t data_rate)
{
    //info("OT %s\n", __func__);
    if (tx_power < TX_POWER_20dBm || tx_power > TX_POWER_0dBm)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    if (modulation < MOD_0P5 || modulation > MOD_1)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    if (data_rate < FSK_200K || data_rate > FSK_150K)
    {
        return OT_ERROR_INVALID_ARGS;
    }
    sPhyTxPower = tx_power;
    sPhyModulation = modulation;
    sPhyDataRate = data_rate;
#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    /*Set RF State to Idle*/
    spRFBCtrl->idle_set();
    /*tx init*/
    spRFBCtrl->tx_config_set(sPhyTxPower, sPhyDataRate, 8, sPhyModulation, FSK_CRC_16, WHITEN_DISABLE, GFSK);

    /*rx init*/
    spRFBCtrl->rx_config_set(sPhyDataRate, 8, sPhyModulation, FSK_CRC_16, WHITEN_DISABLE, 0, true, GFSK);
#endif

    return OT_ERROR_NONE;
}

static otError radioProcessTransmitSecurity(otRadioFrame *aFrame)
{
    otError error = OT_ERROR_NONE;
    #if 0
//#if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    otMacKeyMaterial *key = NULL;
    uint8_t           keyId;

    otEXPECT(otMacFrameIsSecurityEnabled(aFrame) && otMacFrameIsKeyIdMode1(aFrame) &&
             !aFrame->mInfo.mTxInfo.mIsSecurityProcessed);

    if (otMacFrameIsAck(aFrame))
    {
        keyId = otMacFrameGetKeyId(aFrame);

        otEXPECT_ACTION(keyId != 0, error = OT_ERROR_FAILED);

        if (keyId == sKeyId)
        {
            key = &sCurrKey;
        }
        else if (keyId == sKeyId - 1)
        {
            key = &sPrevKey;
        }
        else if (keyId == sKeyId + 1)
        {
            key = &sNextKey;
        }
        else
        {
            error = OT_ERROR_SECURITY;
            otEXPECT(false);
        }
    }
    else
    {
        key   = &sCurrKey;
        keyId = sKeyId;
    }

    aFrame->mInfo.mTxInfo.mAesKey = key;

    if (!aFrame->mInfo.mTxInfo.mIsHeaderUpdated)
    {
        otMacFrameSetKeyId(aFrame, keyId);
        otMacFrameSetFrameCounter(aFrame, sMacFrameCounter++);
    }
#else
    otEXPECT(!aFrame->mInfo.mTxInfo.mIsSecurityProcessed);
#endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2

    otMacFrameProcessTransmitAesCcm(aFrame, &sExtAddress);

    //sMacFrameCounter = otMacFrameGetFrameCounter(aFrame);

exit:
    return error;
}

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
static uint16_t getCslPhase(void)
{
    uint32_t curTime       = rfb_port_rtc_time_read();
    uint32_t cslPeriodInUs = sCslPeriod * OT_US_PER_TEN_SYMBOLS;
    uint32_t diff = ((sCslSampleTime % cslPeriodInUs) - (curTime % cslPeriodInUs) + cslPeriodInUs) % cslPeriodInUs;

    return (uint16_t)(diff / OT_US_PER_TEN_SYMBOLS);
}
#endif

static void radioSendMessage(otInstance *aInstance)
{
    //info("ot %s \n", __func__);
    uint8_t tx_control;
    //uint8_t temp[OT_RADIO_FRAME_MAX_SIZE + 4];
    uint32_t tx_ret = 0;

#if OPENTHREAD_CONFIG_MAC_CSL_RECEIVER_ENABLE
    if (sCslPeriod > 0 && !sTransmitFrame.mInfo.mTxInfo.mIsHeaderUpdated)
    {
        otMacFrameSetCslIe(&sTransmitFrame, (uint16_t)sCslPeriod, getCslPhase());
    }
#endif

    sTransmitMessage.mChannel = sTransmitFrame.mChannel;
    sTransmitMessage.mLength = sTransmitFrame.mLength;

    otEXPECT(radioProcessTransmitSecurity(&sTransmitFrame) == OT_ERROR_NONE);

    if ((sTransmitFrame.mPsdu[0] & IEEE802154_ACK_REQUEST))
    {
        tx_control = 0x03;
    }
    else
    {
        tx_control = 0x02;
    }
#if 0
    tx_control |= (1 << 2 );
    temp[0] = ((sMacFrameCounter >> 0) & 0xFF);
    temp[1] = ((sMacFrameCounter >> 8) & 0xFF);
    temp[2] = ((sMacFrameCounter >> 16) & 0xFF);
    temp[3] = ((sMacFrameCounter >> 24) & 0xFF);

    memcpy(temp + 4, sTransmitMessage.mPsdu, sTransmitMessage.mLength);

    if(sTransmitFrame.mInfo.mTxInfo.mTxDelay != 0)
        tx_control ^= (1 << 1);
    tx_ret = spRFBCtrl->data_send(temp , (sTransmitMessage.mLength + 4 - 2), tx_control, otMacFrameGetSequence(&sTransmitFrame));
#endif
    
    
    tx_ret = spRFBCtrl->data_send(sTransmitMessage.mPsdu, sTransmitMessage.mLength - 2, tx_control, otMacFrameGetSequence(&sTransmitFrame));
    otPlatRadioTxStarted(aInstance, &sTransmitFrame);

    if(tx_ret)
    {
        otPlatRadioTxDone(aInstance, &sTransmitFrame, NULL, OT_ERROR_NO_ACK);
        return;
    }
    // Wait for echo radio in virtual time mode.
    sTxWait = true;
    //otLogWarnPlat("Tx Control %X\n", tx_control);
    //otDumpWarnPlat("Tx Packet", sTransmitMessage.mPsdu, sTransmitMessage.mLength);
exit:
    return;
}


#if OPENTHREAD_CONFIG_MLE_LINK_METRICS_SUBJECT_ENABLE
otError otPlatRadioConfigureEnhAckProbing(otInstance          *aInstance,
        otLinkMetrics        aLinkMetrics,
        const otShortAddress aShortAddress,
        const otExtAddress *aExtAddress)
{
    OT_UNUSED_VARIABLE(aInstance);

    return otLinkMetricsConfigureEnhAckProbing(aShortAddress, aExtAddress, aLinkMetrics);
}
#endif

static bool hasFramePending(const otRadioFrame *aFrame)
{
    bool         rval = false;
    otMacAddress src;

    otEXPECT_ACTION(sIsSrcMatchEnabled, rval = true);
    otEXPECT(otMacFrameGetSrcAddr(aFrame, &src) == OT_ERROR_NONE);

    switch (src.mType)
    {
    case OT_MAC_ADDRESS_TYPE_SHORT:
        rval = utilsSoftSrcMatchShortFindEntry(src.mAddress.mShortAddress) >= 0;
        break;
    case OT_MAC_ADDRESS_TYPE_EXTENDED:
    {
        otExtAddress extAddr;

        ReverseExtAddress(&extAddr, &src.mAddress.mExtAddress);
        rval = utilsSoftSrcMatchExtFindEntry(&extAddr) >= 0;
        break;
    }
    default:
        break;
    }

exit:
    return rval;
}
void platformRadioProcess(otInstance *startThread)
{
    otError      error = OT_ERROR_NONE;
    otRadioFrame *pAckFrame = NULL;
    uint32_t Enh_Ack_Index = MAC_RX_BUFFERS;
    uint8_t id = MAC_RX_BUFFERS;

    maInstance = startThread;
    //if (sState == OT_RADIO_STATE_RECEIVE)
    {
        for (uint32_t i = 0; i < MAC_RX_BUFFERS; i++)
        {
            if (sReceiveFrame[i].mPsdu != NULL)
            {                
                if(_mac_rx_buffer_chk_acl(sReceiveFrame[i].mPsdu))
                {
                    rf_fw_rx_ctrl_msg_t rf_msg = {0};

                    rf_msg.msg_tag = 0x05;
                    rf_msg.msg_len = sReceiveFrame[i].mLength;
                   
                    rf_msg.p_msg = mem_malloc(sReceiveFrame[i].mLength);
                    if(rf_msg.p_msg != 0)
                    {                       
                        memcpy(rf_msg.p_msg, sReceiveFrame[i].mPsdu, sReceiveFrame[i].mLength);
                        while(sys_queue_send_with_timeout(&g_rx_common_queue_handle, &rf_msg, 20) != 0)
                        {
                            err("acl data send failed\n");
                        }

                    }
                    _mac_rx_buffer_free(sReceiveFrame[i].mPsdu);
                    sReceiveFrame[i].mPsdu = NULL;
                    continue;
                }
                else
                {
                    //if(otMacFrameIsVersion2015(&sReceiveFrame[i]))
                    //    otLogWarnPlat("Rx Ver 2015\n");
                    // Unable to simulate SFD, so use the rx done timestamp instead.
                    //sReceiveFrame[i].mInfo.mRxInfo.mTimestamp = sUsCounter;
                    sReceiveFrame[i].mInfo.mRxInfo.mAckedWithFramePending = false;
                    sReceiveFrame[i].mInfo.mRxInfo.mAckedWithSecEnhAck    = false;

                    otEXPECT_ACTION(otMacFrameDoesAddrMatch(&sReceiveFrame[i], sPANID, sShortAddress, &sExtAddress),
                                    error = OT_ERROR_ABORT);
                    if (otMacFrameIsAckRequested(&sTransmitFrame) &&
                            otMacFrameIsAck(&sReceiveFrame[i]) &&
                            otMacFrameIsVersion2015(&sReceiveFrame[i]))
                        //otMacFrameGetSequence(&sReceiveFrame[i]) == otMacFrameGetSequence(&sTransmitFrame))
                    {
                        //util_log_mem(UTIL_LOG_INFO, "Enh-Ack ", sReceiveFrame[i].mPsdu, sReceiveFrame[i].mLength, 0);
                        memset(&sAckFrame,0x0,sizeof(otRadioFrame));                    
                        sAckFrame.mInfo.mRxInfo.mLqi = sReceiveFrame[i].mInfo.mRxInfo.mLqi;
                        sAckFrame.mInfo.mRxInfo.mRssi = sReceiveFrame[i].mInfo.mRxInfo.mRssi;
                        sAckFrame.mInfo.mRxInfo.mTimestamp = sReceiveFrame[i].mInfo.mRxInfo.mTimestamp;
                        sAckFrame.mChannel = sReceiveFrame[i].mChannel;
                        sAckFrame.mLength = sReceiveFrame[i].mLength;
                        sAckFrame.mRadioType = sReceiveFrame[i].mRadioType;
                        memcpy(&sAckFrame.mPsdu, &sReceiveFrame[i].mPsdu, sAckFrame.mLength);
                        Enh_Ack_Index = i;
                        _mac_rx_buffer_free(sReceiveFrame[i].mPsdu);
                        sReceiveFrame[i].mPsdu = NULL;
                        break;
                    }
                    //util_log_mem(UTIL_LOG_INFO, "R", sReceiveFrame[i].mPsdu, sReceiveFrame[i].mLength, 0);
                    if (
    #if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
                        // Determine if frame pending should be set
                        ((otMacFrameIsVersion2015(&sReceiveFrame[i]) && otMacFrameIsCommand(&sReceiveFrame[i])) ||
                        /*otMacFrameIsData(&sReceiveFrame[i]) || */otMacFrameIsDataRequest(&sReceiveFrame[i]))
    #else
                        otMacFrameIsDataRequest(&sReceiveFrame[i])
    #endif
                        && hasFramePending(&sReceiveFrame[i]))
                    {
                        sReceiveFrame[i].mInfo.mRxInfo.mAckedWithFramePending = true;
                    }
    #if OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
                    if (otMacFrameIsVersion2015(&sReceiveFrame[i]) &&
                        otMacFrameIsSecurityEnabled(&sReceiveFrame[i]) &&
                        otMacFrameIsAckRequested(&sReceiveFrame[i]))
                    {
                        sReceiveFrame[i].mInfo.mRxInfo.mAckedWithSecEnhAck = true;
                        sReceiveFrame[i].mInfo.mRxInfo.mAckFrameCounter = ++sMacFrameCounter;
                    }
    #endif // OPENTHREAD_CONFIG_THREAD_VERSION >= OT_THREAD_VERSION_1_2
    exit:
                    if (error != OT_ERROR_ABORT)
                    {
                        otPlatRadioReceiveDone(maInstance, error == OT_ERROR_NONE ? &sReceiveFrame[i] : NULL, error);
                        //if(error == OT_ERROR_NONE)
                        //    otDumpWarnPlat("Rx Packet", sReceiveFrame[i].mPsdu, sReceiveFrame[i].mLength);
                    }
                    _mac_rx_buffer_free(sReceiveFrame[i].mPsdu);
                    sReceiveFrame[i].mPsdu = NULL;
                    //gpio_pin_write(21, 1);
                    break;
                }
            }
        }
    }

    if (sTxDone)
    {
        //otLogWarnPlat("Tx Status %X\n", sTransmitError);
        do
        {
            if (sTransmitError == 0x10)
            {
                sTransmitError = OT_ERROR_CHANNEL_ACCESS_FAILURE;
                sTxWait = false;
                break;
            }
            else if (sTransmitError == 0x20)
            {
                sTransmitError = OT_ERROR_NO_ACK;
                sTxWait = false;
                break;
            }
            else if (sTransmitError == 0)
            {
                sTransmitError = OT_ERROR_NONE;
                sTxWait = false;
                break;
            }

            if (otMacFrameIsAckRequested(&sTransmitFrame))
            {
                if (otMacFrameIsVersion2015(&sTransmitFrame))
                {
                    //otLogWarnPlat("Tx Ver 2015\n");
                    if (Enh_Ack_Index != MAC_RX_BUFFERS)
                    {
                        sTxWait = false;
                        pAckFrame = &sAckFrame;
                        sTransmitError = OT_ERROR_NONE;
                    }
                }
                else
                {
                    otMacFrameGenerateImmAck(&sTransmitFrame, (sTransmitError == 0x40) ? 1 : 0, &sAckFrame);
                    pAckFrame = &sAckFrame;
                    sTxWait = false;
                    sTransmitError = OT_ERROR_NONE;
                }
            }
        } while (0);

        if (sTxWait == false)
        {
            otPlatRadioTxDone(maInstance, &sTransmitFrame,
                              pAckFrame,
                              sTransmitError);

            sTxDone = false;

            sState = OT_RADIO_STATE_RECEIVE;
        }
    }

}
/* Rafael RFB functions */

static rf_rx_buffer_t *_mac_rx_buffer_free_find(void)
{
    for (uint32_t i = 0; i < MAC_RX_BUFFERS; i++)
    {
        if (sMacRxBuffer[i].free)
        {
            return &sMacRxBuffer[i];
        }
    }
    return NULL;
}
static void _mac_rx_buffer_free(uint8_t *pbuf)
{
    for (uint32_t i = 0; i < MAC_RX_BUFFERS; i++)
    {
        if (sMacRxBuffer[i].data == pbuf)
        {
            enter_critical_section();
            sMacRxBuffer[i].free = true;
            sMacRxBuffer[i].acl = false;
            leave_critical_section();
            break;
        }
    }
}

static bool _mac_rx_buffer_chk_acl(uint8_t *pbuf)
{
    for (uint32_t i = 0; i < MAC_RX_BUFFERS; i++)
    {
        if (sMacRxBuffer[i].data == pbuf)
        {
            if(sMacRxBuffer[i].acl == 1)
                return true;
        }
    }
    return false;
}


/**
 * @brief
 *
 * @param u8_tx_status
 */
static void rafael_tx_done(uint8_t u8_tx_status)
{
    
    sTransmitError = u8_tx_status;
    //sTxWait = false;
    sTxDone = true;
    //gpio_pin_write(20, 1);

    otSysEventSignalPending();
}

/**
 * @brief
 *
 * @param packet_length
 * @param rx_data_address
 * @param crc_status
 * @param rssi
 * @param snr
 */
static void rafael_rx_done(uint16_t packet_length, uint8_t *rx_data_address,
                           uint8_t crc_status, uint8_t rssi, uint8_t snr)
{
    uint32_t i;
    rf_rx_buffer_t *p_rx_buffer;

    if (crc_status == 0)
    {
        //util_log_mem(UTIL_LOG_INFO, "Rx ", rx_data_address, packet_length, 0);
        for (i = 0; i < MAC_RX_BUFFERS; i++)
        {
            if (sReceiveFrame[i].mPsdu == NULL)
            {
                memset(&sReceiveFrame[i], 0, sizeof(otRadioFrame));

                p_rx_buffer = _mac_rx_buffer_free_find();
                if (p_rx_buffer)
                {
                    if (rx_data_address[0] != BLE_TRANSPORT_HCI_ACL_DATA)
                    {
                        sReceiveFrame[i].mPsdu = p_rx_buffer->data;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
                        sReceiveFrame[i].mLength = packet_length - 9;
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
                        sReceiveFrame[i].mLength = packet_length - 10;
#endif
                        // sReceiveFrame[i].mInfo.mRxInfo.mTimestamp = otPlatAlarmMicroGetNow()-4000;
                        sReceiveFrame[i].mInfo.mRxInfo.mRssi = -rssi;
                        sReceiveFrame[i].mInfo.mRxInfo.mLqi = ((RAFAEL_RECEIVE_SENSITIVITY - rssi) * 0xFF) / RAFAEL_RECEIVE_SENSITIVITY;
                        sReceiveFrame[i].mChannel = sCurrentChannel;
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
                        memcpy(sReceiveFrame[i].mPsdu, rx_data_address + 8, sReceiveFrame[i].mLength);
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
                        memcpy(sReceiveFrame[i].mPsdu, rx_data_address + 9, sReceiveFrame[i].mLength);
#endif                       
                        //gpio_pin_write(21, 0);
                    }
                    else
                    {
                        sReceiveFrame[i].mPsdu = p_rx_buffer->data;
                        sReceiveFrame[i].mLength = (((rx_data_address[3]) | (rx_data_address[4] << 8)) + 5);
                        memcpy(sReceiveFrame[i].mPsdu, rx_data_address, sReceiveFrame[i].mLength);

                        p_rx_buffer->acl = 1;
                    }
                    p_rx_buffer->free = false;
                }
                break;
            }
        }
    }
    otSysEventSignalPending();

}
void rafael_radio_short_addr_ctrl(uint8_t ctrl_type, uint8_t *short_addr)
{
    //spRFBCtrl->short_addr_ctrl(ctrl_type, short_addr);
}

void rafael_radio_extend_addr_ctrl(uint8_t ctrl_type, uint8_t *extend_addr)
{
    //spRFBCtrl->extend_addr_ctrl(ctrl_type, extend_addr);
}

void rafael_rfb_init(void)
{
    //info("OT %s\n", __func__);
    /* Init MAC rx buffer */
    for (uint32_t i = 0; i < MAC_RX_BUFFERS; i++)
    {
        sMacRxBuffer[i].free = true;
        sReceiveFrame[i].mPsdu = NULL;
    }
    sAckFrame.mPsdu      = sAckMessage.mPsdu;

    /* Register rfb interrupt event */
    sRFBInterruptEvt.tx_done = rafael_tx_done;
    sRFBInterruptEvt.rx_done = rafael_rx_done;

    /* Initial RFB */

#if (MODULE_ENABLE(SUPPORT_MATTER_CONCURRENT))
    spRFBCtrl = rfb_multi_init();
#else

#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    spRFBCtrl = rfb_zb_init();
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    spRFBCtrl = rfb_wisun_init();
#endif
#endif

    spRFBCtrl->init(&sRFBInterruptEvt);

    /* PHY PIB */
    spRFBCtrl->phy_pib_set(sTurnaroundTime,
                           sCCAMode,
                           sCCAThreshold,
                           sCCADuration);

    /* MAC PIB */
    spRFBCtrl->mac_pib_set(MAC_PIB_UNIT_BACKOFF_PERIOD,
                           sMacAckWaitTime,
                           MAC_PIB_MAC_MAX_BE,
                           MAC_PIB_MAC_MAX_CSMACA_BACKOFFS,
                           MAC_PIB_MAC_MAX_FRAME_TOTAL_WAIT_TIME,
                           sMacFrameRetris,
                           MAC_PIB_MAC_MIN_BE);


    flash_get_unique_id((uint32_t)sIEEE_EUI64Addr, 7);

    /* Auto ACK */
    spRFBCtrl->auto_ack_set(true);

    /* Auto State */
    spRFBCtrl->auto_state_set(false);

    sTransmitFrame.mPsdu = sTransmitMessage.mPsdu;

#if OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    /*Set RF State to Idle*/
    spRFBCtrl->idle_set();
    /*tx init*/
    spRFBCtrl->tx_config_set(sPhyTxPower, sPhyDataRate, 8, sPhyModulation, FSK_CRC_16, WHITEN_DISABLE, GFSK);

    /*rx init*/
    spRFBCtrl->rx_config_set(sPhyDataRate, 8, sPhyModulation, FSK_CRC_16, WHITEN_DISABLE, 0, true, GFSK);
#endif


#if 1
    /*
    * Set channel frequency :
    * For band is subg, units is kHz
    * For band is 2.4g, units is mHz
    */
#if OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT
    uint32_t ChannelFrequency = FREQ + 5 * (sCurrentChannel - 11);
#elif OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT
    uint32_t ChannelFrequency = FREQ + 200 * (sCurrentChannel - 1) ;
#endif
    spRFBCtrl->frequency_set(ChannelFrequency);
#endif
    spRFBCtrl->address_filter_set(sPromiscuous,
                                  sShortAddress,
                                  sExtendAddr_0,
                                  sExtendAddr_1,
                                  sPANID,
                                  sCoordinator);
    otRFBInit = 1;
}

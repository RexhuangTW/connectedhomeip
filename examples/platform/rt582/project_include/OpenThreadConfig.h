/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      Overrides to default OpenThread configuration.
 *
 */

#pragma once

// Disable the Qorvo-supplied OpenThread logging facilities and use
// the facilities provided by the Device Layer (see
// src/platform/qpg/Logging.cpp).
#define OPENTHREAD_CONFIG_LOG_OUTPUT OPENTHREAD_CONFIG_LOG_OUTPUT_APP

// When operating in a less than ideal RF environment, having a more forgiving configuration
// of OpenThread makes thread a great deal more reliable.
//#define OPENTHREAD_CONFIG_TMF_ADDRESS_QUERY_MAX_RETRY_DELAY 120    // default is 28800
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_DIRECT 4 //15  // default is 3
#define OPENTHREAD_CONFIG_MAC_DEFAULT_MAX_FRAME_RETRIES_INDIRECT 8 //1 // default is 0
//#define OPENTHREAD_CONFIG_MAC_MAX_TX_ATTEMPTS_INDIRECT_POLLS 16    // default is 4

// Enable periodic parent search to speed up finding a better parent.
//#define OPENTHREAD_CONFIG_PARENT_SEARCH_ENABLE 1                   // default is 0
//#define OPENTHREAD_CONFIG_PARENT_SEARCH_RSS_THRESHOLD -45          // default is -65
//#define OPENTHREAD_CONFIG_MLE_INFORM_PREVIOUS_PARENT_ON_REATTACH 1 // default is 0

// Use smaller maximum interval to speed up reattaching.
//#define OPENTHREAD_CONFIG_MLE_ATTACH_BACKOFF_MAXIMUM_INTERVAL (60 * 10 * 1000) // default 1200000 ms

// Turn on a moderate level of logging in OpenThread
// Enable use of external heap allocator (calloc/free) for OpenThread.
#define OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE 1

#define OPENTHREAD_CONFIG_NUM_MESSAGE_BUFFERS 44

#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_NOTE
//#define OPENTHREAD_CONFIG_LOG_LEVEL OT_LOG_LEVEL_WARN
/*
 * @def OPENTHREAD_CONFIG_PLATFORM_INFO
 *
 * The platform-specific string to insert into the OpenThread version string.
 *
 */

#ifndef OPENTHREAD_CONFIG_PLATFORM_INFO
#define OPENTHREAD_CONFIG_PLATFORM_INFO "RT582"
#endif

/**
 * @def OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
 *
 * The maximum number of children.
 *
 */
#ifndef OPENTHREAD_CONFIG_MLE_MAX_CHILDREN
#define OPENTHREAD_CONFIG_MLE_MAX_CHILDREN 10
#endif


/**
 * @def OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE
 *
 * Define to 1 if you want to enable software transmission security logic.
 *
 */
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_SECURITY_ENABLE 1
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_CSMA_BACKOFF_ENABLE 0
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_RETRANSMIT_ENABLE 0
#define OPENTHREAD_CONFIG_MAC_SOFTWARE_TX_TIMING_ENABLE 1

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
 *
 * Define to 1 if you want to support microsecond timer in platform.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_USEC_TIMER_ENABLE 0
#endif

/**
 * @def OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
 *
 * Define to 1 to enable otPlatFlash* APIs to support non-volatile storage.
 *
 * When defined to 1, the platform MUST implement the otPlatFlash* APIs instead of the otPlatSettings* APIs.
 *
 */
#ifndef OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE
#define OPENTHREAD_CONFIG_PLATFORM_FLASH_API_ENABLE 1
#endif

/**
 * @def OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE
 *
 * The size of heap buffer when DTLS is enabled.
 *
 */
#ifndef OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE
#define OPENTHREAD_CONFIG_HEAP_INTERNAL_SIZE (2048 * sizeof(void *))
#endif
/**
 * @def OPENTHREAD_CONFIG_CLI_TX_BUFFER_SIZE
 *
 *  The size of CLI message buffer in bytes
 *
 */

// Limit CLI buffers
#define OPENTHREAD_CONFIG_CLI_UART_RX_BUFFER_SIZE 384
#define OPENTHREAD_CONFIG_CLI_UART_TX_BUFFER_SIZE 512

//#define OPENTHREAD_CONFIG_NCP_HDLC_ENABLE 1
#define OPENTHREAD_CONFIG_IP6_SLAAC_ENABLE 1
#define OPENTHREAD_CONFIG_DTLS_ENABLE 1
#define OPENTHREAD_CONFIG_ENABLE_BUILTIN_MBEDTLS_MANAGEMENT 1
#define OPENTHREAD_CONFIG_PING_SENDER_ENABLE    1

#define OPENTHREAD_CONFIG_ECDSA_ENABLE 1
#define OPENTHREAD_CONFIG_DETERMINISTIC_ECDSA_ENABLE 1


#define OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT 0
#define OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT 1
#if (OPENTHREAD_CONFIG_RADIO_915MHZ_OQPSK_SUPPORT == 1)
#define OPENTHREAD_CONFIG_IP6_MAX_DATAGRAM_LENGTH 2000
#define OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE 0
#endif

#if (OPENTHREAD_CONFIG_RADIO_2P4GHZ_OQPSK_SUPPORT == 1)
#define OPENTHREAD_CONFIG_IP6_FRAGMENTATION_ENABLE 1
#endif

#define OPENTHREAD_CONFIG_SRP_CLIENT_ENABLE 1
#define OPENTHREAD_CONFIG_DNS_CLIENT_ENABLE 1

#define OPENTHREAD_CONFIG_LOG_LEVEL_DYNAMIC_ENABLE 0
#define OPENTHREAD_CONFIG_LOG_PLATFORM 1
#define OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE 0
#define OPENTHREAD_CONFIG_DIAG_ENABLE 0

#define OPENTHREAD_CONFIG_CHILD_SUPERVISION_ENABLE 0
#define OPENTHREAD_CONFIG_COAP_SECURE_API_ENABLE 0

#define OPENTHREAD_CONFIG_COAP_API_ENABLE 0
#define OPENTHREAD_CONFIG_JOINER_ENABLE 0
#define OPENTHREAD_CONFIG_COMMISSIONER_ENABLE 0
#define OPENTHREAD_CONFIG_UDP_FORWARD_ENABLE 0
#define OPENTHREAD_CONFIG_BORDER_ROUTER_ENABLE 0
#define OPENTHREAD_CONFIG_DHCP6_CLIENT_ENABLE 0
#define OPENTHREAD_CONFIG_DHCP6_SERVER_ENABLE 0
#define OPENTHREAD_CONFIG_TCP_ENABLE 0

#define OPENTHREAD_CONFIG_MAC_CSL_REQUEST_AHEAD_US 0
#define OPENTHREAD_CONFIG_THREAD_VERSION OT_THREAD_VERSION_1_3

#define OPENTHREAD_CONFIG_DUA_ENABLE 1
#define OPENTHREAD_CONFIG_MLR_ENABLE 1
#define DOPENTHREAD_CONFIG_SRP_CLIENT_ENABLE 1
#define DOPENTHREAD_CONFIG_UDP_FORWARD_ENABLE 1
// Use the Qorvo-supplied default platform configuration for remainder
// of OpenThread config options.
//
// NB: This file gets included during the build of OpenThread.  Hence
// it cannot use "openthread" in the path to the included file.
//
// #include "openthread-core-RT58x-config.h"

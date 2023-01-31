#
#  Copyright (c) 2021, The OpenThread Authors.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#  3. Neither the name of the copyright holder nor the
#     names of its contributors may be used to endorse or promote products
#     derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.
#


set(OT_PLATFORM_LIB "openthread-rt58x" PARENT_SCOPE)

if(NOT OT_CONFIG)
    set(OT_CONFIG "openthread-core-RT58x-config.h")
    set(OT_CONFIG ${OT_CONFIG} PARENT_SCOPE)
endif()

list(APPEND OT_PLATFORM_DEFINES
    "OPENTHREAD_CORE_CONFIG_PLATFORM_CHECK_FILE=\"openthread-core-RT58x-config-check.h\""
)
set(OT_PLATFORM_DEFINES ${OT_PLATFORM_DEFINES} PARENT_SCOPE)

list(APPEND OT_PLATFORM_DEFINES "OPENTHREAD_PROJECT_CORE_CONFIG_FILE=\"${OT_CONFIG}\"")

add_library(openthread-rt58x
    src/alarm.c
    src/entropy.c
    src/flash.c
    src/logging.c
    src/misc.c
    src/radio.c
    src/system.c
    src/uart.c
    src/mac_frame.cpp
    src/soft_source_match_table.c
    src/Portable/bsp/bsp.c
    src/Portable/bsp/bsp_button.c
    src/Portable/bsp/bsp_console.c
    src/Portable/bsp/bsp_led.c
    src/Portable/bsp/bsp_uart.c
    src/Portable/bsp/bsp_uart_drv.c
    src/Portable/Utility/util.c
    src/Portable/Utility/util_log.c
    src/Portable/Utility/util_printf.c
    src/Portable/Utility/util_string.c
    src/Portable/Utility/util_queue.c
)

set_target_properties(
    openthread-rt58x
    PROPERTIES
        C_STANDARD 99
        CXX_STANDARD 11
)

target_link_libraries(openthread-rt58x PRIVATE rafael-rt58x-driver ot-config)
target_link_options(openthread-rt58x PUBLIC -T${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/rt58x.ld)
target_link_options(openthread-rt58x PUBLIC -Wl,--gc-sections -Wl,-Map=$<TARGET_PROPERTY:NAME>.map)
target_link_options(openthread-rt58x PUBLIC -Wl,--print-memory-usage)

target_compile_definitions(openthread-rt58x
    PUBLIC
        ${OT_PLATFORM_DEFINES}
)

target_compile_options(openthread-rt58x PRIVATE
    ${OT_CFLAGS}
    -Wno-unknown-pragmas
    -Wno-sign-compare
    -Wno-unused-function
    -Wno-unused-parameter
    -Wno-empty-body
    -w
)

target_include_directories(openthread-rt58x PRIVATE
    ${OT_PUBLIC_INCLUDES}
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/examples/platforms
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/RFB/include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/RUCI/include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/Portable/bsp/Include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/Portable/Utility/include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/CMSIS/Include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/Driver/Include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/Driver/Peripheral/Include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/Driver/Crypto/include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/Driver/PHY/include
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/Driver/PHY/rf_fw
    ${PROJECT_SOURCE_DIR}/examples/platforms/rt58x/src/Driver/PHY/rt569mp/include
)

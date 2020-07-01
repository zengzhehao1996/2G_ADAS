# Top-level CMakeLists.txt for the skeleton application.
#
# Copyright (c) 2017 Open Source Foundries Limited
#
# SPDX-License-Identifier: Apache-2.0
#
# This provides a basic application structure suitable for loading by
# mcuboot, which is easy to customize on a per-board basis. It can be
# used as a starting point for new applications.

# The default top-level application configuration is prj.conf.
# You can place additional board-specific files in boards/${BOARD}.conf,
# and they will be merged into the configuration along with prj.conf.
macro(set_conf_file)
  if(EXISTS ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf)
    set(CONF_FILE "prj.conf ${APPLICATION_SOURCE_DIR}/boards/${BOARD}.conf")
  else()
    set(CONF_FILE "prj.conf")
  endif()
endmacro()

# Standard Zephyr application boilerplate.
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

# Choice hard board
set(SMART_BUILD_BOARD $ENV{SMART_BUILD_BOARD})
if(SMART_BUILD_BOARD STREQUAL "SMART_LINK_V133")
    message("Choice build v133 board ")
    set(SMART_LINK_V133 true)
elseif(SMART_BUILD_BOARD STREQUAL "SMART_LINK_V131")
    message("Choice build v131 board")
    set(SMART_LINK_V131 true)
elseif(SMART_BUILD_BOARD STREQUAL "SMART_LINK_V141")
    message("Choice build v141 board")
    set(SMART_LINK_V141 true)
else()
    #define set v133 board
    set(SMART_LINK_V133 true)
endif()

include($ENV{ZEPHYR_BASE}/apps/linde/version.cmake)
configure_file(version.h.in ${PROJECT_BINARY_DIR}/include/smart_link_version.h)
INCLUDE_DIRECTORIES(${PROJECT_BINARY_DIR}/include)

ADD_DEFINITIONS(-DMQTT_COMM_ENABLED=y)
ADD_DEFINITIONS(-DMQTT_DEVICE_SHADOW=n)
ADD_DEFINITIONS(-DCOAP_COMM_ENABLED=n)
ADD_DEFINITIONS(-DOTA_COMM_ENABLED=y)
ADD_DEFINITIONS(-DOTA_SIGNAL_CHANNEL=MQTT)
ADD_DEFINITIONS(-DAUTH_MODE=KEY)
ADD_DEFINITIONS(-DAUTH_MODE_KEY=y)
ADD_DEFINITIONS(-DAUTH_WITH_NOTLS=y)
ADD_DEFINITIONS(-DSYSTEM_COMM_ENABLED=y)

ADD_DEFINITIONS(-DRELEASE_VERSION)

# Used for All board ***************
INCLUDE_DIRECTORIES(
    src/fs
    src/fs/fatfs
    src/fs/fatfs/drivers
    src/fs/fatfs/optoin
    src/protocol
    src/networkingThread
    src/utils
    src/rfid
    src/vehicle_ctrl
    src/inter-thread_comm
    src/thread
    src/mpu
    src/fota
    src/networkingThread/cache
    src/factory_test
    lib/can/include
	src/work_data/rs485_sensor_thread
	
    src/netlib
    src/qcloud
    src/qcloud/src
    src/qcloud/src/coap/include
    src/qcloud/src/device/include
    src/qcloud/src/mqtt/include
    src/qcloud/src/nbiot/include
    src/qcloud/src/ota/include
    src/qcloud/src/sdk-impl
    src/qcloud/src/sdk-impl/exports
    src/qcloud/src/shadow/include
    src/qcloud/src/utils/digest
    src/qcloud/src/utils/farra
    src/qcloud/src/utils/lite
    src
)
# **************************************

# Choice mpu mpu6050 or bmi160
if(SMART_LINK_V131)
    INCLUDE_DIRECTORIES(src/mpu/mpu6050)
    aux_source_directory(src/mpu/mpu6050 MPU6050)
    target_sources(app PRIVATE ${MPU6050})
elseif(SMART_LINK_V133 OR SMART_LINK_V141)
    INCLUDE_DIRECTORIES(src/mpu/bmi160)
    aux_source_directory(src/mpu/bmi160 BMI160)
    target_sources(app PRIVATE ${BMI160})
else()
    message("No board")
endif()

# Choice can
if(SMART_LINK_V131 OR SMART_LINK_V133)
    INCLUDE_DIRECTORIES(src/work_data/can)
#    INCLUDE_DIRECTORIES(src/work_data/can/rs485_sensor_thread)
	INCLUDE_DIRECTORIES(src/factory_test/v133_factory_test)
    aux_source_directory(src/work_data/can WORK_DATA)
#    aux_source_directory(src/work_data/can/rs485_sensor_thread CARRY_SENSOR)
	aux_source_directory(src/factory_test/v133_factory_test V133_FACTORY_TEST)
#    target_sources(app PRIVATE ${CARRY_SENSOR})
	target_sources(app PRIVATE ${V133_FACTORY_TEST})
elseif(SMART_LINK_V141)
    INCLUDE_DIRECTORIES(src/work_data/no_can)
	INCLUDE_DIRECTORIES(src/work_data/no_can/fork_sensor)
	INCLUDE_DIRECTORIES(src/work_data/no_can/move_sensor)
	INCLUDE_DIRECTORIES(src/work_data/no_can/genenal_sensor)
	INCLUDE_DIRECTORIES(src/factory_test/v141_factory_test)
	aux_source_directory(src/factory_test/v141_factory_test V141_FACTORY_TEST)
	aux_source_directory(src/work_data/no_can WORK_DATA)
    aux_source_directory(src/work_data/no_can/fork_sensor FORK_SENSOR)
	aux_source_directory(src/work_data/no_can/move_sensor MOVE_SENSOR)
	aux_source_directory(src/work_data/no_can/genenal_sensor GENENAL_SENSOR)
	target_sources(app PRIVATE ${V141_FACTORY_TEST})
    target_sources(app PRIVATE ${FORK_SENSOR})
	target_sources(app PRIVATE ${MOVE_SENSOR})
	target_sources(app PRIVATE ${GENENAL_SENSOR})
else()
    message("No board")
endif()

#choose beep type 
if(SMART_LINK_V131 OR SMART_LINK_V133)
	INCLUDE_DIRECTORIES(src/vehicle_ctrl/active_beep)
	aux_source_directory(src/vehicle_ctrl/active_beep VEHICL_BEEP)
elseif(SMART_LINK_V141)
	INCLUDE_DIRECTORIES(src/vehicle_ctrl/passive_beep)
	aux_source_directory(src/vehicle_ctrl/passive_beep VEHICL_BEEP)
else()
	message("NO Board")
endif()

# choise hard version
if(SMART_LINK_V133)
    INCLUDE_DIRECTORIES(src/utils/v133)
    aux_source_directory(src/utils/v133 HARD_VERSION)
    target_sources(app PRIVATE ${HARD_VERSION})
elseif(SMART_LINK_V131)
    INCLUDE_DIRECTORIES(src/utils/v131)
    aux_source_directory(src/utils/v131 HARD_VERSION)
    target_sources(app PRIVATE ${HARD_VERSION})
elseif(SMART_LINK_V141)
    INCLUDE_DIRECTORIES(src/utils/v141)
    aux_source_directory(src/utils/v141 HARD_VERSION)
    target_sources(app PRIVATE ${HARD_VERSION})
endif()

# **** Used for All board *************************************************
aux_source_directory(src/ SRC)
aux_source_directory(src/rfid/ RFID)
aux_source_directory(src/vehicle_ctrl/ CTRL)
aux_source_directory(src/mpu/ MPU)
aux_source_directory(src/fota/ FOTA)
aux_source_directory(src/thread THREAD)
aux_source_directory(src/inter-thread_comm INTER_THREAD_COMM)
aux_source_directory(src/protocol/ PROTOCOL)
aux_source_directory(src/utils/ UTILS)
aux_source_directory(src/networkingThread/ NETWORKING)
aux_source_directory(src/networkingThread/cache/ CACHE)
aux_source_directory(src/fs/ FS)
aux_source_directory(src/fs/fatfs/ FS_FATFS)
aux_source_directory(src/fs/fatfs/drivers/ FS_FATFS_DRIVERS)
aux_source_directory(src/factory_test FACTORY_TEST)
aux_source_directory(src/work_data/rs485_sensor_thread CARRY_SENSOR)
set(FS_FATFS_OPTION src/fs/fatfs/option/syscall.c src/fs/fatfs/option/unicode.c)

target_sources(app PRIVATE 
    ${SRC}
    ${FS} ${FS_FATFS} ${FS_FATFS_DRIVERS} ${FS_FATFS_OPTION}
    ${PROTOCOL}
    ${UTILS}
    ${RFID}
    ${CTRL}
    ${MPU}
    ${WORK_DATA}
    ${INTER_THREAD_COMM}
    ${NETWORKING}
    ${THREAD}
    ${FOTA}
    ${CACHE}
    ${FACTORY_TEST}
    ${RS485_SENSOR_THREAD}
	${VEHICL_BEEP}
    ${CARRY_SENSOR}
    
    src/qcloud/src/device/src/device.c

    src/qcloud/src/mqtt/src/mqtt_client.c
    src/qcloud/src/mqtt/src/mqtt_client_common.c
    src/qcloud/src/mqtt/src/mqtt_client_connect.c
    src/qcloud/src/mqtt/src/mqtt_client_net.c
    src/qcloud/src/mqtt/src/mqtt_client_publish.c
    src/qcloud/src/mqtt/src/mqtt_client_subscribe.c
    src/qcloud/src/mqtt/src/mqtt_client_yield.c
    
    src/qcloud/src/utils/digest/utils_base64.c
    
    src/qcloud/src/utils/farra/utils_list.c
    src/qcloud/src/utils/farra/utils_net.c
    src/qcloud/src/utils/farra/utils_timer.c
    
    src/qcloud/src/platform/os/zephyr/HAL_OS_linux.c
    src/qcloud/src/platform/os/zephyr/HAL_TCP_linux.c
    src/qcloud/src/platform/os/zephyr/HAL_Timer_linux.c

    src/netlib/atcip.c
    src/netlib/atc.c
    src/netlib/atd.c  
    src/netlib/gpio.c
    src/netlib/gpd.c
)
# *************************************************************************

#Choice link library
if(SMART_LINK_V133 OR SMART_LINK_V131)
    link_directories("lib/can")
    execute_process(COMMAND cp ${PROJECT_SOURCE_DIR}/lib/can/libcan.a ./zephyr)
    target_link_libraries(app libcan.a)
elseif(SMART_LINK_V141)
    #Custom Connection Library of v141
endif()

include(${ZEPHYR_BASE}/cmake/hex.cmake)

set(FOTA_SWITCH $ENV{FOTA_SWITCH})

if(SMART_LINK_V131)
    file(READ ${ZEPHYR_BASE}/apps/linde/VERSION_V131 sver)
elseif(SMART_LINK_V133)
    file(READ ${ZEPHYR_BASE}/apps/linde/VERSION_V133 sver)
elseif(SMART_LINK_V141)
    file(READ ${ZEPHYR_BASE}/apps/linde/VERSION_V141 sver)
else()
    # default used hard133
    file(READ ${ZEPHYR_BASE}/apps/linde/VERSION_V133 sver)
endif()

string(REGEX MATCH "DEV_VERSION_MAJOR *=* ([0-9]*)" _ ${sver})
set(DEV_VERSION_MAJOR ${CMAKE_MATCH_1})
# message("1 ${DEV_VERSION_MAJOR}        ${CMAKE_MATCH_1}")

string(REGEX MATCH "DEV_VERSION_MINOR *=* ([0-9]*)" _ ${sver})
set(DEV_VERSION_MINOR ${CMAKE_MATCH_1})
# message("2 ${DEV_VERSION_MINOR}        ${CMAKE_MATCH_1}")

string(REGEX MATCH "DEV_VERSION_TINY *=* ([0-9]*)" _ ${sver})
set(DEV_VERSION_TINY ${CMAKE_MATCH_1})
# message("3 ${DEV_VERSION_TINY}        ${CMAKE_MATCH_1}")

string(REGEX MATCH "DEV_VERSION_PATCH *=* ([0-9]*)" _ ${sver})
set(DEV_VERSION_PATCH ${CMAKE_MATCH_1})
# message("4 ${DEV_VERSION_PATCH}        ${CMAKE_MATCH_1}")

string(REGEX MATCH "SOFT_VERSION_MAJOR *=* ([0-9]*)" _ ${sver})
set(SOFT_VERSION_MAJOR ${CMAKE_MATCH_1})
# message("5 ${SOFT_VERSION_MAJOR}        ${CMAKE_MATCH_1}")

string(REGEX MATCH "SOFT_VERSION_MINOR *=* ([0-9]*)" _ ${sver})
set(SOFT_VERSION_MINOR ${CMAKE_MATCH_1})

# message("6 ${SOFT_VERSION_MINOR}        ${CMAKE_MATCH_1}")

string(REGEX MATCH "SOFT_VERSION_TINY *=* ([0-9]*)" _ ${sver})
set(SOFT_VERSION_TINY ${CMAKE_MATCH_1})
# message("7 ${SOFT_VERSION_TINY}        ${CMAKE_MATCH_1}")

string(REGEX MATCH "SOFT_VERSION_PATCH *=* ([0-9]*)" _ ${sver})
if(1 EQUAL FOTA_SWITCH)
    set(SOFT_VERSION_PATCH 0)
else()
    set(SOFT_VERSION_PATCH ${CMAKE_MATCH_1})
endif()
# message("8 ${SOFT_VERSION_PATCH}        ${CMAKE_MATCH_1}")

# set current timestamp
set(CUR_TIMESTAMP $ENV{CUR_TIMESTAMP})

# set server and log switch
set(SERVER_SWITCH $ENV{SERVER_SWITCH})
set(LOG_SWITCH    $ENV{LOG_SWITCH}) 

message("SmartLink Hard version: ${DEV_VERSION_MAJOR}.${DEV_VERSION_MINOR}.${DEV_VERSION_TINY}.${DEV_VERSION_PATCH}")
message("SmartLink soft version: ${SOFT_VERSION_MAJOR}.${SOFT_VERSION_MINOR}.${SOFT_VERSION_TINY}.${SOFT_VERSION_PATCH}")

set(SMART_VERSION_STR "${SOFT_VERSION_MAJOR}.${SOFT_VERSION_MINOR}.${SOFT_VERSION_TINY}.${SOFT_VERSION_PATCH}")
file(WRITE $ENV{ZEPHYR_BASE}/apps/.tmp_version ${SMART_VERSION_STR})

unset(SMART_VERSION_STR)



#[[
// clang-format off
]]#
# Copyright (c) 2018 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set(MODULE_NAME otctl)

set(cxx-sources
  "CLI.cpp"
  "main.cpp"
)

set(cxx-headers
  "CLI.hpp"
        util.h
)

add_executable(${MODULE_NAME}
  ${cxx-sources}
  ${cxx-headers})

target_link_libraries(
  ${MODULE_NAME}
  PRIVATE
  Threads::Threads
  ${APP_SYSTEM_LIBRARIES}
  ${PROTOBUF_LITE_LIBRARIES}
  ${OPENTXS_PROTO_LIBRARIES}
  ${OPENTXS_LIBRARIES}
  ${Boost_SYSTEM_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARIES}
  ${Boost_PROGRAM_OPTIONS_LIBRARIES}
  ${CURSES_LIBRARIES}
  ${JSONCPP_LIBRARIES}
)

if(OT_WITH_QT)
   target_link_libraries(${MODULE_NAME} PRIVATE ${Qt5Core_LIBRARIES} ${Qt5Qml_LIBRARIES})
endif()

target_include_directories(
  ${MODULE_NAME}
  PRIVATE
  ${PROTOBUF_INCLUDE_DIRS}
  ${OPENTXS_INCLUDE_DIRS}
  ${OPENTXS_PROTO_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${CURSES_INCLUDE_DIRS}
  ${JSONCPP_INCLUDE_DIRS}
)

set_property(TARGET ${MODULE_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${MODULE_NAME} PROPERTY CXX_STANDARD 17)

install(TARGETS ${MODULE_NAME} DESTINATION bin)

#[[
// clang-format on
]]#

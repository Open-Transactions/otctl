# Copyright (c) 2019 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

set(cxx-sources "CLI.cpp" "main.cpp")

set(cxx-headers "CLI.hpp" util.h)

add_executable(otctl ${cxx-sources} ${cxx-headers})

target_link_libraries(
  otctl
  PRIVATE
    Threads::Threads
    opentxs
    jsoncpp_lib
    ${OPENTXS_LIBRARIES}
    ${Boost_SYSTEM_LIBRARIES}
    ${Boost_FILESYSTEM_LIBRARIES}
    ${Boost_PROGRAM_OPTIONS_LIBRARIES}
    ${CURSES_LIBRARIES}
)

if(CMAKE_DL_LIBS)
  target_link_libraries(otctl PRIVATE ${CMAKE_DL_LIBS})
endif()

if(LIB_RT)
  target_link_libraries(otctl PRIVATE ${LIB_RT})
endif()

target_include_directories(otctl PRIVATE ${CURSES_INCLUDE_DIRS})

install(TARGETS otctl DESTINATION bin)

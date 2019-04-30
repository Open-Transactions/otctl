#[[
// clang-format off
]]#
# Copyright (c) 2018 The Open-Transactions developers
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.5)
project(otctl)

find_package(Git)
if(GIT_FOUND)
    execute_process(COMMAND git "describe" OUTPUT_VARIABLE GIT_VERSION WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX REPLACE "^v*([0-9]+)\\..*" "\\1" VERSION_MAJOR "${GIT_VERSION}")
    string(REGEX REPLACE "^v*[0-9]+\\.([0-9]+).*" "\\1" VERSION_MINOR "${GIT_VERSION}")
    string(REGEX REPLACE "^v*[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" VERSION_BUGFIX "${GIT_VERSION}")
    string(REGEX REPLACE "^v*[0-9]+\\.[0-9]+\\.[0-9]+-([0-9]+)-.*" "\\1" VERSION_NEW_COMMITS "${GIT_VERSION}")
    string(REGEX REPLACE "^v*[0-9]+\\.[0-9]+\\.[0-9]+-[0-9]+-(.*)" "\\1" VERSION_SHA1 "${GIT_VERSION}")
    if(("${VERSION_SHA1}" STREQUAL "") OR ("${VERSION_SHA1}" STREQUAL "${GIT_VERSION}"))
        set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUGFIX}")
    else()
        set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_BUGFIX}-${VERSION_NEW_COMMITS}-${VERSION_SHA1}")
    endif()
else()
  message(FATAL_ERROR "Git not found.")
endif()


#-----------------------------------------------------------------------------
# Options for building

option(BUILD_VERBOSE       "Verbose build output." ON)

set(PACKAGE_CONTACT        ""              CACHE <TYPE>  "Package Maintainer")
set(PACKAGE_VENDOR         "Stash Crypto"  CACHE <TYPE>  "Package Vendor")
option(OT_WITH_QT          "opentxs was built with Qt support" OFF)

if(BUILD_VERBOSE)
  set(CMAKE_VERBOSE_MAKEFILE true)
endif()

#-----------------------------------------------------------------------------
# Print system information

message(STATUS "otctl version:                ${VERSION_STRING}")
message(STATUS "CMake version:                ${CMAKE_VERSION}")
message(STATUS "System:                       ${CMAKE_SYSTEM}")
message(STATUS "Processor:                    ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Verbose:                      ${BUILD_VERBOSE}")
message(STATUS "Package Contact:              ${PACKAGE_CONTACT}")
message(STATUS "Package Vendor:               ${PACKAGE_VENDOR}")

#-----------------------------------------------------------------------------
# Collect all binaries into bin subdirectory

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

#-----------------------------------------------------------------------------
# Compile options

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(PEDANTIC_CXX_FLAGS "-Werror")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -std=c11")

if(${CMAKE_CXX_COMPILER_ID} MATCHES GNU)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
  set(PEDANTIC_CXX_FLAGS "${PEDANTIC_CXX_FLAGS} -Wall -Wextra -pedantic -pedantic-errors -Wswitch-default -Wswitch-enum -Wcast-align -Wdisabled-optimization -Wmissing-declarations -Wmissing-field-initializers -Wunused-macros -Wundef -Wformat=2 -Wlogical-op -Wmissing-include-dirs")
  set(PEDANTIC_CXX_FLAGS "${PEDANTIC_CXX_FLAGS} -Weffc++ -Wctor-dtor-privacy -Wold-style-cast -Woverloaded-virtual -Wsynth -Wnoexcept -Wstrict-null-sentinel")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
  set(PEDANTIC_CXX_FLAGS "${PEDANTIC_CXX_FLAGS} -Weverything -Wno-c++98-compat-pedantic -Wno-padded -Wno-weak-vtables -Wno-disabled-macro-expansion -Wno-sign-conversion -Wno-float-conversion -Wno-covered-switch-default -Wno-global-constructors -Wno-exit-time-destructors")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PEDANTIC_CXX_FLAGS}")

if(APPLE)
	include_directories(/usr/local/include)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reserved-id-macro -Wno-zero-as-null-pointer-constant -Wno-deprecated-dynamic-exception-spec -Wno-unused-parameter -Wno-redundant-parens -Wno-documentation -Wno-inconsistent-missing-destructor-override -Wno-non-virtual-dtor -Wno-switch-enum -Wno-old-style-cast -Wno-extra-semi -Wno-documentation-unknown-command -Wno-comma -Wno-double-promotion -Wno-shadow-field -Wno-deprecated -Wno-shorten-64-to-32")
endif()

message(STATUS "C Compiler ID:      ${CMAKE_C_COMPILER_ID}")
message(STATUS "C Flags:            ${CMAKE_C_FLAGS}")
message(STATUS "C link flags:       ${CMAKE_C_LINK_FLAGS}")
message(STATUS "C++ Compiler ID:    ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "C++ Flags:          ${CMAKE_CXX_FLAGS}")
message(STATUS "C++ link flags:     ${CMAKE_CXX_LINK_FLAGS}")

#-----------------------------------------------------------------------------
# System libraries used for linking.

if(UNIX AND NOT APPLE)
  list(APPEND APP_SYSTEM_LIBRARIES rt)
endif()
list(APPEND APP_SYSTEM_LIBRARIES dl)

#-----------------------------------------------------------------------------
# Dependencies

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(opentxs-proto REQUIRED)
find_package(opentxs REQUIRED)
find_package(Boost REQUIRED program_options filesystem system)
set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
find_package(Jsoncpp REQUIRED)

if(OT_WITH_QT)
  find_package(Qt5 COMPONENTS Core Qml REQUIRED)
  set_target_properties(Qt5::Core PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "RELEASE")
  list(GET Qt5Core_INCLUDE_DIRS 0 QT_INCLUDE_DIR)
  include_directories("${QT_INCLUDE_DIR}")
endif()

#-----------------------------------------------------------------------------
# Force out-of-source build

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
message(FATAL_ERROR "CMake generation is not allowed within the source directory!
Remove the CMakeCache.txt file and try again from another folder, e.g.:

   rm CMakeCache.txt
   mkdir build
   cd build
   cmake ..
")
endif()


#-----------------------------------------------------------------------------
# Default to Debug build

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif()

#-----------------------------------------------------------------------------
# Build source

add_definitions(-D_XOPEN_SOURCE=700)
add_subdirectory(src)


#-----------------------------------------------------------------------------
# Uninstall
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/uninstall.cmake)

#[[
// clang-format on
]]#

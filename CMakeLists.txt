# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)


# the following lines before "### END ###" are added during project localization
### START ###
set(ENV{HPM_SDK_BASE} "${CMAKE_CURRENT_SOURCE_DIR}/hpm_sdk_localized_for_hpm5301evklite")
set(LOCALIZED_BOARD "hpm5301evklite")
if(BOARD)
  if(NOT ${BOARD} MATCHES ${LOCALIZED_BOARD})
    message(FATAL_ERROR "ABORT:\n hpm sdk has been localized for ${LOCALIZED_BOARD} already.")
  endif()
endif()
set(BOARD ${LOCALIZED_BOARD})
### END ###


# LibXR HPM drivers require the HPM SDK dma_mgr component.
set(CONFIG_DMA_MGR 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(user_app)

sdk_app_inc(inc)
sdk_app_src(src/main.cpp)

# Debugging profile:
# - keep application entry source easy to debug
# - allow LibXR to stay optimized for timing/size
string(TOLOWER "${CMAKE_BUILD_TYPE}" _app_build_type)
if(_app_build_type STREQUAL "debug")
  if(TARGET app)
    target_compile_options(app PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-O0>
        $<$<COMPILE_LANGUAGE:CXX>:-g3>
    )
  endif()
endif()

# Add LibXR integration
include(${CMAKE_CURRENT_LIST_DIR}/cmake/LibXR.CMake)

# Copyright (c) 2021 HPMicro
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

# The board lives in boards/ next to the application; the SDK comes from HPM_SDK_BASE.
set(BOARD_SEARCH_PATH ${CMAKE_CURRENT_SOURCE_DIR}/boards)

# LibXR HPM drivers require the HPM SDK dma_mgr component.
set(CONFIG_DMA_MGR 1)

find_package(hpm-sdk REQUIRED HINTS $ENV{HPM_SDK_BASE})

project(user_app)

sdk_app_src(main.c User/app_main.cpp)
sdk_app_inc(User)

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

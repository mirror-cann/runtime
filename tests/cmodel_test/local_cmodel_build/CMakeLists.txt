# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------
include_guard(GLOBAL)

set(CMODEL_BUILD_PRODUCT_TYPE ${BUILD_RUNTIME_CMODEL_PRODUCT})
set(CMODEL_BUILD_SHIM_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(CMODEL_BUILD_RUNTIME_CORE_INC_DIR ${RUNTIME_DIR}/src/runtime/core/inc)
set(CMODEL_BUILD_DRIVER_SHIM_SRC ${CMAKE_CURRENT_SOURCE_DIR}/driver_backend_shim.c)

if(NOT TARGET model_headers)
    add_library(model_headers INTERFACE)
    target_include_directories(model_headers INTERFACE
        $<BUILD_INTERFACE:${CMODEL_BUILD_SHIM_INCLUDE_DIR}>
    )
endif()

if(NOT TARGET tsch_headers)
    add_library(tsch_headers INTERFACE)
    target_include_directories(tsch_headers INTERFACE
        $<BUILD_INTERFACE:${CMODEL_BUILD_SHIM_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${CMODEL_BUILD_RUNTIME_CORE_INC_DIR}>
    )
endif()

add_subdirectory(${RUNTIME_DIR}/src/cmodel_driver ${CMAKE_BINARY_DIR}/src/cmodel_driver)

set(CMODEL_BUILD_ENABLE_LOCAL_SHIM OFF)
if("${CMODEL_BUILD_PRODUCT_TYPE}" STREQUAL "ascend910B1" OR
   "${CMODEL_BUILD_PRODUCT_TYPE}" STREQUAL "ascend310B" OR
   "${CMODEL_BUILD_PRODUCT_TYPE}" STREQUAL "ascend950pr_9599" OR
   "${CMODEL_BUILD_PRODUCT_TYPE}" STREQUAL "ascend350" OR
   "${CMODEL_BUILD_PRODUCT_TYPE}" STREQUAL "mc62cm12a" OR
   "${CMODEL_BUILD_PRODUCT_TYPE}" STREQUAL "mc32dm11a")
    set(CMODEL_BUILD_ENABLE_LOCAL_SHIM ON)
endif()

if(CMODEL_BUILD_ENABLE_LOCAL_SHIM)
    foreach(target_name
        npu_drv_${CMODEL_BUILD_PRODUCT_TYPE}
        npu_drv_pvmodel_${CMODEL_BUILD_PRODUCT_TYPE}
        npu_drv_camodel_${CMODEL_BUILD_PRODUCT_TYPE})
        target_sources(${target_name} PRIVATE
            ${CMODEL_BUILD_DRIVER_SHIM_SRC}
        )
        target_include_directories(${target_name} PRIVATE
            ${CMODEL_BUILD_SHIM_INCLUDE_DIR}
            ${CMODEL_BUILD_RUNTIME_CORE_INC_DIR}
        )
    endforeach()
endif()

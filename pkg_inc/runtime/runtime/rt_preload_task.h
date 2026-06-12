/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef CCE_RUNTIME_RT_PRELOAD_TASK_H
#define CCE_RUNTIME_RT_PRELOAD_TASK_H

#include "runtime/base.h"
#include "runtime/rt_external_preload.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @ingroup rt_preload_task
 * @brief get elf header offset
 * @param [in] elfData   kernel bin addr
 * @param [in] elfLen
 * @param [out] offset   elf header offset
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtGetElfOffset(void * const elfData, const uint32_t elfLen, uint32_t* offset);

/**
 * @ingroup rt_preload_task
 * @brief regkernel callback function
 * @param [in] symbol   symbol_name
 * @param [in] kenelLauchFillFunc kenelLauchFillFunc
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtRegKernelLaunchFillFunc(const char* symbol, rtKernelLaunchFillFunc callback);

/**
 * @ingroup rt_preload_task
 * @brief unregkernel callback function
 * @param [in] symbol   symbol_name
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtUnRegKernelLaunchFillFunc(const char* symbol);

/**
* @ingroup rt_preload_task
* @brief get l2cache offset
* @param [in] deviceId   device id
* @param [out]  offset l2cache offset
* @return  0 for success, others for fail
*/
RTS_API rtError_t rtGetL2CacheOffset(uint32_t deviceId, uint64_t *offset);

#if defined(__cplusplus)
}
#endif

#endif  // CCE_RUNTIME_RT_PRELOAD_TASK_H

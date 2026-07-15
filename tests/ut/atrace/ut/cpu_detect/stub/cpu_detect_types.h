/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CPU_DETECT_TYPES_H
#define CPU_DETECT_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t CpudStatus;

#define CPUD_SUCCESS 0
#define CPUD_FAILURE (-1)
#define CPUD_ERROR_PARAM (-2)
#define CPUD_ERROR_BUSY (-3)
#define CPUD_ERROR_CREATE_PROCESS (-4)
#define CPUD_ERROR_STOP (-5)
#define CPUD_ERROR_TESTCASE (-6)
#define CPUD_ERROR_CGROUP_BIND (-7)
#define CPUD_ERROR_CPU_AFFINITY (-8)
#define CPUD_ERROR_MALLOC (-9)
#define CPUD_ERROR_CREATE_THREAD (-10)
#define CPUD_ERROR_INIT (-11)

#define CPUD_DETECT_MAX_TIME 3600

#ifdef __cplusplus
}
#endif

#endif

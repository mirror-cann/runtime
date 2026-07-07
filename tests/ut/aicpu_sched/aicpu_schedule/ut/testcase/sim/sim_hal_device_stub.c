/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// 设备拓扑：6 核 — 0=CCPU, 1=DCPU, 2-5=AICPU

#include "ascend_hal.h"
#include "ascend_hal_base.h"
#include "ascend_hal_define.h"
#include "ascend_hal_error.h"

#define SIM_AICPU_CORE_NUM 4
#define SIM_AICPU_OCCUPY 0x3CLL
#define SIM_CCPU_CORE_NUM 1
#define SIM_CCPU_OCCUPY 0x1LL
#define SIM_DCPU_CORE_NUM 1
#define SIM_DCPU_OCCUPY 0x2LL

static void FillModuleInfo(int32_t infoType, int64_t coreNum, int64_t occupy, int64_t* value)
{
    switch (infoType) {
        case INFO_TYPE_CORE_NUM:
        case INFO_TYPE_PF_CORE_NUM:
            *value = coreNum;
            break;
        case INFO_TYPE_OCCUPY:
        case INFO_TYPE_PF_OCCUPY:
            *value = occupy;
            break;
        case INFO_TYPE_OS_SCHED:
            *value = 1;
            break;
        default:
            *value = 0;
            break;
    }
}

drvError_t halGetDeviceInfo(uint32_t devId, int32_t moduleType, int32_t infoType, int64_t* value)
{
    (void)devId;
    if (value == NULL) {
        return DRV_ERROR_PARA_ERROR;
    }

    switch (moduleType) {
        case MODULE_TYPE_AICPU:
            FillModuleInfo(infoType, SIM_AICPU_CORE_NUM, SIM_AICPU_OCCUPY, value);
            break;
        case MODULE_TYPE_CCPU:
            FillModuleInfo(infoType, SIM_CCPU_CORE_NUM, SIM_CCPU_OCCUPY, value);
            break;
        case MODULE_TYPE_DCPU:
            FillModuleInfo(infoType, SIM_DCPU_CORE_NUM, SIM_DCPU_OCCUPY, value);
            break;
        default:
            *value = 0;
            break;
    }
    return DRV_ERROR_NONE;
}

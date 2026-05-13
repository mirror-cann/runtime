/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "maintenance_task.h"
#include "task_manager.h"

namespace cce {
namespace runtime {

rtError_t NpuGetFloatStaTaskInit(TaskInfo *taskInfo, void * const outputAddrPtr,
                                 const uint64_t outputSize, const uint32_t checkMode,
                                 bool debugFlag)
{
    TaskCommonInfoInit(taskInfo);
    taskInfo->typeName = "NPU_GET_FLOAT_STATUS";
    taskInfo->type = TS_TASK_TYPE_NPU_GET_FLOAT_STATUS;

    NpuGetFloatStatusTaskInfo *npuGetFloatSta = &taskInfo->u.npuGetFloatStatusTask;

    npuGetFloatSta->outputAddrPtr = outputAddrPtr;
    npuGetFloatSta->outputSize = outputSize;
    npuGetFloatSta->checkMode = checkMode;
    npuGetFloatSta->debugFlag = debugFlag;
    return RT_ERROR_NONE;
}

rtError_t NpuClrFloatStaTaskInit(TaskInfo *taskInfo, const uint32_t checkMode, bool debugFlag)
{
    TaskCommonInfoInit(taskInfo);
    taskInfo->typeName = "NPU_CLEAR_FLOAT_STATUS";
    taskInfo->type = TS_TASK_TYPE_NPU_CLEAR_FLOAT_STATUS;
    taskInfo->u.npuClrFloatStatusTask.checkMode = checkMode;
    taskInfo->u.npuClrFloatStatusTask.debugFlag = debugFlag;
    return RT_ERROR_NONE;
}

} // namespace runtime
} // namespace cce
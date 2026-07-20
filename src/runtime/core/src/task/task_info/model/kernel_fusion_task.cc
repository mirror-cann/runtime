/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "kernel_fusion_task.h"
#include "task_manager.h"

namespace cce {
namespace runtime {

rtError_t KernelFusionTaskInit(TaskInfo* const taskInfo, const FusionFlag fusFlag)
{
    KernelFusionTaskInfo* kernelFusionTaskInfo = &(taskInfo->u.kernelFusionTaskInfo);
    TaskCommonInfoInit(taskInfo);

    taskInfo->type = TS_TASK_TYPE_FUSION_ISSUE;
    taskInfo->typeName = "KERNEL_FUSION";
    kernelFusionTaskInfo->flag = fusFlag;
    return RT_ERROR_NONE;
}

void ToCommandBodyForKernelFusionTask(TaskInfo* const taskInfo, rtCommand_t* const command)
{
    KernelFusionTaskInfo* kernelFusionTaskInfo = &(taskInfo->u.kernelFusionTaskInfo);
    command->u.fusion.flag = kernelFusionTaskInfo->flag;
}

} // namespace runtime
} // namespace cce
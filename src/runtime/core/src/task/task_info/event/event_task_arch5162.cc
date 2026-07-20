/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "event_task.h"
#include "task_info_v100.h"

namespace cce {
namespace runtime {

uint16_t GetSqeEventId(const rtStarsSqe_t* sqe)
{
    UNUSED(sqe);
    return 0;
}

void ConstructSqeForNotifyRecordTask(TaskInfo* taskInfo, rtStarsSqe_t* const command)
{
    UNUSED(taskInfo);
    UNUSED(command);
}

} // namespace runtime
} // namespace cce

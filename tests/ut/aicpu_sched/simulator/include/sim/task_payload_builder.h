/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef SIM_TASK_PAYLOAD_BUILDER_H
#define SIM_TASK_PAYLOAD_BUILDER_H

#include <cstdint>
#include <securec.h>
#include <vector>

#include "ascend_hal_base.h"

namespace sim {
// 构造 hwts_ts_task（EVENT_TS_HWTS_KERNEL 事件载荷）
class HwtsTsTaskBuilder {
public:
    HwtsTsTaskBuilder() { (void)memset_s(&task_, sizeof(task_), 0, sizeof(task_)); }

    HwtsTsTaskBuilder& MailboxId(uint32_t id)
    {
        task_.mailbox_id = id;
        return *this;
    }
    HwtsTsTaskBuilder& SerialNo(uint64_t no)
    {
        task_.serial_no = no;
        return *this;
    }
    HwtsTsTaskBuilder& Pid(int32_t pid)
    {
        task_.kernel_info.pid = pid;
        return *this;
    }
    HwtsTsTaskBuilder& KernelType(uint16_t type)
    {
        task_.kernel_info.kernel_type = (type & 0xFFU);
        return *this;
    }
    HwtsTsTaskBuilder& StreamId(uint16_t streamId)
    {
        task_.kernel_info.streamID = streamId;
        return *this;
    }
    HwtsTsTaskBuilder& KernelName(uint64_t kernelNameAddr)
    {
        task_.kernel_info.kernelName = kernelNameAddr;
        return *this;
    }
    HwtsTsTaskBuilder& KernelSo(uint64_t kernelSoAddr)
    {
        task_.kernel_info.kernelSo = kernelSoAddr;
        return *this;
    }
    HwtsTsTaskBuilder& ParamBase(uint64_t paramBaseAddr)
    {
        task_.kernel_info.paramBase = paramBaseAddr;
        return *this;
    }
    HwtsTsTaskBuilder& BlockId(uint16_t blockId)
    {
        task_.kernel_info.blockId = blockId;
        return *this;
    }
    HwtsTsTaskBuilder& BlockNum(uint16_t blockNum)
    {
        task_.kernel_info.blockNum = blockNum;
        return *this;
    }
    HwtsTsTaskBuilder& TaskId(uint64_t taskId)
    {
        task_.kernel_info.taskID = taskId;
        return *this;
    }

    const hwts_ts_task& Get() const { return task_; }
    std::vector<char> Build() const
    {
        const char* p = reinterpret_cast<const char*>(&task_);
        return std::vector<char>(p, p + sizeof(task_));
    }

private:
    hwts_ts_task task_;
};
} // namespace sim

#endif // SIM_TASK_PAYLOAD_BUILDER_H

/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef SIM_TSD_H
#define SIM_TSD_H

#include <atomic>
#include <cstdint>
#include <unistd.h>

namespace sim {

// tsd 仿真引擎：模拟 tsdaemon 与 aicpu_sched 之间的交互。
// WaitForShutDown 阻塞等待 SimNotifyShutdown 通知，使 ComputeProcessMain 在事件处理完成前不退出。
// 其余 tsd 接口均为空操作桩（返回成功），由 sim_tsd.cpp 以强符号提供。
class TsdSimEngine {
public:
    static TsdSimEngine& Instance()
    {
        static TsdSimEngine instance;
        return instance;
    }

    void Reset() { shutdownFlag_.store(false); }

    // 通知 ComputeProcessMain 退出 WaitForShutDown
    void NotifyShutdown() { shutdownFlag_.store(true); }

    // 阻塞等待 shutdown 通知（由 WaitForShutDown 强符号桩调用）
    int32_t WaitForShutdown(uint32_t deviceId)
    {
        (void)deviceId;
        while (!shutdownFlag_.load()) {
            usleep(1000U);
        }
        shutdownFlag_.store(false);
        return 0;
    }

private:
    TsdSimEngine() = default;
    std::atomic<bool> shutdownFlag_{false};
};

} // namespace sim

// 测试面快捷接口
void SimNotifyShutdown();

#endif // SIM_TSD_H

/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef SIM_SIM_PLATFORM_H
#define SIM_SIM_PLATFORM_H

#include "sim/esched_sim_engine.h"
#include "sim/event_info_builder.h"
#include "sim/task_payload_builder.h"

namespace sim {
// 测试面门面：聚合各仿真引擎，提供统一 Reset 做用例隔离
class SimPlatform {
public:
    static EschedSimEngine& Esched() { return EschedSimEngine::Instance(); }

    static void Reset() { EschedSimEngine::Instance().Reset(); }
};
} // namespace sim

#endif // SIM_SIM_PLATFORM_H

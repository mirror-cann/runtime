/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef __CCE_RUNTIME_LOAD_POLICY_HPP__
#define __CCE_RUNTIME_LOAD_POLICY_HPP__

#include <cstdint>

namespace cce {
namespace runtime {

class Stream;

// 参数加载语义策略
enum class LoadPolicy : uint8_t {
    LP_GENERIC = 0, // starsv2默认路径
    LP_NO_MIX,      // Load
    LP_MIX,         // LoadForMix
    LP_CPU_KRN,     // LoadCpuKernelArgs
    LP_CPU_KRN_EX,  // LoadCpuKernelArgsEx
    LP_FFTS         // PureLoad
};

bool ShouldCopyByPolicy(const Stream& stream, uint8_t isNoNeedH2DCopy, LoadPolicy policy);

} // namespace runtime
} // namespace cce

#endif // __CCE_RUNTIME_LOAD_POLICY_HPP__

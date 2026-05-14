/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "load_policy.hpp"
#include "stream.hpp"

namespace cce {
namespace runtime {
bool ShouldCopyByPolicy(const Stream& stream, uint8_t isNoNeedH2DCopy, LoadPolicy policy)
{
    switch (policy) {
        case LoadPolicy::LP_GENERIC:
        case LoadPolicy::LP_NO_MIX:
        case LoadPolicy::LP_MIX: {
            return isNoNeedH2DCopy == 0U;
        }
        case LoadPolicy::LP_CPU_KRN:
        case LoadPolicy::LP_CPU_KRN_EX: {
            return stream.NonSupportModelCompile() || (isNoNeedH2DCopy == 0U);
        }
        case LoadPolicy::LP_FFTS: {
            return true;
        }
        default: {
            return isNoNeedH2DCopy == 0U;
        }
    }
}

} // namespace runtime
} // namespace cce

/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "dev_info.h"
#include "dev_info_manage.h"
#include "device_error_proc.hpp"

namespace cce {
namespace runtime {
static bool HitBlackListErrors(uint32_t devId) { return IsHitBlacklist(devId, g_mulBitEccEventIdBlkList); }

static DevDynInfoProcFunc CHIP_DAVID_PROC_FUNC = {
    .devPropsUpdateFunc = nullptr,
    .devHitBlackListErrors = &HitBlackListErrors,
};

REGISTER_DEV_INFO_PROC_FUNC(CHIP_DAVID, CHIP_DAVID_PROC_FUNC);
} // namespace runtime
} // namespace cce
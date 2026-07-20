/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef RUNTIME_ARGS_BUFFER_GUARD_HPP
#define RUNTIME_ARGS_BUFFER_GUARD_HPP

#include "base.hpp"

namespace cce {
namespace runtime {

class ArgsBufferGuard {
public:
    ~ArgsBufferGuard();
    void* EnsureCapacity(uint64_t requiredSize);

private:
    static constexpr uint64_t ARGS_BUFFER_DEFAULT_SIZE = 4096ULL;
    void* buffer_ = nullptr;
    uint64_t size_ = 0ULL;
};

} // namespace runtime
} // namespace cce

#endif // RUNTIME_ARGS_BUFFER_GUARD_HPP
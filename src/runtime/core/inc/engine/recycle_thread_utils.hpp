/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef __CCE_RUNTIME_RECYCLE_THREAD_UTILS_HPP__
#define __CCE_RUNTIME_RECYCLE_THREAD_UTILS_HPP__

#include "base.hpp"
#include <thread>

namespace cce {
namespace runtime {

/**
 * @brief Cleanup recycle thread resources on Start() failure.
 *
 * After recycleThreadAlive_ is set to true, another thread may call
 * WakeUpRecycleThread() which increments inFlightWakeUps_ and may
 * call mmSemPost on recycleThreadSem_. This helper ensures all
 * in-flight WakeUpRecycleThread calls complete before destroying
 * the semaphore, preventing use-after-destroy.
 *
 * @param alive       Reference to recycleThreadAlive_ (set to false before resource cleanup)
 * @param thread      Reference to recycleThread_ pointer (will be deleted and nulled)
 * @param sem         Reference to recycleThreadSem_ (will be destroyed)
 * @param inFlight    Reference to inFlightWakeUps_ counter (drained to 0)
 */
inline void CleanupRecycleThreadOnFailure(
    std::atomic<bool> &alive,
    Thread *&thread,
    mmSem_t &sem,
    std::atomic<int32_t> &inFlight)
{
    alive.store(false, std::memory_order_release);
    DELETE_O(thread);
    while (inFlight.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }
    (void)mmSemDestroy(&sem);
}

}  // namespace runtime
}  // namespace cce

#endif  // __CCE_RUNTIME_RECYCLE_THREAD_UTILS_HPP__

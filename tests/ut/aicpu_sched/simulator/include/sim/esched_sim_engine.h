/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef SIM_ESCHED_SIM_ENGINE_H
#define SIM_ESCHED_SIM_ENGINE_H

#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <tuple>
#include <vector>

#include "ascend_hal_base.h"
#include "ascend_hal_define.h"
#include "ascend_hal_error.h"
#include "ascend_hal_external.h"

namespace sim {
struct SimEvent {
    EVENT_ID eventId = EVENT_RANDOM_KERNEL;
    uint32_t subeventId = 0U;
    int32_t pid = 0;
    int32_t hostPid = 0;
    uint32_t grpId = 0U;
    uint64_t submitTs = 0U;
    uint64_t schedTs = 0U;
    std::vector<char> msg; // 任务载荷，拷入 event_info.priv.msg
};

// halEschedSubmitEvent 捕获记录
struct SubmittedEvent {
    uint32_t devId = 0U;
    int32_t pid = 0;
    uint32_t grpId = 0U;
    EVENT_ID eventId = EVENT_RANDOM_KERNEL;
    uint32_t subeventId = 0U;
    uint32_t dstEngine = 0U;
    std::vector<char> msg;
};

// halEschedAckEvent 捕获记录
struct AckRecord {
    uint32_t devId = 0U;
    EVENT_ID eventId = EVENT_RANDOM_KERNEL;
    uint32_t subeventId = 0U;
    std::vector<char> msg;
};

// esched 事件队列引擎，线程安全
// 控制面 API：
//   InjectEvent(dev,grp,SimEvent)          注入待消费事件
//   SetWaitReturnSequence({codes})         强制 wait 返回码序列（异常路径）
//   SetEmptyQueueBehavior(code)            空队列返回码（默认 NO_EVENT）
//   SetExitWhenDrained(true)               队列耗尽即 PROCESS_EXIT
//   SetAutoExitAfter(n)                    第 n 次 wait 兜底退出
//   SetSubmitLoopback(true)                submit 回灌事件队列（split kernel 闭环）
//   TakeSubmittedEvents()/TakeAckRecords() 应答/回执断言取数
//   AckedEventCount(eventId)               统计指定类型 ack 数（非破坏性）
//   PendingEventCount(dev,grp)             队列待消费事件数
class EschedSimEngine {
public:
    static EschedSimEngine& Instance();

    void Reset();

    drvError_t AttachDevice(uint32_t devId);
    drvError_t DetachDevice(uint32_t devId);
    drvError_t CreateGrp(uint32_t devId, uint32_t grpId, GROUP_TYPE type);
    drvError_t SubscribeEvent(uint32_t devId, uint32_t grpId, uint32_t threadId, uint64_t eventBitmap);
    drvError_t WaitEvent(uint32_t devId, uint32_t grpId, uint32_t threadId, int32_t timeout, event_info* event);
    drvError_t GetEvent(uint32_t devId, uint32_t grpId, uint32_t threadId, EVENT_ID eventId, event_info* event);
    drvError_t SubmitEvent(uint32_t devId, const event_summary* event);
    drvError_t SubmitEventBatch(
        uint32_t devId, SUBMIT_FLAG flag, const event_summary* events, uint32_t eventNum, uint32_t* succEventNum);
    drvError_t AckEvent(uint32_t devId, EVENT_ID eventId, uint32_t subeventId, const char* msg, uint32_t msgLen);

    void InjectEvent(uint32_t devId, uint32_t grpId, const SimEvent& event);
    void SetWaitReturnSequence(const std::vector<drvError_t>& codes);
    void SetEmptyQueueBehavior(drvError_t code);
    // 队列耗尽即返回 PROCESS_EXIT，使 LoopProcess 退出
    void SetExitWhenDrained(bool enable);
    void SetAutoExitAfter(uint32_t maxWaitCount);
    // submit 的事件回灌进 wait 队列，供 split kernel 等闭环场景
    void SetSubmitLoopback(bool enable);

    std::vector<SubmittedEvent> TakeSubmittedEvents();
    std::vector<AckRecord> TakeAckRecords();
    uint32_t WaitCount() const;
    size_t AckedEventCount(EVENT_ID eventId);
    size_t PendingEventCount(uint32_t devId, uint32_t grpId);

private:
    EschedSimEngine() = default;
    ~EschedSimEngine() = default;
    EschedSimEngine(const EschedSimEngine&) = delete;
    EschedSimEngine& operator=(const EschedSimEngine&) = delete;

    using QueueKey = std::pair<uint32_t, uint32_t>;             // (devId, grpId)
    using ThreadKey = std::tuple<uint32_t, uint32_t, uint32_t>; // (devId, grpId, threadId)

    void FillEventInfo(const SimEvent& src, event_info* out) const;
    bool MatchSubscription(const ThreadKey& key, EVENT_ID eventId) const;

    mutable std::mutex mtx_;
    std::map<QueueKey, std::deque<SimEvent>> eventQueues_;
    std::map<ThreadKey, uint64_t> subscriptions_;
    std::map<uint32_t, int32_t> attachRefCnt_;
    std::vector<SubmittedEvent> submitted_;
    std::vector<AckRecord> acks_;
    std::deque<drvError_t> waitReturnSeq_;
    drvError_t emptyQueueCode_ = DRV_ERROR_SCHED_NO_EVENT;
    bool exitWhenDrained_ = false;
    uint32_t autoExitAfter_ = 0U;
    uint32_t waitCount_ = 0U;
    bool submitLoopback_ = false;
};
} // namespace sim

#endif // SIM_ESCHED_SIM_ENGINE_H

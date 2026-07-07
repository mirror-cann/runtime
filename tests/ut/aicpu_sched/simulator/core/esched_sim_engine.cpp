/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "sim/esched_sim_engine.h"

#include <algorithm>
#include <securec.h>

namespace sim {
namespace {
constexpr uint32_t kMaxMsgLen = static_cast<uint32_t>(EVENT_MAX_MSG_LEN);

uint32_t ClampLen(size_t len) { return static_cast<uint32_t>(std::min<size_t>(len, kMaxMsgLen)); }
} // namespace

EschedSimEngine& EschedSimEngine::Instance()
{
    static EschedSimEngine instance;
    return instance;
}

void EschedSimEngine::Reset()
{
    std::lock_guard<std::mutex> lock(mtx_);
    eventQueues_.clear();
    subscriptions_.clear();
    attachRefCnt_.clear();
    submitted_.clear();
    acks_.clear();
    waitReturnSeq_.clear();
    emptyQueueCode_ = DRV_ERROR_SCHED_NO_EVENT;
    exitWhenDrained_ = false;
    autoExitAfter_ = 0U;
    waitCount_ = 0U;
    submitLoopback_ = false;
}

void EschedSimEngine::FillEventInfo(const SimEvent& src, event_info* out) const
{
    out->comm.event_id = src.eventId;
    out->comm.subevent_id = src.subeventId;
    out->comm.pid = src.pid;
    out->comm.host_pid = src.hostPid;
    out->comm.grp_id = src.grpId;
    out->comm.submit_timestamp = src.submitTs;
    out->comm.sched_timestamp = src.schedTs;
    const uint32_t len = ClampLen(src.msg.size());
    out->priv.msg_len = len;
    if (len > 0U) {
        (void)memcpy_s(out->priv.msg, EVENT_MAX_MSG_LEN, src.msg.data(), len);
    }
}

bool EschedSimEngine::MatchSubscription(const ThreadKey& key, EVENT_ID eventId) const
{
    const auto it = subscriptions_.find(key);
    if (it == subscriptions_.end()) {
        return true; // 未显式订阅：投递任意事件
    }
    const uint64_t bit = 1ULL << static_cast<uint32_t>(eventId);
    return (it->second & bit) != 0ULL;
}

drvError_t EschedSimEngine::AttachDevice(uint32_t devId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    attachRefCnt_[devId]++;
    return DRV_ERROR_NONE;
}

drvError_t EschedSimEngine::DetachDevice(uint32_t devId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = attachRefCnt_.find(devId);
    if (it != attachRefCnt_.end() && it->second > 0) {
        it->second--;
    }
    return DRV_ERROR_NONE;
}

drvError_t EschedSimEngine::CreateGrp(uint32_t devId, uint32_t grpId, GROUP_TYPE type)
{
    (void)type;
    std::lock_guard<std::mutex> lock(mtx_);
    (void)eventQueues_[QueueKey(devId, grpId)];
    return DRV_ERROR_NONE;
}

drvError_t EschedSimEngine::SubscribeEvent(uint32_t devId, uint32_t grpId, uint32_t threadId, uint64_t eventBitmap)
{
    std::lock_guard<std::mutex> lock(mtx_);
    subscriptions_[ThreadKey(devId, grpId, threadId)] = eventBitmap;
    return DRV_ERROR_NONE;
}

drvError_t EschedSimEngine::WaitEvent(
    uint32_t devId, uint32_t grpId, uint32_t threadId, int32_t timeout, event_info* event)
{
    (void)timeout;
    std::lock_guard<std::mutex> lock(mtx_);
    if (event == nullptr) {
        return DRV_ERROR_PARA_ERROR;
    }
    if (!waitReturnSeq_.empty()) {
        const drvError_t code = waitReturnSeq_.front();
        waitReturnSeq_.pop_front();
        return code;
    }
    waitCount_++;

    auto& queue = eventQueues_[QueueKey(devId, grpId)];
    const ThreadKey tkey(devId, grpId, threadId);
    for (auto it = queue.begin(); it != queue.end(); ++it) {
        if (MatchSubscription(tkey, it->eventId)) {
            FillEventInfo(*it, event);
            (void)queue.erase(it);
            return DRV_ERROR_NONE;
        }
    }
    if ((autoExitAfter_ != 0U) && (waitCount_ >= autoExitAfter_)) {
        return DRV_ERROR_SCHED_PROCESS_EXIT;
    }
    if (exitWhenDrained_) {
        return DRV_ERROR_SCHED_PROCESS_EXIT;
    }
    return emptyQueueCode_;
}

drvError_t EschedSimEngine::GetEvent(
    uint32_t devId, uint32_t grpId, uint32_t threadId, EVENT_ID eventId, event_info* event)
{
    (void)threadId;
    std::lock_guard<std::mutex> lock(mtx_);
    if (event == nullptr) {
        return DRV_ERROR_PARA_ERROR;
    }
    // GetEvent 按 eventId 主动拉取，不检查订阅、不阻塞
    auto& queue = eventQueues_[QueueKey(devId, grpId)];
    for (auto it = queue.begin(); it != queue.end(); ++it) {
        if (it->eventId == eventId) {
            FillEventInfo(*it, event);
            (void)queue.erase(it);
            return DRV_ERROR_NONE;
        }
    }
    return DRV_ERROR_SCHED_NO_EVENT;
}

drvError_t EschedSimEngine::SubmitEvent(uint32_t devId, const event_summary* event)
{
    std::lock_guard<std::mutex> lock(mtx_);
    if (event == nullptr) {
        return DRV_ERROR_PARA_ERROR;
    }
    SubmittedEvent rec;
    rec.devId = devId;
    rec.pid = event->pid;
    rec.grpId = event->grp_id;
    rec.eventId = event->event_id;
    rec.subeventId = event->subevent_id;
    rec.dstEngine = event->dst_engine;
    if ((event->msg != nullptr) && (event->msg_len > 0U)) {
        const uint32_t len = ClampLen(event->msg_len);
        rec.msg.assign(event->msg, event->msg + len);
    }
    submitted_.push_back(rec);

    if (submitLoopback_) {
        SimEvent ev;
        ev.eventId = event->event_id;
        ev.subeventId = event->subevent_id;
        ev.pid = event->pid;
        ev.grpId = event->grp_id;
        ev.msg = rec.msg;
        eventQueues_[QueueKey(devId, event->grp_id)].push_back(ev);
    }
    return DRV_ERROR_NONE;
}

drvError_t EschedSimEngine::SubmitEventBatch(
    uint32_t devId, SUBMIT_FLAG flag, const event_summary* events, uint32_t eventNum, uint32_t* succEventNum)
{
    if ((events == nullptr) && (eventNum != 0U)) {
        return DRV_ERROR_PARA_ERROR;
    }
    for (uint32_t i = 0U; i < eventNum; ++i) {
        const event_summary* ev = (flag == SHARED_EVENT_ENTRY) ? &events[0] : &events[i];
        (void)SubmitEvent(devId, ev);
    }
    if (succEventNum != nullptr) {
        *succEventNum = eventNum;
    }
    return DRV_ERROR_NONE;
}

drvError_t EschedSimEngine::AckEvent(
    uint32_t devId, EVENT_ID eventId, uint32_t subeventId, const char* msg, uint32_t msgLen)
{
    std::lock_guard<std::mutex> lock(mtx_);
    AckRecord rec;
    rec.devId = devId;
    rec.eventId = eventId;
    rec.subeventId = subeventId;
    if ((msg != nullptr) && (msgLen > 0U)) {
        const uint32_t len = ClampLen(msgLen);
        rec.msg.assign(msg, msg + len);
    }
    acks_.push_back(rec);
    return DRV_ERROR_NONE;
}

void EschedSimEngine::InjectEvent(uint32_t devId, uint32_t grpId, const SimEvent& event)
{
    std::lock_guard<std::mutex> lock(mtx_);
    eventQueues_[QueueKey(devId, grpId)].push_back(event);
}

void EschedSimEngine::SetWaitReturnSequence(const std::vector<drvError_t>& codes)
{
    std::lock_guard<std::mutex> lock(mtx_);
    waitReturnSeq_.assign(codes.begin(), codes.end());
}

void EschedSimEngine::SetEmptyQueueBehavior(drvError_t code)
{
    std::lock_guard<std::mutex> lock(mtx_);
    emptyQueueCode_ = code;
}

void EschedSimEngine::SetExitWhenDrained(bool enable)
{
    std::lock_guard<std::mutex> lock(mtx_);
    exitWhenDrained_ = enable;
}

void EschedSimEngine::SetAutoExitAfter(uint32_t maxWaitCount)
{
    std::lock_guard<std::mutex> lock(mtx_);
    autoExitAfter_ = maxWaitCount;
}

void EschedSimEngine::SetSubmitLoopback(bool enable)
{
    std::lock_guard<std::mutex> lock(mtx_);
    submitLoopback_ = enable;
}

std::vector<SubmittedEvent> EschedSimEngine::TakeSubmittedEvents()
{
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<SubmittedEvent> out;
    out.swap(submitted_);
    return out;
}

std::vector<AckRecord> EschedSimEngine::TakeAckRecords()
{
    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<AckRecord> out;
    out.swap(acks_);
    return out;
}

size_t EschedSimEngine::AckedEventCount(EVENT_ID eventId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    size_t n = 0U;
    for (const auto& a : acks_) {
        if (a.eventId == eventId) {
            ++n;
        }
    }
    return n;
}

uint32_t EschedSimEngine::WaitCount() const
{
    std::lock_guard<std::mutex> lock(mtx_);
    return waitCount_;
}

size_t EschedSimEngine::PendingEventCount(uint32_t devId, uint32_t grpId)
{
    std::lock_guard<std::mutex> lock(mtx_);
    return eventQueues_[QueueKey(devId, grpId)].size();
}
} // namespace sim

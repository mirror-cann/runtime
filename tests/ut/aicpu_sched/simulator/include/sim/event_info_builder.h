/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef SIM_EVENT_INFO_BUILDER_H
#define SIM_EVENT_INFO_BUILDER_H

#include <cstdint>
#include <securec.h>
#include <type_traits>
#include <vector>

#include "sim/esched_sim_engine.h"

namespace sim {
class EventInfoBuilder {
public:
    EventInfoBuilder& EventId(EVENT_ID id)
    {
        ev_.eventId = id;
        return *this;
    }
    EventInfoBuilder& SubeventId(uint32_t id)
    {
        ev_.subeventId = id;
        return *this;
    }
    EventInfoBuilder& Pid(int32_t pid)
    {
        ev_.pid = pid;
        return *this;
    }
    EventInfoBuilder& HostPid(int32_t hostPid)
    {
        ev_.hostPid = hostPid;
        return *this;
    }
    EventInfoBuilder& GrpId(uint32_t grpId)
    {
        ev_.grpId = grpId;
        return *this;
    }
    EventInfoBuilder& SubmitTs(uint64_t ts)
    {
        ev_.submitTs = ts;
        return *this;
    }
    EventInfoBuilder& SchedTs(uint64_t ts)
    {
        ev_.schedTs = ts;
        return *this;
    }
    EventInfoBuilder& Payload(const std::vector<char>& bytes)
    {
        ev_.msg = bytes;
        return *this;
    }
    EventInfoBuilder& PayloadRaw(const void* data, size_t len)
    {
        const char* p = static_cast<const char*>(data);
        ev_.msg.assign(p, p + len);
        return *this;
    }
    // 任意 POD 结构载荷
    template <typename T>
    EventInfoBuilder& PayloadStruct(const T& payload)
    {
        static_assert(std::is_trivially_copyable<T>::value, "payload must be trivially copyable");
        const char* p = reinterpret_cast<const char*>(&payload);
        ev_.msg.assign(p, p + sizeof(T));
        return *this;
    }
    // 截断载荷制造畸形 msg_len，验证模块解析校验
    EventInfoBuilder& TruncatePayload(size_t len)
    {
        if (len < ev_.msg.size()) {
            ev_.msg.resize(len);
        }
        return *this;
    }

    SimEvent Build() const { return ev_; }

    // 直接填充 event_info，免经引擎队列
    void BuildInto(event_info& out) const
    {
        out.comm.event_id = ev_.eventId;
        out.comm.subevent_id = ev_.subeventId;
        out.comm.pid = ev_.pid;
        out.comm.host_pid = ev_.hostPid;
        out.comm.grp_id = ev_.grpId;
        out.comm.submit_timestamp = ev_.submitTs;
        out.comm.sched_timestamp = ev_.schedTs;
        size_t len = ev_.msg.size();
        if (len > static_cast<size_t>(EVENT_MAX_MSG_LEN)) {
            len = static_cast<size_t>(EVENT_MAX_MSG_LEN);
        }
        out.priv.msg_len = static_cast<unsigned int>(len);
        if (len > 0U) {
            (void)memcpy_s(out.priv.msg, EVENT_MAX_MSG_LEN, ev_.msg.data(), len);
        }
    }

private:
    SimEvent ev_;
};
} // namespace sim

#endif // SIM_EVENT_INFO_BUILDER_H

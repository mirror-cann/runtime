/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <gtest/gtest.h>

#include "sim/sim_platform.h"

using namespace sim;

namespace {
class EschedSimEngineTest : public testing::Test {
protected:
    void SetUp() override { SimPlatform::Reset(); }
    void TearDown() override { SimPlatform::Reset(); }
    EschedSimEngine& Eng() { return SimPlatform::Esched(); }
};

TEST_F(EschedSimEngineTest, InjectThenWaitDeliversEventWithPayload)
{
    SimEvent ev = EventInfoBuilder()
                      .EventId(EVENT_TS_HWTS_KERNEL)
                      .SubeventId(42U)
                      .Pid(7)
                      .HostPid(8)
                      .Payload(HwtsTsTaskBuilder().MailboxId(0x55AAU).SerialNo(0x1122U).KernelName(0x1234U).Build())
                      .Build();
    Eng().InjectEvent(1U, 2U, ev);

    event_info out{};
    EXPECT_EQ(Eng().WaitEvent(1U, 2U, 0U, 100, &out), DRV_ERROR_NONE);
    EXPECT_EQ(out.comm.event_id, EVENT_TS_HWTS_KERNEL);
    EXPECT_EQ(out.comm.subevent_id, 42U);
    EXPECT_EQ(out.comm.pid, 7);
    EXPECT_EQ(out.comm.host_pid, 8);
    EXPECT_EQ(out.priv.msg_len, sizeof(hwts_ts_task));
    auto* task = reinterpret_cast<hwts_ts_task*>(out.priv.msg);
    EXPECT_EQ(task->mailbox_id, 0x55AAU);
    EXPECT_EQ(task->serial_no, 0x1122U);
    EXPECT_EQ(task->kernel_info.kernelName, 0x1234U);
}

TEST_F(EschedSimEngineTest, EmptyQueueReturnsNoEvent)
{
    event_info out{};
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 100, &out), DRV_ERROR_SCHED_NO_EVENT);
}

TEST_F(EschedSimEngineTest, NullEventReturnsParaError)
{
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 100, nullptr), DRV_ERROR_PARA_ERROR);
}

TEST_F(EschedSimEngineTest, ExitWhenDrainedReturnsProcessExit)
{
    Eng().SetExitWhenDrained(true);
    event_info out{};
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 0, &out), DRV_ERROR_SCHED_PROCESS_EXIT);
}

TEST_F(EschedSimEngineTest, DrainAllThenExit)
{
    for (int i = 0; i < 3; ++i) {
        Eng().InjectEvent(0U, 0U, EventInfoBuilder().EventId(EVENT_TS_HWTS_KERNEL).Pid(i).Build());
    }
    Eng().SetExitWhenDrained(true);
    event_info out{};
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 0, &out), DRV_ERROR_NONE);
        EXPECT_EQ(out.comm.pid, i);
    }
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 0, &out), DRV_ERROR_SCHED_PROCESS_EXIT);
}

TEST_F(EschedSimEngineTest, ForcedReturnSequence)
{
    Eng().SetWaitReturnSequence({DRV_ERROR_PARA_ERROR, DRV_ERROR_SCHED_NO_EVENT});
    event_info out{};
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 0, &out), DRV_ERROR_PARA_ERROR);
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 0, &out), DRV_ERROR_SCHED_NO_EVENT);
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 0, &out), DRV_ERROR_SCHED_NO_EVENT);
}

TEST_F(EschedSimEngineTest, SubmitCapturedViaStrongShim)
{
    event_summary es{};
    es.pid = 9;
    es.grp_id = 3U;
    es.event_id = EVENT_AICPU_MSG;
    es.subevent_id = 5U;
    char payload[4] = {1, 2, 3, 4};
    es.msg = payload;
    es.msg_len = 4U;
    EXPECT_EQ(halEschedSubmitEvent(1U, &es), DRV_ERROR_NONE);

    auto submitted = Eng().TakeSubmittedEvents();
    ASSERT_EQ(submitted.size(), 1U);
    EXPECT_EQ(submitted[0].pid, 9);
    EXPECT_EQ(submitted[0].eventId, EVENT_AICPU_MSG);
    EXPECT_EQ(submitted[0].subeventId, 5U);
    ASSERT_EQ(submitted[0].msg.size(), 4U);
    EXPECT_EQ(submitted[0].msg[0], 1);
    EXPECT_EQ(submitted[0].msg[3], 4);
}

TEST_F(EschedSimEngineTest, WaitViaStrongShim)
{
    Eng().InjectEvent(0U, 0U, EventInfoBuilder().EventId(EVENT_QS_MSG).Build());
    event_info out{};
    EXPECT_EQ(halEschedWaitEvent(0U, 0U, 0U, 100, &out), DRV_ERROR_NONE);
    EXPECT_EQ(out.comm.event_id, EVENT_QS_MSG);
}

TEST_F(EschedSimEngineTest, SubscriptionBitmapFiltering)
{
    Eng().SubscribeEvent(0U, 0U, 0U, (1ULL << static_cast<uint32_t>(EVENT_TS_HWTS_KERNEL)));
    Eng().InjectEvent(0U, 0U, EventInfoBuilder().EventId(EVENT_AICPU_MSG).Build());
    Eng().InjectEvent(0U, 0U, EventInfoBuilder().EventId(EVENT_TS_HWTS_KERNEL).Build());
    event_info out{};
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 100, &out), DRV_ERROR_NONE);
    EXPECT_EQ(out.comm.event_id, EVENT_TS_HWTS_KERNEL);
}

TEST_F(EschedSimEngineTest, SubmitLoopbackProducerConsumer)
{
    Eng().SetSubmitLoopback(true);
    event_summary es{};
    es.grp_id = 0U;
    es.event_id = EVENT_AICPU_MSG;
    es.msg = nullptr;
    es.msg_len = 0U;
    EXPECT_EQ(halEschedSubmitEvent(0U, &es), DRV_ERROR_NONE);
    event_info out{};
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 100, &out), DRV_ERROR_NONE);
    EXPECT_EQ(out.comm.event_id, EVENT_AICPU_MSG);
}

TEST_F(EschedSimEngineTest, BatchSubmit)
{
    event_summary batch[3] = {};
    for (int i = 0; i < 3; ++i) {
        batch[i].event_id = EVENT_TS_HWTS_KERNEL;
        batch[i].pid = i;
    }
    unsigned int succ = 0U;
    EXPECT_EQ(halEschedSubmitEventBatch(0U, SINGLE_EVENT_ENTRY, batch, 3U, &succ), DRV_ERROR_NONE);
    EXPECT_EQ(succ, 3U);
    EXPECT_EQ(Eng().TakeSubmittedEvents().size(), 3U);
}

TEST_F(EschedSimEngineTest, BatchSubmitSharedEntry)
{
    event_summary es{};
    es.event_id = EVENT_SPLIT_KERNEL;
    es.pid = 42;
    unsigned int succ = 0U;
    EXPECT_EQ(halEschedSubmitEventBatch(0U, SHARED_EVENT_ENTRY, &es, 4U, &succ), DRV_ERROR_NONE);
    EXPECT_EQ(succ, 4U);
    auto submitted = Eng().TakeSubmittedEvents();
    ASSERT_EQ(submitted.size(), 4U);
    for (const auto& s : submitted) {
        EXPECT_EQ(s.eventId, EVENT_SPLIT_KERNEL);
        EXPECT_EQ(s.pid, 42);
    }
}

TEST_F(EschedSimEngineTest, AckCapturedViaStrongShim)
{
    char rsp[2] = {7, 8};
    EXPECT_EQ(halEschedAckEvent(1U, EVENT_TS_HWTS_KERNEL, 99U, rsp, 2U), DRV_ERROR_NONE);
    auto acks = Eng().TakeAckRecords();
    ASSERT_EQ(acks.size(), 1U);
    EXPECT_EQ(acks[0].eventId, EVENT_TS_HWTS_KERNEL);
    EXPECT_EQ(acks[0].subeventId, 99U);
    ASSERT_EQ(acks[0].msg.size(), 2U);
    EXPECT_EQ(acks[0].msg[0], 7);
}

TEST_F(EschedSimEngineTest, AttachAndCreateGrpSucceed)
{
    EXPECT_EQ(halEschedAttachDevice(0U), DRV_ERROR_NONE);
    EXPECT_EQ(halEschedAttachDevice(0U), DRV_ERROR_NONE);
    EXPECT_EQ(halEschedCreateGrp(0U, 0U, GRP_TYPE_BIND_DP_CPU), DRV_ERROR_NONE);
    EXPECT_EQ(halEschedSubscribeEvent(0U, 0U, 0U, ~0ULL), DRV_ERROR_NONE);
    EXPECT_EQ(halEschedDettachDevice(0U), DRV_ERROR_NONE);
}

TEST_F(EschedSimEngineTest, MalformedPayloadShorterThanStruct)
{
    SimEvent ev = EventInfoBuilder()
                      .EventId(EVENT_AICPU_MSG)
                      .Payload(HwtsTsTaskBuilder().MailboxId(1U).Build())
                      .TruncatePayload(4U)
                      .Build();
    Eng().InjectEvent(0U, 0U, ev);
    event_info out{};
    EXPECT_EQ(Eng().WaitEvent(0U, 0U, 0U, 100, &out), DRV_ERROR_NONE);
    EXPECT_EQ(out.priv.msg_len, 4U);
}
} // namespace

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

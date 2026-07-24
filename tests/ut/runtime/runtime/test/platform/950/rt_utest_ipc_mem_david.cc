/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "driver/ascend_hal.h"
#include "securec.h"
#include "runtime/rt.h"
#include "runtime/event.h"
#include "runtime/mem.h"
#include "runtime/rts/rts_mem.h"
#include "runtime/rt_inner_mem.h"
#define private public
#define protected public
#include "runtime.hpp"
#include "api.hpp"
#include "api_impl.hpp"
#include "api_impl_david.hpp"
#include "context.hpp"
#include "raw_device.hpp"
#include "npu_driver.hpp"
#include "device_state_callback_manager.hpp"
#undef protected
#undef private

#include "api_error.hpp"
#include "event.hpp"
#include "stream.hpp"
#include "notify.hpp"
#include "count_notify.hpp"
#include "thread_local_container.hpp"
#include "../../rt_utest_config_define.hpp"
#include "stub/hal_stub.h"
#include "rt_utest_david_fixture_helper.h"

using namespace testing;
using namespace cce::runtime;

class IpcMemDavidTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {
        MOCKER(halGetDeviceInfo).stubs().will(invoke(stubDavidGetDeviceInfo));
        char* socVer = "Ascend950PR_9599";
        MOCKER(halGetSocVersion)
            .stubs()
            .with(mockcpp::any(), outBoundP(socVer, strlen("Ascend950PR_9599")), mockcpp::any())
            .will(returnValue(DRV_ERROR_NONE));
        std::cout << "IpcMemDavidTest SetUP" << std::endl;
        Runtime* rtInstance = (Runtime*)Runtime::Instance();
        rtInstance->SetDisableThread(true);
        originType_ = rtInstance->GetChipType();
        rtInstance->SetChipType(CHIP_DAVID);
        GlobalContainer::SetRtChipType(CHIP_DAVID);
        rtInstance->SetConnectUbFlag(true);
        std::cout << "IpcMemDavidTest start" << std::endl;
    }

    static void TearDownTestCase()
    {
        Runtime* rtInstance = (Runtime*)Runtime::Instance();
        rtInstance->SetChipType(originType_);
        GlobalContainer::SetRtChipType(originType_);
        rtInstance->SetDisableThread(false);
        rtInstance->SetConnectUbFlag(false);
        std::cout << "IpcMemDavidTest end" << std::endl;
    }

    virtual void SetUp()
    {
        GlobalMockObject::reset();
        Driver* driver = MockDavidDriverSetup();

        MOCKER_CPP_VIRTUAL(driver, &Driver::GetRunMode).stubs().will(returnValue((uint32_t)RT_RUN_MODE_ONLINE));

        bool enable = false;
        MOCKER_CPP_VIRTUAL(driver, &Driver::GetSqEnable)
            .stubs()
            .with(mockcpp::any(), mockcpp::any(), mockcpp::any(), outBound(enable))
            .will(returnValue(RT_ERROR_NONE));

        MOCKER_CPP_VIRTUAL(driver, &Driver::EnableSq).stubs().will(returnValue(RT_ERROR_NONE));
        SetupDavidDeviceAndEngine(device_, engine_);

        rtError_t res = rtSetDevice(0);
        EXPECT_EQ(res, RT_ERROR_NONE);
    }

    virtual void TearDown()
    {
        rtError_t res = rtDeviceReset(0);
        EXPECT_EQ(res, RT_ERROR_NONE);

        // Clean up IPC memory name map for test isolation
        std::unordered_map<std::string, ipcMemInfoV2_t>& ipcMemNameVaMap = Runtime::Instance()->GetIpcMemNameVaMap();
        std::mutex& ipcMemNameLock = Runtime::Instance()->GetIpcMemNameLock();
        ipcMemNameLock.lock();
        ipcMemNameVaMap.clear();
        ipcMemNameLock.unlock();

        GlobalMockObject::verify();
    }

    static rtChipType_t originType_;
    static Device* device_;
    static Engine* engine_;
};

rtChipType_t IpcMemDavidTest::originType_ = CHIP_DAVID;
Device* IpcMemDavidTest::device_ = nullptr;
Engine* IpcMemDavidTest::engine_ = nullptr;

TEST_F(IpcMemDavidTest, IpcMemNameVaMap_set_attr_and_import_simulation)
{
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    // Mock CheckIpcMapRoute for IPC V2
    Driver* driver = device_->Driver_();
    MOCKER_CPP_VIRTUAL(driver, &Driver::CheckIpcMapRoute).stubs().will(returnValue(RT_ERROR_NONE));

    std::unordered_map<std::string, ipcMemInfoV2_t>& ipcMemNameVaMap = Runtime::Instance()->GetIpcMemNameVaMap();
    std::mutex& ipcMemNameLock = Runtime::Instance()->GetIpcMemNameLock();

    const char* ipcName = "test_ipc_rt_interfaces";
    uint64_t expectedAttrs[] = {1, 2, 3};
    void* devPtr = nullptr;

    for (int round = 0; round < 3; round++) {
        error = rtIpcSetMemoryAttr(ipcName, 0, expectedAttrs[round]);
        EXPECT_EQ(error, RT_ERROR_NONE);

        ipcMemNameLock.lock();
        auto it = ipcMemNameVaMap.find(std::string(ipcName));
        EXPECT_NE(it, ipcMemNameVaMap.end());
        EXPECT_EQ(it->second.latestAttr, expectedAttrs[round]);
        ipcMemNameLock.unlock();

        error = rtsIpcMemImportByKey(&devPtr, ipcName, 0);
        EXPECT_EQ(error, RT_ERROR_NONE);

        ipcMemNameLock.lock();
        it = ipcMemNameVaMap.find(std::string(ipcName));
        EXPECT_NE(it, ipcMemNameVaMap.end());
        EXPECT_EQ(it->second.latestAttr, expectedAttrs[round]);
        ipcMemNameLock.unlock();
    }

    ipcMemNameLock.lock();
    auto finalIt = ipcMemNameVaMap.find(std::string(ipcName));
    EXPECT_NE(finalIt, ipcMemNameVaMap.end());
    EXPECT_EQ(finalIt->second.vaInfo.size(), 3);
    EXPECT_EQ(finalIt->second.vaInfo[2].first, expectedAttrs[2]);
    ipcMemNameLock.unlock();

    error = rtsIpcMemClose(ipcName);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    auto afterCloseIt = ipcMemNameVaMap.find(std::string(ipcName));
    EXPECT_EQ(afterCloseIt, ipcMemNameVaMap.end()) << "IPC name should be removed from map after close";
    ipcMemNameLock.unlock();

    error = rtDeviceReset(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(IpcMemDavidTest, IpcMemNameVaMap_set_attr_once_import_multiple_times)
{
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    // Mock CheckIpcMapRoute for IPC V2
    Driver* driver = device_->Driver_();
    MOCKER_CPP_VIRTUAL(driver, &Driver::CheckIpcMapRoute).stubs().will(returnValue(RT_ERROR_NONE));

    std::unordered_map<std::string, ipcMemInfoV2_t>& ipcMemNameVaMap = Runtime::Instance()->GetIpcMemNameVaMap();
    std::mutex& ipcMemNameLock = Runtime::Instance()->GetIpcMemNameLock();

    const char* ipcName = "test_ipc_set_once_import_multi";
    uint64_t expectedAttr = 1;
    void* devPtr = nullptr;

    error = rtIpcSetMemoryAttr(ipcName, 0, expectedAttr);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    auto it = ipcMemNameVaMap.find(std::string(ipcName));
    EXPECT_NE(it, ipcMemNameVaMap.end());
    EXPECT_EQ(it->second.latestAttr, expectedAttr);
    EXPECT_EQ(it->second.vaInfo.size(), 0);
    ipcMemNameLock.unlock();

    for (int round = 0; round < 5; round++) {
        error = rtsIpcMemImportByKey(&devPtr, ipcName, 0);
        EXPECT_EQ(error, RT_ERROR_NONE);

        ipcMemNameLock.lock();
        it = ipcMemNameVaMap.find(std::string(ipcName));
        EXPECT_NE(it, ipcMemNameVaMap.end());
        EXPECT_EQ(it->second.latestAttr, expectedAttr);
        EXPECT_EQ(it->second.vaInfo.size(), round + 1);
        EXPECT_EQ(it->second.vaInfo[round].first, expectedAttr);
        ipcMemNameLock.unlock();
    }

    ipcMemNameLock.lock();
    auto finalIt = ipcMemNameVaMap.find(std::string(ipcName));
    EXPECT_NE(finalIt, ipcMemNameVaMap.end());
    EXPECT_EQ(finalIt->second.latestAttr, expectedAttr);
    EXPECT_EQ(finalIt->second.vaInfo.size(), 5);
    for (size_t i = 0; i < finalIt->second.vaInfo.size(); i++) {
        EXPECT_EQ(finalIt->second.vaInfo[i].first, expectedAttr) << "vaInfo[" << i << "] attr mismatch";
    }
    ipcMemNameLock.unlock();

    error = rtsIpcMemClose(ipcName);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    EXPECT_EQ(ipcMemNameVaMap.find(std::string(ipcName)), ipcMemNameVaMap.end());
    ipcMemNameLock.unlock();

    error = rtDeviceReset(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(IpcMemDavidTest, IpcMemNameVaMap_import_without_set_attr_default_zero)
{
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    std::unordered_map<std::string, ipcMemInfoV2_t>& ipcMemNameVaMap = Runtime::Instance()->GetIpcMemNameVaMap();
    std::mutex& ipcMemNameLock = Runtime::Instance()->GetIpcMemNameLock();

    const char* ipcName = "test_ipc_import_without_set";
    uint64_t expectedDefaultAttr = 0;
    void* devPtr = nullptr;

    ipcMemNameLock.lock();
    auto preCheck = ipcMemNameVaMap.find(std::string(ipcName));
    EXPECT_EQ(preCheck, ipcMemNameVaMap.end());
    ipcMemNameLock.unlock();

    for (int round = 0; round < 5; round++) {
        error = rtsIpcMemImportByKey(&devPtr, ipcName, 0);
        EXPECT_EQ(error, RT_ERROR_NONE);

        ipcMemNameLock.lock();
        auto it = ipcMemNameVaMap.find(std::string(ipcName));
        EXPECT_NE(it, ipcMemNameVaMap.end());
        EXPECT_EQ(it->second.latestAttr, expectedDefaultAttr);
        EXPECT_EQ(it->second.vaInfo.size(), round + 1);
        EXPECT_EQ(it->second.vaInfo[round].first, expectedDefaultAttr);
        ipcMemNameLock.unlock();
    }

    ipcMemNameLock.lock();
    auto finalIt = ipcMemNameVaMap.find(std::string(ipcName));
    EXPECT_NE(finalIt, ipcMemNameVaMap.end());
    EXPECT_EQ(finalIt->second.latestAttr, expectedDefaultAttr);
    EXPECT_EQ(finalIt->second.vaInfo.size(), 5);
    for (size_t i = 0; i < finalIt->second.vaInfo.size(); i++) {
        EXPECT_EQ(finalIt->second.vaInfo[i].first, expectedDefaultAttr)
            << "vaInfo[" << i << "] attr should be default 0";
    }
    ipcMemNameLock.unlock();

    error = rtsIpcMemClose(ipcName);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    auto closeIt = ipcMemNameVaMap.find(std::string(ipcName));
    EXPECT_EQ(closeIt, ipcMemNameVaMap.end());
    ipcMemNameLock.unlock();

    error = rtDeviceReset(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(IpcMemDavidTest, IpcMemNameVaMap_import_multiple_ipc_default_attr)
{
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    std::unordered_map<std::string, ipcMemInfoV2_t>& ipcMemNameVaMap = Runtime::Instance()->GetIpcMemNameVaMap();
    std::mutex& ipcMemNameLock = Runtime::Instance()->GetIpcMemNameLock();

    uint64_t expectedDefaultAttr = 0;
    void* devPtr1 = nullptr;
    void* devPtr2 = nullptr;

    error = rtsIpcMemImportByKey(&devPtr1, "test_ipc_default_1", 0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    auto it1 = ipcMemNameVaMap.find("test_ipc_default_1");
    EXPECT_NE(it1, ipcMemNameVaMap.end());
    EXPECT_EQ(it1->second.latestAttr, expectedDefaultAttr);
    EXPECT_EQ(it1->second.vaInfo.size(), 1);
    ipcMemNameLock.unlock();

    error = rtsIpcMemImportByKey(&devPtr2, "test_ipc_default_2", 0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    auto it2 = ipcMemNameVaMap.find("test_ipc_default_2");
    EXPECT_NE(it2, ipcMemNameVaMap.end());
    EXPECT_EQ(it2->second.latestAttr, expectedDefaultAttr);
    EXPECT_EQ(ipcMemNameVaMap.size(), 2);
    ipcMemNameLock.unlock();

    error = rtDeviceReset(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(IpcMemDavidTest, IpcMemNameVaMap_set_attr_and_import_multiple_ipc)
{
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    Driver* driver = device_->Driver_();
    MOCKER_CPP_VIRTUAL(driver, &Driver::CheckIpcMapRoute).stubs().will(returnValue(RT_ERROR_NONE));

    std::unordered_map<std::string, ipcMemInfoV2_t>& ipcMemNameVaMap = Runtime::Instance()->GetIpcMemNameVaMap();
    std::mutex& ipcMemNameLock = Runtime::Instance()->GetIpcMemNameLock();

    void* devPtr3 = nullptr;
    void* devPtr4 = nullptr;

    error = rtIpcSetMemoryAttr("test_ipc_attr_3", 0, 1);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    auto it3 = ipcMemNameVaMap.find("test_ipc_attr_3");
    EXPECT_NE(it3, ipcMemNameVaMap.end());
    EXPECT_EQ(it3->second.latestAttr, 1);
    EXPECT_EQ(it3->second.vaInfo.size(), 0);
    ipcMemNameLock.unlock();

    error = rtsIpcMemImportByKey(&devPtr3, "test_ipc_attr_3", 0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    error = rtIpcSetMemoryAttr("test_ipc_attr_4", 0, 2);
    EXPECT_EQ(error, RT_ERROR_NONE);

    error = rtsIpcMemImportByKey(&devPtr4, "test_ipc_attr_4", 0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    EXPECT_EQ(ipcMemNameVaMap.size(), 2);
    ipcMemNameLock.unlock();

    error = rtDeviceReset(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(IpcMemDavidTest, IpcMemNameVaMap_close_ipc_entries_sequentially)
{
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    Driver* driver = device_->Driver_();
    MOCKER_CPP_VIRTUAL(driver, &Driver::CheckIpcMapRoute).stubs().will(returnValue(RT_ERROR_NONE));

    std::unordered_map<std::string, ipcMemInfoV2_t>& ipcMemNameVaMap = Runtime::Instance()->GetIpcMemNameVaMap();
    std::mutex& ipcMemNameLock = Runtime::Instance()->GetIpcMemNameLock();

    void* devPtr1 = nullptr;
    void* devPtr2 = nullptr;
    void* devPtr3 = nullptr;
    void* devPtr4 = nullptr;

    error = rtsIpcMemImportByKey(&devPtr1, "ipc_close_test_1", 0);
    EXPECT_EQ(error, RT_ERROR_NONE);
    error = rtsIpcMemImportByKey(&devPtr2, "ipc_close_test_2", 0);
    EXPECT_EQ(error, RT_ERROR_NONE);
    error = rtIpcSetMemoryAttr("ipc_close_test_3", 0, 1);
    EXPECT_EQ(error, RT_ERROR_NONE);
    error = rtsIpcMemImportByKey(&devPtr3, "ipc_close_test_3", 0);
    EXPECT_EQ(error, RT_ERROR_NONE);
    error = rtIpcSetMemoryAttr("ipc_close_test_4", 0, 2);
    EXPECT_EQ(error, RT_ERROR_NONE);
    error = rtsIpcMemImportByKey(&devPtr4, "ipc_close_test_4", 0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    EXPECT_EQ(ipcMemNameVaMap.size(), 4);
    ipcMemNameLock.unlock();

    error = rtsIpcMemClose("ipc_close_test_1");
    EXPECT_EQ(error, RT_ERROR_NONE);

    error = rtsIpcMemClose("ipc_close_test_2");
    EXPECT_EQ(error, RT_ERROR_NONE);

    error = rtsIpcMemClose("ipc_close_test_3");
    EXPECT_EQ(error, RT_ERROR_NONE);

    error = rtsIpcMemClose("ipc_close_test_4");
    EXPECT_EQ(error, RT_ERROR_NONE);

    ipcMemNameLock.lock();
    EXPECT_EQ(ipcMemNameVaMap.find("ipc_close_test_4"), ipcMemNameVaMap.end());
    EXPECT_EQ(ipcMemNameVaMap.size(), 0);
    ipcMemNameLock.unlock();

    error = rtDeviceReset(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(IpcMemDavidTest, IpcMemNameVaMap_close_memory)
{
    const char* ipcName = "aaa";
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    std::unordered_map<std::string, ipcMemInfoV2_t>& ipcMemNameVaMap = Runtime::Instance()->GetIpcMemNameVaMap();
    std::mutex& ipcMemNameLock = Runtime::Instance()->GetIpcMemNameLock();

    uint64_t va = 1;
    uint64_t attr = 2;
    ipcMemNameLock.lock();
    ipcMemNameVaMap.emplace(ipcName, ipcMemInfoV2_t{attr, {{attr, va}}});
    ipcMemNameLock.unlock();

    error = rtIpcCloseMemory(RtValueToPtr<void*>(va));
    EXPECT_EQ(error, RT_ERROR_NONE);

    error = rtDeviceReset(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}

TEST_F(IpcMemDavidTest, close_memory_with_no_va)
{
    const char* ipcName = "aaa";
    rtError_t error = rtSetDevice(0);
    EXPECT_EQ(error, RT_ERROR_NONE);

    uint64_t va = 1;
    error = rtIpcCloseMemory(RtValueToPtr<void*>(va));
    EXPECT_EQ(error, RT_ERROR_NONE);

    error = rtDeviceReset(0);
    EXPECT_EQ(error, RT_ERROR_NONE);
}
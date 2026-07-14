/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <gtest/gtest.h>
#include <fstream>
#include <functional>
#include <thread>
#include "mockcpp/mockcpp.hpp"
#include "acl_dump.h"
#include "adump_api.h"

using namespace Adx;

namespace {
// 文件作用域静态函数作为回调，静态存储期，避免局部 lambda 的生命周期歧义。
// 注意：AdumpRegisterCallback 无对应反注册接口，回调会残留在 DumpManager 单例中，
// 使用静态函数可保证即使后续被触发指针依然有效。
int32_t DumpCallbackStub(uint64_t, const char *, int32_t)
{
    return 0;
}
}

class TinyAdumpApiPlatformUtest : public testing::Test {
protected:
    virtual void SetUp() {}
    virtual void TearDown()
    {
        GlobalMockObject::verify();
    }
};

TEST_F(TinyAdumpApiPlatformUtest, Test_AdumpGetSizeInfoAddr_OverMax)
{
    uint32_t atomicIndex = 0;
    EXPECT_EQ(AdumpGetSizeInfoAddr(MAX_TENSOR_NUM + 1, atomicIndex), nullptr);
}

TEST_F(TinyAdumpApiPlatformUtest, Test_AdumpGetSizeInfoAddr_Valid)
{
    uint32_t atomicIndex = 0;
    void *addr = AdumpGetSizeInfoAddr(1, atomicIndex);
    EXPECT_NE(addr, nullptr);

    void *addr2 = AdumpGetSizeInfoAddr(MAX_TENSOR_NUM, atomicIndex);
    EXPECT_NE(addr2, nullptr);
}

TEST_F(TinyAdumpApiPlatformUtest, Test_AdumpRegisterCallback_NullFunc)
{
    EXPECT_EQ(AdumpRegisterCallback(1, nullptr, nullptr), ADUMP_FAILED);
}

TEST_F(TinyAdumpApiPlatformUtest, Test_AdumpRegisterCallback_Success)
{
    EXPECT_EQ(AdumpRegisterCallback(1, DumpCallbackStub, DumpCallbackStub), ADUMP_SUCCESS);
}

TEST_F(TinyAdumpApiPlatformUtest, Test_acldumpGetPath)
{
    EXPECT_EQ(acldumpGetPath(DATA_DUMP), nullptr);
}

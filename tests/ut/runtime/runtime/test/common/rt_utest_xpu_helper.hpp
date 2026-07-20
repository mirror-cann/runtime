/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef TESTS_UT_RUNTIME_RUNTIME_TEST_COMMON_RT_UTEST_XPU_HELPER_HPP_
#define TESTS_UT_RUNTIME_RUNTIME_TEST_COMMON_RT_UTEST_XPU_HELPER_HPP_

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "runtime/rt.h"
#include "tprt_api.h"
#include "tprt_error_code.h"

namespace cce {
namespace runtime {
namespace ut {

inline uint32_t TprtOpSqCqInfoMock(uint32_t devId, TprtSqCqOpInfo_t* opInfo)
{
    (void)devId;
    if (opInfo != nullptr) {
        opInfo->value[0] = 1U;
    }
    return TPRT_SUCCESS;
}

inline void MockXpuTprtRuntime()
{
    MOCKER(TprtDeviceOpen).stubs().will(returnValue(static_cast<uint32_t>(RT_ERROR_NONE)));
    MOCKER(TprtDeviceClose).stubs().will(returnValue(static_cast<uint32_t>(RT_ERROR_NONE)));
    MOCKER(TprtSqCqCreate).stubs().will(returnValue(static_cast<uint32_t>(TPRT_SUCCESS)));
    MOCKER(TprtSqCqDestroy).stubs().will(returnValue(static_cast<uint32_t>(TPRT_SUCCESS)));
    MOCKER(TprtSqPushTask).stubs().will(returnValue(static_cast<uint32_t>(TPRT_SUCCESS)));
    MOCKER(TprtOpSqCqInfo).stubs().will(invoke(TprtOpSqCqInfoMock));
}

class XpuRuntimeMockTest : public testing::Test {
protected:
    static void MockTprtRuntime() { MockXpuTprtRuntime(); }

    void SetUp() override { MockTprtRuntime(); }

    void TearDown() override { GlobalMockObject::verify(); }
};

} // namespace ut
} // namespace runtime
} // namespace cce

#endif // TESTS_UT_RUNTIME_RUNTIME_TEST_COMMON_RT_UTEST_XPU_HELPER_HPP_

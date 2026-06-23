/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <cstring>
#include "exception_info_common.h"
#include "kernel_symbol_locator.h"
#include "runtime/rts/rts_kernel.h"
#include "adump_pub.h"
#include "str_utils.h"
#include "log/adx_log.h"
#include "log/hdc_log.h"

namespace Adx {
namespace {
constexpr char MIX_AIC_SUFFIX[] = "_mix_aic";
constexpr char MIX_AIV_SUFFIX[] = "_mix_aiv";

const char* GetKernelMixSuffix(KernelMixType kernelMixType)
{
    switch (kernelMixType) {
        case KernelMixType::AIC:
            return MIX_AIC_SUFFIX;
        case KernelMixType::AIV:
            return MIX_AIV_SUFFIX;
        default:
            return nullptr;
    }
}
}  // namespace

int32_t ExceptionInfoCommon::GetExceptionInfo(const rtExceptionInfo &exception,
                                              rtExceptionArgsInfo_t &exceptionArgsInfo)
{
    rtExceptionExpandType_t exceptionTaskType = exception.expandInfo.type;
    if (exceptionTaskType == RT_EXCEPTION_AICORE) {
        exceptionArgsInfo = exception.expandInfo.u.aicoreInfo.exceptionArgs;
    } else if (exceptionTaskType == RT_EXCEPTION_FFTS_PLUS) {
        exceptionArgsInfo = exception.expandInfo.u.fftsPlusInfo.exceptionArgs;
    } else if (exceptionTaskType == RT_EXCEPTION_FUSION) {
        exceptionArgsInfo = exception.expandInfo.u.fusionInfo.u.aicoreCcuInfo.exceptionArgs;
    } else {
        IDE_LOGW("Exception type[%d(%s)] is not supported.", static_cast<int32_t>(exceptionTaskType),
            GetExceptionTaskTypeName(exception).c_str());
        return ADUMP_FAILED;
    }

    return ADUMP_SUCCESS;
}

std::string ExceptionInfoCommon::GetExceptionTaskTypeName(const rtExceptionInfo &exception)
{
    switch (exception.expandInfo.type) {
        case RT_EXCEPTION_INVALID:
            return "invalid";
        case RT_EXCEPTION_FFTS_PLUS:
            return "fftsplus";
        case RT_EXCEPTION_AICORE:
            return "aicore";
        case RT_EXCEPTION_UB:
            return "ub";
        case RT_EXCEPTION_CCU:
            return "ccu";
        case RT_EXCEPTION_FUSION:
            return "fusion";
        default:
            return "unknown";
    }
}

std::string ExceptionInfoCommon::GetKernelNameWithoutMixSuffix(const std::string &kernelName)
{
    const char *suffix = GetKernelMixSuffix(GetKernelMixType(kernelName));
    if (suffix == nullptr) {
        return kernelName;
    }

    std::string processedKernelName = kernelName;
    const size_t suffixLen = std::strlen(suffix);
    processedKernelName.resize(processedKernelName.size() - suffixLen);
    return processedKernelName;
}

std::string ExceptionInfoCommon::GetExceptionKernelName(const rtExceptionInfo &exception)
{
    rtExceptionArgsInfo_t exceptionArgsInfo{};
    if (GetExceptionInfo(exception, exceptionArgsInfo) != ADUMP_SUCCESS) {
        return "";
    }

    const rtExceptionKernelInfo_t &kernelInfo = exceptionArgsInfo.exceptionKernelInfo;
    if (kernelInfo.kernelName == nullptr || kernelInfo.kernelNameSize == 0) {
        return "";
    }

    const std::string kernelName(kernelInfo.kernelName, kernelInfo.kernelNameSize);
    return GetKernelNameWithoutMixSuffix(kernelName);
}

int32_t ExceptionInfoCommon::GetKernelDeviceAddr(rtBinHandle binHandle, void * &devAddr)
{
    devAddr = nullptr;
    uint32_t binSize = 0;
    IDE_CTRL_VALUE_WARN(binHandle != nullptr, return ADUMP_FAILED, "bindHandle is nullptr.");
    rtError_t ret = rtsBinaryGetDevAddress(binHandle, &devAddr, &binSize);
    IDE_CTRL_VALUE_FAILED(ret == RT_ERROR_NONE && devAddr != nullptr, return ADUMP_FAILED,
        "rtsBinaryGetDevAddress failed, ret=%d, binHandle=%p, devAddr=%p.",
        static_cast<int32_t>(ret), binHandle, devAddr);

    IDE_LOGI("The device address of binHandle(%p) is %p, binSize=%u.", binHandle, devAddr, binSize);
    return ADUMP_SUCCESS;
}

KernelMixType ExceptionInfoCommon::GetKernelMixType(const std::string &kernelName)
{
    if (kernelName.size() > std::strlen(MIX_AIC_SUFFIX) && StrUtils::EndsWith(kernelName, MIX_AIC_SUFFIX)) {
        return KernelMixType::AIC;
    }
    if (kernelName.size() > std::strlen(MIX_AIV_SUFFIX) && StrUtils::EndsWith(kernelName, MIX_AIV_SUFFIX)) {
        return KernelMixType::AIV;
    }
    return KernelMixType::NONE;
}

int32_t ExceptionInfoCommon::GetExceptionRegInfo(const rtExceptionInfo &exception, ExceptionRegInfo &exceptionRegInfo)
{
    rtError_t rtRet = rtGetExceptionRegInfo(&exception, &exceptionRegInfo.errRegInfo, &exceptionRegInfo.coreNum);
    IDE_CTRL_VALUE_FAILED(rtRet == RT_ERROR_NONE, return ADUMP_FAILED,
        "rtGetExceptionRegInfo failed. ret: %d", static_cast<int32_t>(rtRet));

    IDE_LOGI("Get exception register information. coreNum=%u", exceptionRegInfo.coreNum);
    return ADUMP_SUCCESS;
}

int32_t ExceptionInfoCommon::GetBinDataFromHandle(rtBinHandle binHandle, std::string &binData, uint32_t &binSize)
{
    IDE_CTRL_VALUE_FAILED(binHandle != nullptr, return ADUMP_FAILED, "binHandle is null");

    void *bufAddr = nullptr;
    uint32_t bufSize = 0;
    rtError_t ret = rtGetBinBuffer(binHandle, RT_BIN_HOST_ADDR, &bufAddr, &bufSize);
    IDE_CTRL_VALUE_FAILED(ret == RT_ERROR_NONE && bufAddr != nullptr && bufSize != 0, return ADUMP_FAILED,
        "rtGetBinBuffer failed, ret=%d, binHandle=%p, bufAddr=%p, bufSize=%u.",
        static_cast<int32_t>(ret), binHandle, bufAddr, bufSize);

    IDE_LOGI("Get kernel bin data success. binHandle=%p, bufAddr=%p, bufSize=%u", binHandle, bufAddr, bufSize);
    binData.assign(reinterpret_cast<char*>(bufAddr), bufSize);
    binSize = bufSize;
    return ADUMP_SUCCESS;
}

}  // namespace Adx

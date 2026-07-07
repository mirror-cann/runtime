/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// tsd 仿真桩：模拟 tsdaemon 与 aicpu_sched 之间的交互。
// WaitForShutDown 阻塞等待通知，其余接口为空操作。

#include "sim_tsd.h"

#include "tsd.h"

void SimNotifyShutdown() { sim::TsdSimEngine::Instance().NotifyShutdown(); }

int32_t TsdWaitForShutdown(
    const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId)
{
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    return 0;
}

int32_t WaitForShutDown(const uint32_t deviceId) { return sim::TsdSimEngine::Instance().WaitForShutdown(deviceId); }

int32_t TsdDestroy(const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    return 0;
}

int32_t TsdHeartbeatSend(const uint32_t deviceId, const TsdWaitType waitType)
{
    (void)deviceId;
    (void)waitType;
    return 0;
}

int32_t CreateOrFindCustPid(
    const uint32_t deviceId, const uint32_t loadLibNum, const char* const loadLibName[], const uint32_t hostPid,
    const uint32_t vfId, const char* const groupNameList, const uint32_t groupNameNum, int32_t* const custProcPid,
    bool* const firstStart)
{
    (void)deviceId;
    (void)loadLibNum;
    (void)loadLibName;
    (void)hostPid;
    (void)vfId;
    (void)groupNameList;
    (void)groupNameNum;
    if (custProcPid != nullptr) {
        *custProcPid = 0;
    }
    if (firstStart != nullptr) {
        *firstStart = false;
    }
    return 0;
}

int32_t StartupResponse(
    const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    return 0;
}

int32_t TsdReportStartOrStopErrCode(
    const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId,
    const char* errCode, const uint32_t errLen)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    (void)errCode;
    (void)errLen;
    return 0;
}

int32_t ReportMsgToTsd(
    const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId,
    const char* const msgInfo)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    (void)msgInfo;
    return 0;
}

int32_t RegEventMsgCallBackFunc(const struct SubProcEventCallBackInfo* regInfo)
{
    (void)regInfo;
    return 0;
}

void UnRegEventMsgCallBackFunc(const uint32_t eventType) { (void)eventType; }

int32_t SubModuleProcessResponse(
    const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId,
    const uint32_t eventType)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    (void)eventType;
    return 0;
}

int32_t StopWaitForCustAicpu() { return 0; }

int32_t SetDstTsdEventPid(const uint32_t dstPid)
{
    (void)dstPid;
    return 0;
}

int32_t SendUpdateProfilingRspToTsd(
    const uint32_t deviceId, const uint32_t waitType, const uint32_t hostPid, const uint32_t vfId)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    return 0;
}

int32_t SetSubProcScheduleMode(
    const uint32_t deviceId, const uint32_t waitType, const uint32_t hostPid, const uint32_t vfId,
    const struct SubProcScheduleModeInfo* scheInfo)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    (void)scheInfo;
    return 0;
}

int32_t SendStartUpFinishMsg(
    const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    return 0;
}

int32_t ReportProcessStartUpErrorCode(
    const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId,
    const char* errCode, const uint32_t errLen)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    (void)errCode;
    (void)errLen;
    return 0;
}

int32_t DestroySubProcess(
    const uint32_t deviceId, const TsdWaitType waitType, const uint32_t hostPid, const uint32_t vfId)
{
    (void)deviceId;
    (void)waitType;
    (void)hostPid;
    (void)vfId;
    return 0;
}

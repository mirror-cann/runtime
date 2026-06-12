/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CCE_RUNTIME_BASE_H
#define CCE_RUNTIME_BASE_H

#include <stdbool.h>
#include <stdint.h>
#include "profiling/prof_api.h"
#include "runtime/rt_external_base.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum ErrRegInfoIdxV100 {
    RT_V100_AIC_ERR_0 = 0,
    RT_V100_AIC_ERR_1,
    RT_V100_AIC_ERR_2,
    RT_V100_AIC_ERR_3,
    RT_V100_AIC_ERR_4,
    RT_V100_AIC_ERR_5,
    RT_V100_BIU_ERR_0,
    RT_V100_BIU_ERR_1,
    RT_V100_CCU_ERR_0,
    RT_V100_CCU_ERR_1,
    RT_V100_CUBE_ERR_0,
    RT_V100_CUBE_ERR_1,
    RT_V100_IFU_ERR_0,
    RT_V100_IFU_ERR_1,
    RT_V100_MTE_ERR_0,
    RT_V100_MTE_ERR_1,
    RT_V100_VEC_ERR_0,
    RT_V100_VEC_ERR_1,
    RT_V100_FIXP_ERR_0,
    RT_V100_FIXP_ERR_1,
    RT_V100_AIC_COND_0, 
    RT_V100_AIC_COND_1
} rtErrRegInfoIdxV100_t;

/**
 * @ingroup stream_capture_mode
 * @brief stream capture mode
 */
typedef enum tagRtStreamCaptureMode {
    RT_STREAM_CAPTURE_MODE_GLOBAL       = 0,
    RT_STREAM_CAPTURE_MODE_THREAD_LOCAL = 1,
    RT_STREAM_CAPTURE_MODE_RELAXED      = 2,

    RT_STREAM_CAPTURE_MODE_MAX
} rtStreamCaptureMode;

#define RT_ERR_REG_NUMS  (64U)
typedef struct rtExceptionErrRegInfo {
    uint32_t coreId;
    rtCoreType_t coreType;
    uint64_t startPC;
    uint64_t currentPC;
    uint32_t errReg[RT_ERR_REG_NUMS];
} rtExceptionErrRegInfo_t;

/**
 * @ingroup dvrt_base
 * @brief task handle.
 */
typedef void *rtTask_t;

typedef void (*rtCallback_t)(void *fnData);

typedef enum {
    RT_UTIL_TYPE_AICORE = 0,
    RT_UTIL_TYPE_AIVECTOR,
    RT_UTIL_TYPE_AICPU,
    RT_UTIL_TYPE_MAX
} rtTypeUtil_t;

typedef enum {
    RT_DEVICE_ABORT = 0,
    RT_DEVICE_KILL,
    RT_DEVICE_CLEAN,
    RT_DEVICE_ABORT_PRE,
    RT_DEVICE_ABORT_POST,
} rtTaskAbortStage_t;

typedef int32_t (*rtTaskAbortCallBack)(uint32_t devId, rtTaskAbortStage_t stage, uint32_t timeout, void *args);

/**
 * @ingroup profiling_base
 * @brief runtime handle.
 */
RTS_API rtError_t rtSetProfDirEx(const char_t *profDir, const char_t *address, const char_t *jobCtx);

/**
 * @ingroup profiling_base
 * @brief init profiler object.
 */
RTS_API rtError_t rtProfilerInit(const char_t *profDir, const char_t *address, const char_t *jobCtx);

/**
 * @ingroup profiling_base
 * @brief config rts profiler.
 */
RTS_API rtError_t rtProfilerConfig(uint16_t profConfig);

/**
 * @ingroup profiling_base
 * @brief ts send keypoint profiler log.
 */
RTS_API rtError_t rtProfilerTrace(uint64_t id, bool notify, uint32_t flags, rtStream_t stm);

/**
 * @ingroup profiling_base
 * @brief ts send keypoint profiler log.
 */
RTS_API rtError_t rtProfilerTraceEx(uint64_t id, uint64_t modelId, uint16_t tagId, rtStream_t stm);

/**
 * @ingroup profiling_base
 * @brief add the map of deviceId and GE model index, called by ge
 * @param [in] geModelIdx  The index of GE model
 * @param [in] deviceId    The id of device
 * @return RT_ERROR_NONE for ok
 * @return ACL_ERROR_RT_PARAM_INVALID for error input
 */
RTS_API rtError_t rtSetDeviceIdByGeModelIdx(uint32_t geModelIdx, uint32_t deviceId);

/**
 * @ingroup profiling_base
 * @brief del the map of deviceId and GE model index, called by ge
 * @param [in] geModelIdx  The index of GE model
 * @param [in] deviceId    The id of device
 * @return RT_ERROR_NONE for ok
 * @return ACL_ERROR_RT_PARAM_INVALID for error input
 */
RTS_API rtError_t rtUnsetDeviceIdByGeModelIdx(uint32_t geModelIdx, uint32_t deviceId);

/**
 * @ingroup profiling_base
 * @brief find deviceId by GE model index, called by profiling
 * @param [in]  geModelIdx  The index of GE model
 * @param [out] deviceId    The id of device
 * @return RT_ERROR_NONE for ok
 * @return ACL_ERROR_RT_PARAM_INVALID for error input
 * @return ACL_ERROR_RT_INTERNAL_ERROR for can't find deviceId by geModelIdx
 */
RTS_API rtError_t rtGetDeviceIdByGeModelIdx(uint32_t geModelIdx, uint32_t *deviceId);

/**
 * @ingroup profiling_base
 * @brief set profling switch, called by profiling
 * @param [in]  data  rtProfilingCommandHandle
 * @param [in]  len   length of data
 * @return RT_ERROR_NONE for ok
 * @return ACL_ERROR_RT_PARAM_INVALID for error input
 */
RTS_API rtError_t rtProfilingCommandHandle(uint32_t type, void *data, uint32_t len);

/**
 * @ingroup dvrt_base
 * @brief register callback for error code
 * @param [out] NA
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtSetExceptCallback(rtErrorCallback callback);

/**
 * @ingroup profiling_base
 * @brief get binary device base address, called by profiling
 * @param [in]  handle  program handle
 * @param [out] deviceBase   device base address
 * @return RT_ERROR_NONE for ok
 * @return ACL_ERROR_RT_PARAM_INVALID for error input
 */
RTS_API rtError_t rtGetBinaryDeviceBaseAddr(void *handle, void **deviceBase);

/**
 * @ingroup dvrt_base
 * @brief register callback for error code
 * @param [out] NA
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtSetTaskAbortCallBack(const char *moduleName, rtTaskAbortCallBack callback, void *args);

/**
 * @ingroup dvrt_base
 * @brief register callback for task fail
 * @param [out] NA
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtSetTaskFailCallback(rtTaskFailCallback callback);

typedef enum DevCallBackDir {
    DEV_CB_POS_FRONT = 1,
    DEV_CB_POS_BACK = 2,
    DEV_CB_POS_END
} rtDevCallBackDir_t;

/**
 * @ingroup dvrt_base
 * @brief register callback for deviceid by position
 * @param [in] regName unique register name, can't be null
 * @param [in] callback Device state callback function
 * @param [in] notifyPos callback notify Postion
 * @param [out] NA
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtRegDeviceStateCallbackEx(const char_t *regName, rtDeviceStateCallback callback,
    const rtDevCallBackDir_t notifyPos);

/**
 * @ingroup dvrt_base
 * @brief get exception register info while core exception
 * @param [in] exceptionInfo used to find error register info
 * @param [out] exceptionErrRegInfo exception error register info array
 * @param [out] num the num of elements in the array
 * @return RT_ERROR_NONE for ok, errno for failed
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtGetExceptionRegInfo(const rtExceptionInfo_t * const exceptionInfo,
    rtExceptionErrRegInfo_t **exceptionErrRegInfo, uint32_t *num);


/**
 * @ingroup dvrt_base
 * @brief create label instance
 * @param [out]    lbl   created label
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelCreate(rtLabel_t *lbl);

/**
 * @ingroup dvrt_base
 * @brief create label instance
 * @param [out] lbl  created label
 * @param [in] mdl  label set model
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelCreateV2(rtLabel_t *lbl, rtModel_t mdl);

/**
 * @ingroup dvrt_base
 * @brief set label and stream instance
 * @param [in] lbl   set label
 * @param [in] stm  set stream
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelSet(rtLabel_t lbl, rtStream_t stm);

/**
 * @ingroup dvrt_base
 * @brief destroy label instance
 * @param [in] lbl   label to destroy
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelDestroy(rtLabel_t lbl);

/**
 * @ingroup dvrt_base
 * @brief goto label instance
 * @param [in] lbl   goto label
 * @param [in] stm  to submit label_goto task
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelGoto(rtLabel_t lbl, rtStream_t stm);

/**
 * @ingroup dvrt_base
 * @brief label switch by index
 * @param [in] ptr  index value ptr
 * @param [in] maxValue  index max value
 * @param [in] labelInfoPtr  label content info ptr
 * @param [in] stm  set stream
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelSwitchByIndex(void *ptr, uint32_t maxValue, void *labelInfoPtr, rtStream_t stm);

/**
 * @ingroup dvrt_base
 * @brief stream goto label
 * @param [in] lbl  goto label
 * @param [in] stm  stream  to submit label_goto task
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelGotoEx(rtLabel_t lbl, rtStream_t stm);

/**
 * @ingroup dvrt_base
 * @brief labels to dev info
 * @param [in] lbl  model label list
 * @param [in] labelNumber  label number
 * @param [in] dst  device ptr
 * @param [in] dstMax  dst size
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelListCpy(rtLabel_t *lbl, uint32_t labelNumber, void *dst, uint32_t dstMax);

/**
 * @ingroup dvrt_base
 * @brief labels to dev info
 * @param [out] lbl  created label handle
 * @param [in] stm  label bind stream
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelCreateEx(rtLabel_t *lbl, rtStream_t stm);

/**
 * @ingroup dvrt_base
 * @brief labels to dev info
 * @param [out] lbl  created label handle
 * @param [in] mdl  label bind model
 * @param [in] stm  label bind stream
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 */
RTS_API rtError_t rtLabelCreateExV2(rtLabel_t *lbl, rtModel_t mdl, rtStream_t stm);

/**
 * @ingroup dvrt_base
 * @brief set stream mode
 * @param [in] stm  stream needed to be set mode
 * @param [in] stmMode mode
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtStreamSetMode(rtStream_t stm, const uint64_t stmMode);

/**
 * @ingroup dvrt_base
 * @brief get stream mode
 * @param [in] stm  stream needed to get its mode
 * @param [out] stmMode mode pointer
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtStreamGetMode(rtStream_t const stm, uint64_t * const stmMode);

/**
 * @ingroup dvrt_base
 * @brief Sets the SSID of the shinared notify.
 * @param [in] name share id name to be set
 * @param [in] sdid whitelisted sdid
 * @param [in] pid  whitelisted process
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 * @return RT_ERROR_DRV_ERR for driver error
 */
RTS_API rtError_t rtSetIpcNotifySuperPodPid(const char *name, uint32_t sdid, int32_t pid);

/**
 * @ingroup dvrt_base
 * @brief Setting SSIDs of Shared Memory in Batches.
 * @param [in] name Name used for sharing between processes
 * @param [in] sdid whitelisted sdid
 * @param [in] pid  host pid whitelist array
 * @param [in] num  number of pid arrays
 * @return RT_ERROR_NONE for ok
 * @return RT_ERROR_INVALID_VALUE for error input
 * @return RT_ERROR_DRV_ERR for driver error
 */
RTS_API rtError_t rtSetIpcMemorySuperPodPid(const char *name, uint32_t sdid, int32_t pid[], int32_t num);

#if defined(__cplusplus)
}
#endif

#endif  // CCE_RUNTIME_BASE_H

/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CMODEL_BUILD_SHIM_MODEL_API_H_
#define CMODEL_BUILD_SHIM_MODEL_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MODEL_RW_ERROR_NONE = 0,
    MODEL_RW_ERROR_ADDRESS = 1,
    MODEL_RW_ERROR_LEN = 2,
} modelRWStatus_t;

typedef enum {
    MODEL_INIT_SUCCESS = 0,
    MODEL_INIT_FAILED = 1,
} modelInitStatus_t;

modelRWStatus_t busDirectRead(void* ptr, uint64_t size, uint64_t address, uint32_t chipIndex);
modelRWStatus_t busDirectWrite(uint64_t address, uint64_t size, void* ptr, uint32_t chipIndex);
modelInitStatus_t modelInit(char* fileName, uint32_t chipNum);
void modelRun(void);
void startModel(const char* filepath, ...);
void stopModel(void);

#ifdef __cplusplus
}
#endif

#endif

/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include <stdint.h>
#include <stdlib.h>

#include "model/model_api.h"
#include "securec.h"
#include "tsch/host_interface.h"
#include "tsch/tsch_interrupt.h"

#define DRV_STUB_HBM_BASE (0x2000000ULL)
#define DRV_STUB_HBM_MAX_ADDR (0x400000000ULL)
#define DRV_STUB_MAX_ALLOC (DRV_STUB_HBM_MAX_ADDR - DRV_STUB_HBM_BASE)

static ts_task_cmd_queue_t g_ts_task_cmd[TS_TASK_CMD_QUEUE_PRIORITIES_LEVEL];
static ts_task_report_queue_t g_ts_task_report;

static modelRWStatus_t ValidateModelRwArgs(void* ptr, uint64_t size, uint64_t address)
{
    if (ptr == NULL) {
        return MODEL_RW_ERROR_ADDRESS;
    }
    if (address > DRV_STUB_HBM_MAX_ADDR) {
        return MODEL_RW_ERROR_ADDRESS;
    }
    if (size > (DRV_STUB_MAX_ALLOC - address)) {
        return MODEL_RW_ERROR_LEN;
    }
    return MODEL_RW_ERROR_NONE;
}

modelRWStatus_t busDirectRead(void* ptr, uint64_t size, uint64_t address, uint32_t chipIndex)
{
    (void)chipIndex;
    return ValidateModelRwArgs(ptr, size, address);
}

modelRWStatus_t busDirectWrite(uint64_t address, uint64_t size, void* ptr, uint32_t chipIndex)
{
    (void)chipIndex;
    return ValidateModelRwArgs(ptr, size, address);
}

ts_task_cmd_queue_t* ts_get_task_cmd_queues(uint8_t priority)
{
    errno_t ret;
    if (priority >= TS_TASK_CMD_QUEUE_PRIORITIES_LEVEL) {
        return NULL;
    }

    ret = memset_s(&g_ts_task_cmd[priority], sizeof(g_ts_task_cmd[priority]), 0, sizeof(g_ts_task_cmd[priority]));
    if (ret != EOK) {
        return NULL;
    }
    g_ts_task_cmd[priority].read_idx = 0U;
    g_ts_task_cmd[priority].write_idx++;
    return &g_ts_task_cmd[priority];
}

ts_task_report_queue_t* ts_get_task_report_queue(void)
{
    if (g_ts_task_report.read_idx > TS_TASK_REPORT_QUEUE_SLOTS_COUNT) {
        g_ts_task_report.read_idx = 0U;
    }
    if (g_ts_task_report.write_idx > TS_TASK_REPORT_QUEUE_SLOTS_COUNT) {
        g_ts_task_report.write_idx = 0U;
    }
    g_ts_task_report.write_idx = (g_ts_task_report.write_idx + 1U) % TS_TASK_REPORT_QUEUE_SLOTS_COUNT;
    return &g_ts_task_report;
}

void ts_trigger_interrupt(ts_interrupt_num_t interruptId) { (void)interruptId; }
void start_task_scheduler(void) {}
void stop_task_scheduler(void) {}

modelInitStatus_t modelInit(char* fileName, uint32_t chipNum)
{
    (void)fileName;
    (void)chipNum;
    return MODEL_INIT_SUCCESS;
}

void modelRun(void) {}
void startModel(const char* filepath, ...) { (void)filepath; }
void stopModel(void) {}

#ifndef __DRV_CFG_DEV_PLATFORM_ESL__
typedef void (*DriverReportIrqTriger)(uint32_t);

void tsRegDrvReportIrqTriger(DriverReportIrqTriger irqTriger)
{
    (void)irqTriger;
    (void)setenv("CAMODEL_LOG_PATH", "model", 1);
}
#endif

/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CMODEL_BUILD_SHIM_TSCH_HOST_INTERFACE_H_
#define CMODEL_BUILD_SHIM_TSCH_HOST_INTERFACE_H_

#include <stdint.h>

#include "tsch/tsch_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t task_command_slot[TS_SIZE_OF_PER_TASK_CMD_QUEUE][TS_TASK_COMMAND_SIZE];
    uint32_t read_idx;
    uint32_t write_idx;
} ts_task_cmd_queue_t;

typedef struct {
    uint8_t data[TS_TASK_REPORT_MSG_SIZE];
} ts_task_report_msg_t;

typedef struct {
    ts_task_report_msg_t task_report_slot[TS_TASK_REPORT_QUEUE_SLOTS_COUNT];
    uint32_t read_idx;
    uint32_t write_idx;
} ts_task_report_queue_t;

ts_task_cmd_queue_t* ts_get_task_cmd_queues(uint8_t priority);
ts_task_report_queue_t* ts_get_task_report_queue(void);

#ifdef __cplusplus
}
#endif

#endif

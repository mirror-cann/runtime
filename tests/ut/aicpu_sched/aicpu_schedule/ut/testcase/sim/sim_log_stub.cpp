/**
 * Copyright (c) 2026 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

// 日志桩：ASCEND_GLOBAL_LOG_LEVEL 控制级别（0=DEBUG..3=ERROR，默认 ERROR），ASCEND_SIM_LOG_FILE=1 落盘

#include <cstdlib>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <securec.h>
#include <mutex>

#include "slog.h"

static int32_t InitLogLevel()
{
    const char* env = std::getenv("ASCEND_GLOBAL_LOG_LEVEL");
    if (env != nullptr) {
        int32_t lvl = atoi(env);
        if (lvl >= DLOG_DEBUG && lvl <= DLOG_ERROR) {
            return lvl;
        }
    }
    return DLOG_ERROR;
}

static bool InitLogFileEnable()
{
    const char* env = std::getenv("ASCEND_SIM_LOG_FILE");
    return (env != nullptr && env[0] == '1');
}

static int32_t g_simLogLevel = InitLogLevel();
static bool g_simLogFileEnable = InitLogFileEnable();

static const char* LevelStr(int level)
{
    switch (level) {
        case DLOG_DEBUG:
            return "DEBUG";
        case DLOG_INFO:
            return "INFO";
        case DLOG_WARN:
            return "WARN";
        case DLOG_ERROR:
            return "ERROR";
        default:
            return "LOG";
    }
}

static bool IsRunLog(int32_t moduleId) { return (static_cast<uint32_t>(moduleId) & RUN_LOG_MASK) != 0U; }

static bool ShouldOutput(int32_t moduleId, int level)
{
    if (IsRunLog(moduleId)) {
        return true;
    }
    if (level >= DLOG_ERROR) {
        return true;
    }
    return level >= g_simLogLevel;
}

static void OutputLog(int level, const char* levelStr, const char* fmt, va_list valist)
{
    char buf[1024];
    int32_t ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, fmt, valist);
    if (ret < 0) {
        return;
    }
    // 多线程串行化 + flush，避免 stdout 缓冲导致日志丢失或交错
    static std::mutex s_logMutex;
    const std::lock_guard<std::mutex> lock(s_logMutex);
    printf("[%s] %s\n", levelStr, buf);
    fflush(stdout);
    if (g_simLogFileEnable) {
        FILE* fp = fopen("sim_utest.log", "a");
        if (fp != nullptr) {
            fprintf(fp, "[%s] %s\n", levelStr, buf);
            fclose(fp);
        }
    }
}

// slog 桩接口

void DlogRecord(int module_id, int level, const char* fmt, ...)
{
    if (!ShouldOutput(module_id, level)) {
        return;
    }
    va_list valist;
    va_start(valist, fmt);
    OutputLog(level, LevelStr(level), fmt, valist);
    va_end(valist);
}

int CheckLogLevel(int moduleId, int logLevel)
{
    if (IsRunLog(moduleId)) {
        return 1;
    }
    if (logLevel >= DLOG_ERROR) {
        return 1;
    }
    return (logLevel >= g_simLogLevel) ? 1 : 0;
}

int dlog_setlevel(int module_id, int level, int enable_event)
{
    // 空操作，避免 ComputeProcessMain 覆盖环境变量配置
    return g_simLogLevel;
}

int dlog_getlevel(int module_id, int* enable_event) { return g_simLogLevel; }

void DlogFlush() {}

int DlogSetAttr(LogAttr logAttr) { return 0; }

int DlogReportInitialize() { return 0; }
int DlogReportFinalize() { return 0; }

void DlogErrorInner(int module_id, const char* fmt, ...)
{
    va_list valist;
    va_start(valist, fmt);
    OutputLog(DLOG_ERROR, "ERROR", fmt, valist);
    va_end(valist);
}

void DlogWarnInner(int module_id, const char* fmt, ...)
{
    if (!ShouldOutput(module_id, DLOG_WARN))
        return;
    va_list valist;
    va_start(valist, fmt);
    OutputLog(DLOG_WARN, "WARN", fmt, valist);
    va_end(valist);
}

void DlogInfoInner(int module_id, const char* fmt, ...)
{
    if (!ShouldOutput(module_id, DLOG_INFO))
        return;
    va_list valist;
    va_start(valist, fmt);
    OutputLog(DLOG_INFO, "INFO", fmt, valist);
    va_end(valist);
}

void DlogDebugInner(int module_id, const char* fmt, ...)
{
    if (!ShouldOutput(module_id, DLOG_DEBUG))
        return;
    va_list valist;
    va_start(valist, fmt);
    OutputLog(DLOG_DEBUG, "DEBUG", fmt, valist);
    va_end(valist);
}

void DlogEventInner(int module_id, const char* fmt, ...)
{
    va_list valist;
    va_start(valist, fmt);
    OutputLog(DLOG_INFO, "EVENT", fmt, valist);
    va_end(valist);
}

void DlogInner(int module_id, int level, const char* fmt, ...) {}

void DlogWrite(int module_id, int level, const char* fmt, ...)
{
    if (!ShouldOutput(module_id, level))
        return;
    va_list valist;
    va_start(valist, fmt);
    OutputLog(level, LevelStr(level), fmt, valist);
    va_end(valist);
}

void dav_log(int module_id, const char* fmt, ...) {}

/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#include "gtest/gtest.h"
#include "mockcpp/mockcpp.hpp"
#include "trace_recorder.h"
#include "trace_attr.h"
#include "adiag_utils.h"
#include <pwd.h>
#include "mmpa_api.h"
#include "trace_types.h"
#include "trace_system_api.h"
#include <atomic>
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>

#define TIMESTAMP_MAX_LENGTH 29U

static std::atomic<uint32_t> g_traceOpenCallIndex(0);
static std::atomic<bool> g_firstTraceOpenEntered(false);
static std::atomic<bool> g_allowFirstTraceOpen(false);
static std::atomic<bool> g_agingThreadDone(false);

typedef struct {
    TraceDirInfo dirInfo;
    TraceFileInfo fileInfo;
    char dirTime[TIMESTAMP_MAX_LENGTH];
    TraStatus ret;
    int32_t fd;
    int32_t err;
} TraceGetFdThreadArg;

typedef struct {
    bool success;
} TraceAgingThreadArg;

int32_t TraceOpenBlockFirstThenOpen(const char *filePath, int32_t flag, uint32_t mode)
{
    uint32_t callIndex = g_traceOpenCallIndex.fetch_add(1);
    if (callIndex == 0U) {
        g_firstTraceOpenEntered.store(true);
        while (!g_allowFirstTraceOpen.load()) {
            usleep(1000);
        }
    }

    int32_t fd = open(filePath, flag, mode);
    if (fd >= 0) {
        (void)fchmod(fd, mode);
    }
    return fd;
}

void *TraceRecorderGetFdThread(void *arg)
{
    TraceGetFdThreadArg *threadArg = static_cast<TraceGetFdThreadArg *>(arg);
    errno = 0;
    threadArg->ret = TraceRecorderGetFd(&threadArg->dirInfo, &threadArg->fileInfo, &threadArg->fd);
    threadArg->err = errno;
    if (threadArg->fd >= 0) {
        close(threadArg->fd);
    }
    return NULL;
}

void *TraceRecorderCreateAgingDirsThread(void *arg)
{
    TraceAgingThreadArg *threadArg = static_cast<TraceAgingThreadArg *>(arg);
    threadArg->success = true;
    for (uint32_t i = 0; i < 10U; i++) {
        char timeStr[TIMESTAMP_MAX_LENGTH] = {0};
        int32_t ret = snprintf_s(timeStr, TIMESTAMP_MAX_LENGTH, TIMESTAMP_MAX_LENGTH - 1U,
            "1970010108000001%04u", i);
        if (ret == -1) {
            threadArg->success = false;
            break;
        }
        TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, getpid(), timeStr, true };
        TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "other", TRACE_FILE_TXT_SUFFIX };
        int32_t fd = -1;
        if (TraceRecorderGetFd(&dirInfo, &fileInfo, &fd) != TRACE_SUCCESS) {
            threadArg->success = false;
            break;
        }
        close(fd);
    }
    g_agingThreadDone.store(true);
    return NULL;
}

bool BuildRecorderDirPath(char *path, size_t len, const char *eventName, const char *dirTime)
{
    int32_t ret = snprintf_s(path, len, len - 1U, "%s/atrace/trace_%d_%d_%s/%s_event_%d_%s",
        LLT_TEST_DIR, TraceAttrGetPgid(), TraceAttrGetPid(), TraceAttrGetTime(), eventName, getpid(), dirTime);
    return ret != -1;
}

bool BuildRecorderFilePath(char *path, size_t len, const char *eventName, const char *dirTime,
    const char *tracerName, const char *objName, const char *suffix)
{
    char dirPath[MAX_FULLPATH_LEN + 1U] = {0};
    if (!BuildRecorderDirPath(dirPath, MAX_FULLPATH_LEN + 1U, eventName, dirTime)) {
        return false;
    }
    int32_t ret = snprintf_s(path, len, len - 1U, "%s/%s_tracer_%s%s", dirPath, tracerName, objName, suffix);
    return ret != -1;
}

class TraceRecorderUtest: public testing::Test {
protected:
    static void SetUpTestCase()
    {
    }

    virtual void SetUp()
    {
        system("mkdir -p " LLT_TEST_DIR "/ascend");
        struct passwd *pwd = getpwuid(getuid());
        pwd->pw_dir = LLT_TEST_DIR;
        MOCKER(getpwuid).stubs().will(returnValue(pwd));
        EXPECT_EQ(TRACE_SUCCESS, TraceAttrInit());
        EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());
    }

    virtual void TearDown()
    {
        system("rm -rf " LLT_TEST_DIR "/*");
        TraceRecorderExit();
        TraceAttrExit();
        GlobalMockObject::verify();
    }

    static void TearDownTestCase()
    {
        system("rm -rf " LLT_TEST_DIR );
    }
};

void TestRecorderWrite(uint32_t dirNum, uint32_t fileNum, uint32_t msgNum, const char *suffix)
{
    for (uint32_t i = 0; i < dirNum; i++) {
        char timeStr[TIMESTAMP_MAX_LENGTH] = {0};
        TraStatus ret = TimestampToFileStr(GetRealTime() + i * 100, timeStr, TIMESTAMP_MAX_LENGTH);
        EXPECT_EQ(TRACE_SUCCESS, ret);
        TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, TraceGetPid(), timeStr };
        printf("dir[%u] time = [%s]\n", i, timeStr);
        for (uint32_t j = 0; j < fileNum; j++) {
            std::string objName = "HCCL_" + std::to_string(i) + "_" + std::to_string(j);
            TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, objName.c_str(), suffix };
            int32_t fd = -1;
            ret = TraceRecorderGetFd(&dirInfo, &fileInfo, &fd);
            EXPECT_EQ(TRACE_SUCCESS, ret);
            for (uint32_t k = 0; k < msgNum; k++) {
                std::string msg = "msg_" + std::to_string(k) + "\n";
                ret = TraceRecorderWrite(fd, msg.c_str(), msg.size() + 1);
                EXPECT_EQ(TRACE_SUCCESS, ret);
            }
            close(fd);
        }
    }
}

void *RecordTrace(void *arg)
{
    (void)arg;
    sleep(1);
    TestRecorderWrite(10, 1, 2, TRACE_FILE_TXT_SUFFIX);
    return NULL;
}

TEST_F(TraceRecorderUtest, TestRecord_Concurrent)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    int32_t threadNum = 10;
    pthread_t theadId[threadNum] = { 0 };

    for (int i = 0; i < threadNum; i++) {
        int32_t ret = pthread_create(&theadId[i], NULL, RecordTrace, NULL);
        EXPECT_EQ(0, ret);
    }

    for (int i = 0; i < threadNum; i++) {
        pthread_join(theadId[i], NULL);
    }
    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TestRecordEnvPath)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    TestRecorderWrite(1, 1, 2, TRACE_FILE_TXT_SUFFIX);

    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TestRecordEnvInvalidPath)
{
    printf("path doesn't exist, then create it successfully.\n");
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR "/env/", 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    printf("path doesn't exist, then create it failed.\n");
    TraceRecorderExit();
    system("rm -rf " LLT_TEST_DIR "/env");
    MOCKER(TraceMkdir).stubs().will(returnValue(TRACE_FAILURE));
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());
    GlobalMockObject::verify();

    printf("path doesn't have permission to write.\n");
    TraceRecorderExit();
    MOCKER(TraceAccess).stubs().will(returnValue(-1));
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR "/env/", 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    unsetenv("ASCEND_WORK_PATH");
}


TEST_F(TraceRecorderUtest, TestRecordEnv_MkdirRecurFailed)
{
    printf("path doesn't exist, then create it.\n");
    printf("strdup failed.\n");
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR "/env/", 1);
    MOCKER(strdup).stubs().will(returnValue((char *)0));
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());
    GlobalMockObject::verify();

    printf("snprintf_s failed.\n");
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR "/env/", 1);
    MOCKER(vsnprintf_s).stubs().will(returnValue(-1));
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());
    GlobalMockObject::verify();

    printf("strncat_s failed.\n");
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR "/env/", 1);
    MOCKER(strncat_s).stubs().will(returnValue(-1));
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());
    GlobalMockObject::verify();

    printf("mkdir failed.\n");
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR "/env/", 1);
    MOCKER(TraceMkdir).stubs().will(returnValue(TRACE_FAILURE));
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());
    GlobalMockObject::verify();
    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TestRecordRealPathFailed)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    MOCKER(mmRealPath).stubs().will(returnValue(EN_ERROR));
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    TraceRecorderExit();
    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TestRecordHomePath)
{
    TestRecorderWrite(1, 1, 2, TRACE_FILE_TXT_SUFFIX);
}

TEST_F(TraceRecorderUtest, TestGetDirMkdirFailed)
{
    char timeStr[TIMESTAMP_MAX_LENGTH] = {0};
    auto ret = TimestampToFileStr(std::time(0), timeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);

    int testNum = 4;
    for (int i = 0 ; i < testNum; i++) {
        MOCKER(mkdir)
            .stubs()
            .will(repeat(0, i))
            .then(returnValue(-1));
        MOCKER(access).stubs().will(returnValue(-1));
        MOCKER(chown).stubs().will(returnValue(0));
        MOCKER(chmod).stubs().will(returnValue(0));
        TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, getpid(), timeStr };
        const TraceDirNode *dir = TraceRecorderGetDirPath(&dirInfo);
        EXPECT_EQ((const TraceDirNode *)0, dir);
        GlobalMockObject::verify();
    }
}

TEST_F(TraceRecorderUtest, TestSafeGetFdMkdirFailed)
{
    MOCKER(mkdir)
        .stubs()
        .will(returnValue(-1));
    char dirTimeStr[TIMESTAMP_MAX_LENGTH] = {0};
    auto ret = TimestampToFileStr(std::time(0), dirTimeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);
    TraceDirInfo dirInfo = {  TRACER_STACKCORE_NAME, getpid(), dirTimeStr};
    TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "HCCL", TRACE_FILE_TXT_SUFFIX };
    int32_t fd;
    ret = TraceRecorderSafeGetFd(&dirInfo, &fileInfo, &fd);
    EXPECT_EQ(TRACE_FAILURE, ret);
}

TEST_F(TraceRecorderUtest, TestSafeGetFd)
{
    char dirTimeStr[TIMESTAMP_MAX_LENGTH] = {0};
    auto ret = TimestampToFileStr(std::time(0), dirTimeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);
    TraceDirInfo dirInfo = {  TRACER_STACKCORE_NAME, getpid(), dirTimeStr};
    TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "HCCL", TRACE_FILE_TXT_SUFFIX };
    int32_t fd;
    ret = TraceRecorderSafeGetFd(&dirInfo, &fileInfo, &fd);
    EXPECT_EQ(TRACE_SUCCESS, ret);
}

TEST_F(TraceRecorderUtest, TestSafeGetBinFd)
{
    char dirTimeStr[TIMESTAMP_MAX_LENGTH] = {0};
    auto ret = TimestampToFileStr(std::time(0), dirTimeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);
    TraceDirInfo dirInfo = {  TRACER_STACKCORE_NAME, getpid(), dirTimeStr};
    TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "HCCL", TRACE_FILE_TXT_SUFFIX };
    int32_t fd;
    ret = TraceRecorderSafeGetFd(&dirInfo, &fileInfo, &fd);
    EXPECT_EQ(TRACE_SUCCESS, ret);
}

TEST_F(TraceRecorderUtest, TraceRecorderGetFd_Failed)
{
    char timeStr[TIMESTAMP_MAX_LENGTH] = {0};
    auto ret = TimestampToFileStr(std::time(0), timeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);

    TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, getpid(), timeStr };
    TraceFileInfo info = { TRACER_SCHEDULE_NAME,  "ts_0", ".txt" };
    int32_t fd = -1;
    MOCKER(TraceRecorderGetDirPath).stubs().will(returnValue((const TraceDirNode *)0));
    ret = TraceRecorderGetFd(&dirInfo, &info, &fd);
    EXPECT_EQ(TRACE_FAILURE, ret);
}

TEST_F(TraceRecorderUtest, TraceRecorderGetFd_SameDirAgingKeepsWritable)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    setenv("ASCEND_TRACE_RECORD_NUM", "10", 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    char timeStr[TIMESTAMP_MAX_LENGTH] = {0};
    auto ret = TimestampToFileStr(std::time(0), timeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);

    TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, getpid(), timeStr, true };
    TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "ts_0", TRACE_FILE_TXT_SUFFIX };
    for (uint32_t i = 0; i < 11U; i++) {
        int32_t fd = -1;
        ret = TraceRecorderGetFd(&dirInfo, &fileInfo, &fd);
        EXPECT_EQ(TRACE_SUCCESS, ret);
        close(fd);
    }

    char filePath[MAX_FULLPATH_LEN + 1U] = {0};
    ASSERT_TRUE(BuildRecorderFilePath(filePath, MAX_FULLPATH_LEN + 1U, TRACER_SCHEDULE_NAME, timeStr,
        TRACER_SCHEDULE_NAME, "ts_0", TRACE_FILE_TXT_SUFFIX));
    EXPECT_EQ(0, access(filePath, F_OK));

    unsetenv("ASCEND_TRACE_RECORD_NUM");
    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TraceRecorderGetFd_ConcurrentDirAgingKeepsFirstOpen)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    setenv("ASCEND_TRACE_RECORD_NUM", "10", 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    g_traceOpenCallIndex.store(0);
    g_firstTraceOpenEntered.store(false);
    g_allowFirstTraceOpen.store(false);
    g_agingThreadDone.store(false);
    MOCKER(TraceOpen).stubs().will(invoke(TraceOpenBlockFirstThenOpen));

    TraceGetFdThreadArg firstArg = {};
    int32_t ret = snprintf_s(firstArg.dirTime, TIMESTAMP_MAX_LENGTH, TIMESTAMP_MAX_LENGTH - 1U,
        "19700101080000000001");
    EXPECT_NE(-1, ret);
    firstArg.dirInfo = { TRACER_SCHEDULE_NAME, getpid(), firstArg.dirTime, true };
    firstArg.fileInfo = { TRACER_SCHEDULE_NAME, "first", TRACE_FILE_TXT_SUFFIX };
    firstArg.fd = -1;
    firstArg.ret = TRACE_SUCCESS;
    firstArg.err = 0;

    pthread_t threadId = 0;
    EXPECT_EQ(0, pthread_create(&threadId, NULL, TraceRecorderGetFdThread, &firstArg));
    for (uint32_t i = 0; i < 5000U && !g_firstTraceOpenEntered.load(); i++) {
        usleep(1000);
    }
    ASSERT_TRUE(g_firstTraceOpenEntered.load());

    TraceAgingThreadArg agingArg = { true };
    pthread_t agingThreadId = 0;
    EXPECT_EQ(0, pthread_create(&agingThreadId, NULL, TraceRecorderCreateAgingDirsThread, &agingArg));
    usleep(100000);
    EXPECT_FALSE(g_agingThreadDone.load());

    g_allowFirstTraceOpen.store(true);
    EXPECT_EQ(0, pthread_join(threadId, NULL));
    EXPECT_EQ(0, pthread_join(agingThreadId, NULL));
    EXPECT_EQ(TRACE_SUCCESS, firstArg.ret);
    EXPECT_GE(firstArg.fd, 0);
    EXPECT_TRUE(agingArg.success);

    GlobalMockObject::verify();
    unsetenv("ASCEND_TRACE_RECORD_NUM");
    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TraceRecorderGetFd_WriteCreatesExpectedFile)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    const char *dirTime = "19700101080002000001";
    TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, getpid(), dirTime, true };
    TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "normal", TRACE_FILE_TXT_SUFFIX };
    int32_t fd = -1;
    auto ret = TraceRecorderGetFd(&dirInfo, &fileInfo, &fd);
    EXPECT_EQ(TRACE_SUCCESS, ret);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderWrite(fd, "abc", 3U));
    close(fd);

    char filePath[MAX_FULLPATH_LEN + 1U] = {0};
    ASSERT_TRUE(BuildRecorderFilePath(filePath, MAX_FULLPATH_LEN + 1U, TRACER_SCHEDULE_NAME, dirTime,
        TRACER_SCHEDULE_NAME, "normal", TRACE_FILE_TXT_SUFFIX));
    EXPECT_EQ(0, access(filePath, F_OK));

    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TraceRecorderGetFd_SameDirWithinLimitKeepsWritable)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    setenv("ASCEND_TRACE_RECORD_NUM", "10", 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    const char *dirTime = "19700101080002000002";
    TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, getpid(), dirTime, true };
    TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "repeat", TRACE_FILE_TXT_SUFFIX };
    for (uint32_t i = 0; i < 2U; i++) {
        int32_t fd = -1;
        auto ret = TraceRecorderGetFd(&dirInfo, &fileInfo, &fd);
        EXPECT_EQ(TRACE_SUCCESS, ret);
        EXPECT_EQ(TRACE_SUCCESS, TraceRecorderWrite(fd, "x", 1U));
        close(fd);
    }

    char filePath[MAX_FULLPATH_LEN + 1U] = {0};
    ASSERT_TRUE(BuildRecorderFilePath(filePath, MAX_FULLPATH_LEN + 1U, TRACER_SCHEDULE_NAME, dirTime,
        TRACER_SCHEDULE_NAME, "repeat", TRACE_FILE_TXT_SUFFIX));
    EXPECT_EQ(0, access(filePath, F_OK));

    unsetenv("ASCEND_TRACE_RECORD_NUM");
    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TraceRecorderGetFd_UniqueDirAgingRemovesOldestOnly)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    setenv("ASCEND_TRACE_RECORD_NUM", "10", 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    char dirPaths[11][MAX_FULLPATH_LEN + 1U] = {};
    for (uint32_t i = 0; i < 11U; i++) {
        char dirTime[TIMESTAMP_MAX_LENGTH] = {0};
        int32_t ret = snprintf_s(dirTime, TIMESTAMP_MAX_LENGTH, TIMESTAMP_MAX_LENGTH - 1U,
            "1970010108000201%04u", i);
        EXPECT_NE(-1, ret);
        ASSERT_TRUE(BuildRecorderDirPath(dirPaths[i], MAX_FULLPATH_LEN + 1U, TRACER_SCHEDULE_NAME, dirTime));

        TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, getpid(), dirTime, true };
        TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "unique", TRACE_FILE_TXT_SUFFIX };
        int32_t fd = -1;
        EXPECT_EQ(TRACE_SUCCESS, TraceRecorderGetFd(&dirInfo, &fileInfo, &fd));
        close(fd);
    }

    EXPECT_NE(0, access(dirPaths[0], F_OK));
    for (uint32_t i = 1; i < 11U; i++) {
        EXPECT_EQ(0, access(dirPaths[i], F_OK));
    }

    unsetenv("ASCEND_TRACE_RECORD_NUM");
    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TraceRecorderGetFd_ReusedOldDirDoesNotRefreshAgingOrder)
{
    TraceRecorderExit();
    setenv("ASCEND_WORK_PATH", LLT_TEST_DIR, 1);
    setenv("ASCEND_TRACE_RECORD_NUM", "10", 1);
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderInit());

    char dirPaths[11][MAX_FULLPATH_LEN + 1U] = {};
    char dirTimes[11][TIMESTAMP_MAX_LENGTH] = {};
    for (uint32_t i = 0; i < 10U; i++) {
        int32_t ret = snprintf_s(dirTimes[i], TIMESTAMP_MAX_LENGTH, TIMESTAMP_MAX_LENGTH - 1U,
            "1970010108000301%04u", i);
        EXPECT_NE(-1, ret);
        ASSERT_TRUE(BuildRecorderDirPath(dirPaths[i], MAX_FULLPATH_LEN + 1U, TRACER_SCHEDULE_NAME, dirTimes[i]));

        TraceDirInfo dirInfo = { TRACER_SCHEDULE_NAME, getpid(), dirTimes[i], true };
        TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "timestamp", TRACE_FILE_TXT_SUFFIX };
        int32_t fd = -1;
        EXPECT_EQ(TRACE_SUCCESS, TraceRecorderGetFd(&dirInfo, &fileInfo, &fd));
        close(fd);
    }

    TraceDirInfo reusedDirInfo = { TRACER_SCHEDULE_NAME, getpid(), dirTimes[0], true };
    TraceFileInfo reusedFileInfo = { TRACER_SCHEDULE_NAME, "reuse", TRACE_FILE_TXT_SUFFIX };
    int32_t reusedFd = -1;
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderGetFd(&reusedDirInfo, &reusedFileInfo, &reusedFd));
    close(reusedFd);

    int32_t ret = snprintf_s(dirTimes[10], TIMESTAMP_MAX_LENGTH, TIMESTAMP_MAX_LENGTH - 1U,
        "1970010108000301%04u", 10U);
    EXPECT_NE(-1, ret);
    ASSERT_TRUE(BuildRecorderDirPath(dirPaths[10], MAX_FULLPATH_LEN + 1U, TRACER_SCHEDULE_NAME, dirTimes[10]));

    TraceDirInfo newDirInfo = { TRACER_SCHEDULE_NAME, getpid(), dirTimes[10], true };
    TraceFileInfo newFileInfo = { TRACER_SCHEDULE_NAME, "timestamp", TRACE_FILE_TXT_SUFFIX };
    int32_t newFd = -1;
    EXPECT_EQ(TRACE_SUCCESS, TraceRecorderGetFd(&newDirInfo, &newFileInfo, &newFd));
    close(newFd);

    EXPECT_NE(0, access(dirPaths[0], F_OK));
    for (uint32_t i = 1; i < 11U; i++) {
        EXPECT_EQ(0, access(dirPaths[i], F_OK));
    }

    unsetenv("ASCEND_TRACE_RECORD_NUM");
    unsetenv("ASCEND_WORK_PATH");
}

TEST_F(TraceRecorderUtest, TestTraceRecorderSafeGetFd_Failed)
{
    auto ret = TraceRecorderSafeGetFd(NULL, NULL, NULL);
    EXPECT_EQ(TRACE_INVALID_PARAM, ret);

    char timeStr[TIMESTAMP_MAX_LENGTH] = {0};
    ret = TimestampToFileStr(std::time(0), timeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);
    TraceDirInfo dirInfo = { TRACER_STACKCORE_NAME, getpid(), timeStr };
    TraceFileInfo fileInfo = { TRACER_SCHEDULE_NAME, "HCCL", ".txt" };
    int32_t fd = -1;
    TraStatus status = TRACE_FAILURE;

    MOCKER(TraceMkdir).stubs().will(returnValue(status));
    ret = TraceRecorderSafeGetFd(&dirInfo, &fileInfo, &fd);
    EXPECT_EQ(status, ret);
    GlobalMockObject::verify();

    MOCKER(strncat_s).stubs().will(returnValue(-1));
    ret = TraceRecorderSafeGetFd(&dirInfo, &fileInfo, &fd);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();

    MOCKER(TraceOpen).stubs().will(returnValue(-1));
    ret = TraceRecorderSafeGetFd(&dirInfo, &fileInfo, &fd);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();
}

TEST_F(TraceRecorderUtest, TestTraceRecorderSafeMkdirPath_Failed)
{
    char timeStr[TIMESTAMP_MAX_LENGTH] = {0};
    auto ret = TimestampToFileStr(std::time(0), timeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);
    TraceDirInfo dirInfo = { TRACER_STACKCORE_NAME, getpid(), timeStr };

    MOCKER(vsnprintf_s).stubs().will(returnValue(-1));
    ret = TraceRecorderSafeMkdirPath(&dirInfo);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();

    MOCKER(vsnprintf_s).stubs()
        .will(returnValue(0))
        .then(returnValue(-1));
    MOCKER(TraceMkdir).stubs().will(returnValue(TRACE_SUCCESS));
    ret = TraceRecorderSafeMkdirPath(&dirInfo);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();

    MOCKER(vsnprintf_s).stubs()
        .will(returnValue(0))
        .then(returnValue(0))
        .then(returnValue(-1));
    MOCKER(TraceMkdir).stubs().will(returnValue(TRACE_SUCCESS));
    ret = TraceRecorderSafeMkdirPath(&dirInfo);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();

    MOCKER(TraceMkdir).stubs().will(returnValue(TRACE_FAILURE));
    ret = TraceRecorderSafeMkdirPath(&dirInfo);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();

    MOCKER(TraceMkdir).stubs()
        .will(returnValue(TRACE_SUCCESS))
        .then(returnValue(TRACE_FAILURE));
    ret = TraceRecorderSafeMkdirPath(&dirInfo);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();

    MOCKER(TraceMkdir).stubs()
        .will(returnValue(TRACE_SUCCESS))
        .then(returnValue(TRACE_SUCCESS))
        .then(returnValue(TRACE_FAILURE));
    ret = TraceRecorderSafeMkdirPath(&dirInfo);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();

    MOCKER(TraceMkdir).stubs()
        .will(returnValue(TRACE_SUCCESS))
        .then(returnValue(TRACE_SUCCESS))
        .then(returnValue(TRACE_SUCCESS))
        .then(returnValue(TRACE_FAILURE));
    ret = TraceRecorderSafeMkdirPath(&dirInfo);
    EXPECT_EQ(TRACE_FAILURE, ret);
    GlobalMockObject::verify();
}

TEST_F(TraceRecorderUtest, TestTraceRecorderSafeGetDirPath_Failed)
{
    char timeStr[TIMESTAMP_MAX_LENGTH] = {0};
    auto ret = TimestampToFileStr(std::time(0), timeStr, TIMESTAMP_MAX_LENGTH);
    EXPECT_EQ(TRACE_SUCCESS, ret);
    TraceDirInfo dirInfo = { TRACER_STACKCORE_NAME, getpid(), timeStr };
    char path[1024] = {0};

    EXPECT_EQ(TRACE_INVALID_PARAM, TraceRecorderSafeGetDirPath(NULL, path, 1024));
    EXPECT_EQ(TRACE_INVALID_PARAM, TraceRecorderSafeGetDirPath(&dirInfo, NULL, 1024));
    EXPECT_EQ(TRACE_INVALID_PARAM, TraceRecorderSafeGetDirPath(&dirInfo, path, 0));

    MOCKER(vsnprintf_s).stubs().will(returnValue(-1));
    EXPECT_EQ(TRACE_FAILURE, TraceRecorderSafeGetDirPath(&dirInfo, path, 1024));
}

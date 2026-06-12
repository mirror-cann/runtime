/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */

#ifndef CCE_RUNTIME_RT_MEM_QUEUE_H
#define CCE_RUNTIME_RT_MEM_QUEUE_H

#include "base.h"
#include "runtime/rt_external_mem.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define RT_BUFF_SP_HUGEPAGE_ONLY (1 << 1)

typedef enum rt_queue_work_mode {
    RT_QUEUE_MODE_PUSH = 1,
    RT_QUEUE_MODE_PULL,
} RT_QUEUE_WORK_MODE;

RTS_API rtError_t rtBuffGetInfo(rtBuffGetCmdType type, const void * const inBuff, uint32_t inLen,
    void * const outBuff, uint32_t * const outLen);

/**
 * @ingroup rt_mem_queue
 * @brief init queue schedule
 * @param [in] devId   the logical device id
 * @param [in] grpName   the name of group, can be nullptr
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueInitQS(int32_t devId, const char_t *grpName);

/**
 * @ingroup rt_mem_queue
 * @brief init flow gateway
 * @param [in] devId   the logical device id
 * @param [in] initInfo   Initialization parameters
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueInitFlowGw(int32_t devId, const rtInitFlowGwInfo_t * const initInfo);

/**
 * @ingroup rt_mem_queue
 * @brief create mbuf queue
 * @param [in] devId   the logical device id
 * @param [in] queAttr   attribute of queue
 * @param [out] qid  queue id
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueCreate(int32_t devId, const rtMemQueueAttr_t *queAttr, uint32_t *qid);

/**
 * @ingroup rt_mem_queue
 * @brief mbuf queue export for share
 * @param [in] devId   the logical id of the shared device
 * @param [in] qid     the shared Memory queue id
 * @param [in] peerDevId   the logical device id of peer
 * @param [in] shareName   the name of the share memque
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueExport(int32_t devId, uint32_t qid, int32_t peerDevId, const char * const shareName);

/**
 * @ingroup rt_mem_queue
 * @brief mbuf queue unexport for share
 * @param [in] devId    the logical id of the shared device
 * @param [in] qid  the shared Memory queue id
 * @param [in] peerDevId   the logical device id of peer
 * @param [in] shareName   the name of the share memque
 * @return RT_ERROR_NONE for ok
 */
 RTS_API rtError_t rtMemQueueUnExport(int32_t devId, uint32_t qid, int32_t peerDevId, const char * const shareName);

/**
 * @ingroup rt_mem_queue
 * @brief mbuf queue import
 * @param [in] devId   the logical id of the import device
 * @param [in] peerDevId   the logical device id of peer
 * @param [in] shareName   the name of the shared memque
 * @param [out] qid   the queue id of the import device
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueImport(int32_t devId, int32_t peerDevId, const char * const shareName, uint32_t * const qid);

/**
 * @ingroup rt_mem_queue
 * @brief mbuf queue unimport
 * @param [in] devId   the logical id of the import device
 * @param [in] qid     the queue id of the import device
 * @param [in] peerDevId   the logical device id of peer
 * @param [in] shareName   the name of the shared memque
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueUnImport(int32_t devId, uint32_t qid, int32_t peerDevId, const char * const shareName);

/**
 * @ingroup rt_mem_queue
 * @brief mbuf queue set
 * @param [in] devId   the logical device id
 * @param [in] cmd     cmd type of queue set
 * @param [in] input   input param of queue set
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueSet(int32_t devId, rtMemQueueSetCmdType cmd, const rtMemQueueSetInputPara *input);

/**
 * @ingroup rt_mem_queue
 * @brief destroy mbuf queue
 * @param [in] devId   the logical device id
 * @param [in] qid  queue id
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueDestroy(int32_t devId, uint32_t qid);

/**
 * @ingroup rt_mem_queue
 * @brief destroy mbuf queue init
 * @param [in] devId   the logical device id
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueInit(int32_t devId);

/**
* @ingroup rt_mem_queue
* @brief  queue reset
* @attention null
* @param [in] qid: qid
* @param [in] devId: logic devid
* @return 0 for success, others for fail
**/
RTS_API rtError_t rtMemQueueReset(int32_t devId, uint32_t qid);

/**
 * @ingroup rt_mem_queue
 * @brief enqueue memBuf
 * @param [in] devId   the logical device id
 * @param [in] qid  queue id
 * @param [in] memBuf   enqueue memBuf
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueEnQueue(int32_t devId, uint32_t qid, void *memBuf);


/**
 * @ingroup rt_mem_queue
 * @brief dequeue memBuf
 * @param [in] devId   the logical device id
 * @param [in] qid  queue id
 * @param [out] memBuf   dequeue memBuf
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueDeQueue(int32_t devId, uint32_t qid, void **memBuf);

/**
 * @ingroup rt_mem_queue
 * @brief enqueu peek
 * @param [in] devId   the logical device id
 * @param [in] qid  queue id
 * @param [out] bufLen   length of mbuf in queue
 * @param [in] timeout  peek timeout  (ms), -1: wait all the time until peeking success
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueuePeek(int32_t devId, uint32_t qid, size_t *bufLen, int32_t timeout);

/**
 * @ingroup rt_mem_queue
 * @brief enqueu  buff
 * @param [in] devId   the logical device id
 * @param [in] qid  queue id
 * @param [in] inBuf   enqueue buff
 * @param [in] timeout  enqueue timeout  (ms), -1: wait all the time until enqueue success
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueEnQueueBuff(int32_t devId, uint32_t qid, rtMemQueueBuff_t *inBuf, int32_t timeout);

/**
 * @ingroup rt_mem_queue
 * @brief enqueu  buff
 * @param [in] devId   the logical device id
 * @param [in] qid  queue id
 * @param [out] outBuf   dequeue buff
 * @param [in] timeout  dequeue timeout  (ms), -1: wait all the time until dequeue success
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueDeQueueBuff(int32_t devId, uint32_t qid, rtMemQueueBuff_t *outBuf, int32_t timeout);

/**
 * @ingroup rt_mem_queue
 * @brief query current queue info
 * @param [in] devId   the logical device id
 * @param [in] qid  queue id
 * @param [out] queInfo   current queue info
 * @return RT_ERROR_NONE for ok
 */
RTS_API rtError_t rtMemQueueQueryInfo(int32_t devId, uint32_t qid, rtMemQueueInfo_t *queInfo);

/**
* @ingroup rt_mem_queue
* @brief  query queue status
* @param [in] devId: the logical device id
* @param [in] cmd: query cmd
* @param [in] inBuff: input buff
* @param [in] inLen: the length of input
* @param [in|out] outBuff: output buff
* @param [in|out] outLen: the length of output
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMemQueueQuery(int32_t devId, rtMemQueueQueryCmd_t cmd, const void *inBuff, uint32_t inLen,
    void *outBuff, uint32_t *outLen);

/**
* @ingroup rt_mem_queue
* @brief  grant queue
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] pid: pid
* @param [in] attr: queue share attr
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMemQueueGrant(int32_t devId, uint32_t qid, int32_t pid, rtMemQueueShareAttr_t *attr);

/**
* @ingroup rt_mem_queue
* @brief  attach queue
* @param [in] devId: logic devid
* @param [in] qid: queue id
* @param [in] timeOut: timeOut
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMemQueueAttach(int32_t devId, uint32_t qid, int32_t timeOut);

/**
* @ingroup rt_mem_queue
* @brief  Commit the event to a specific process
* @param [in] devId: logic devid
* @param [in] evt: event summary info
* @param [out] ack: event reply info
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtEschedSubmitEventSync(int32_t devId, rtEschedEventSummary_t *evt,
                                          rtEschedEventReply_t *ack);

/**
* @ingroup rt_mem_queue
* @brief  query device proccess id
* @param [in] info: see struct rtBindHostpidInfo_t
* @param [out] devPid: device proccess id
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtQueryDevPid(rtBindHostpidInfo_t *info, int32_t *devPid);

/**
* @ingroup rt_mem_queue
* @brief device buff init
* @param [in] cfg, init cfg
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufInit(rtMemBuffCfg_t *cfg);

/**
* @ingroup rt_mem_queue
* @brief alloc buff
* @param [out]  buff: The buff id the shared memory pointer applied by calling halBuffAlloc and halBuffAllocByPool
* @param [in]  size: The amount of memory space requested
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtBuffAlloc(const uint64_t size, void **buff);

/**
* @ingroup rt_mem_queue
* @brief determine whether buff id is the shared memory pointer applied by calling halBuffAlloc and halBuffAllocByPool
* @param [in]  buff: The buff id the shared memory pointer applied by calling halBuffAlloc and halBuffAllocByPool
* @param [in]  size: The amount of memory space requested
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtBuffConfirm(void *buff, const uint64_t size);

/**
* @ingroup rt_mem_queue
* @brief alloc buff
* @param [out] mbufPtr: buff addr alloced
* @param [in]  buff: The buff must be the shared memory pointer applied by calling halBuffAlloc and halBuffAllocByPool
* @param [in]  size: The amount of memory space requested
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufBuild(void *buff, const uint64_t size, rtMbufPtr_t *mbufPtr);

/**
* @ingroup rt_mem_queue
* @brief alloc buff
* @param [out] memBuf: buff addr alloced
* @param [in]  size: The amount of memory space requested
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufAlloc(rtMbufPtr_t *memBuf, uint64_t size);

/**
* @ingroup rt_mem_queue
* @brief alloc buff
* @param [out] memBuf: buff addr alloced
* @param [in]  size: The amount of memory space requested
* @param [in]  flag: Huge page flag(bit0~31: mem type, bit32~bit35: devid, bit36~63: resv)
* @param [in]  grpId: group id
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufAllocEx(rtMbufPtr_t *memBuf, uint64_t size, uint64_t flag, int32_t grpId);

/**
* @ingroup rt_mem_queue
* @brief free buff
* @param [in]  buff: The buff id the shared memory pointer applied by calling halBuffAlloc and halBuffAllocByPool
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtBuffFree(void *buff);

/**
* @ingroup rt_mem_queue
* @brief free the head of mbufPtr
* @param [in] mbufPtr: buff addr alloced
* @param [out]  buff: The buffer of mbuPtr
* @param [out]  size: The amount of memory space of buffer
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufUnBuild(const rtMbufPtr_t mbufPtr, void **buff, uint64_t *size);

/**
* @ingroup rt_mem_queue
* @brief get mbuffer
* @param [in] mbufPtr: buff addr alloced
* @param [out]  buff: The buffer of mbuPtr
* @param [in]  size: The amount of memory space of buffer
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtBuffGet(const rtMbufPtr_t mbufPtr, void *buff, const uint64_t size);

/**
* @ingroup rt_mem_queue
* @brief put mbuffer
* @param [in] mbufPtr: buff addr alloced
* @param [out]  buff: The buffer of mbuPtr
* @param [out]  size: The amount of memory space of buffer
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtBuffPut(const rtMbufPtr_t mbufPtr, void *buff);


/**
* @ingroup rt_mem_queue
* @brief free buff
* @param [in] memBuf: buff addr to be freed
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufFree(rtMbufPtr_t memBuf);

/**
* @ingroup rt_mem_queue
* @brief set Data len of Mbuf
* @param [in] memBuf: Mbuf addr
* @param [in] len: data len
* @return   RT_ERROR_NONE for success, others for fail
*/
RTS_API rtError_t rtMbufSetDataLen(rtMbufPtr_t memBuf, uint64_t len);

/**
* @ingroup rt_mem_queue
* @brief set Data len of Mbuf
* @param [in] memBuf: Mbuf addr
* @param [out] len: data len
* @return   RT_ERROR_NONE for success, others for fail
*/
RTS_API rtError_t rtMbufGetDataLen(rtMbufPtr_t memBuf, uint64_t *len);


/**
* @ingroup rt_mem_queue
* @brief get Data addr of Mbuf
* @param [in] memBuf: Mbuf addr
* @param [out] buf: Mbuf data addr
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufGetBuffAddr(rtMbufPtr_t memBuf, void **buf);

/**
* @ingroup rt_mem_queue
* @brief get total Buffer size of Mbuf
* @param [in] memBuf: Mbuf addr
* @param [out] totalSize: total buffer size of Mbuf
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufGetBuffSize(rtMbufPtr_t memBuf, uint64_t *totalSize);

/**
* @ingroup rt_mem_queue
* @brief Get the address and length of its user_data from the specified Mbuf
* @param [in] memBuf: Mbuf addr
* @param [out] priv: address of its user_data
* @param [out]  size: length of its user_data
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufGetPrivInfo(rtMbufPtr_t memBuf,  void **priv, uint64_t *size);

/**
* @ingroup rt_mem_queue
* @brief copy buf ref
* @param [in] memBuf: src buff addr
* @param [out] newMemBuf: des buff addr
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufCopyBufRef(rtMbufPtr_t memBuf, rtMbufPtr_t *newMemBuf);

/**
* @ingroup rt_mem_queue
* @brief append mbuf to mbuf chain
* @param [inout] memBufChainHead, the mbuf chain head
* @param [in] memBuf, the mbuf to append
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufChainAppend(rtMbufPtr_t memBufChainHead, rtMbufPtr_t memBuf);

/**
* @ingroup rt_mem_queue
* @brief get mbuf num in mbuf chain
* @param [in] memBufChainHead, the mbuf chain head
* @param [out] num, the mbuf chain size
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufChainGetMbufNum(rtMbufPtr_t memBufChainHead, uint32_t *num);

/**
* @ingroup rt_mem_queue
* @brief get mbuf in mbuf chain
* @param [in] mbufChainHead, the mbuf chain head
* @param [in] index, the mbuf index which to get in chain
* @param [out] mbuf, the mbuf to get
* @return RT_ERROR_NONE for ok
*/
RTS_API rtError_t rtMbufChainGetMbuf(rtMbufPtr_t memBufChainHead, uint32_t index, rtMbufPtr_t *memBuf);


/**
* @ingroup rt_mem_queue
* @brief create mem group
* @attention null
* @param [in] name, group name
* @param [in] cfg, group cfg
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtMemGrpCreate(const char_t *name, const rtMemGrpConfig_t *cfg);

/**
* @ingroup rt_mem_queue
* @brief alloc mem group cache
* @attention null
* @param [in] name, group name
* @param [in] devId, device id
* @param [in] para, mem group cache alloc para
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtMemGrpCacheAlloc(const char_t *name, int32_t devId, const rtMemGrpCacheAllocPara *para);

/**
* @ingroup rt_mem_queue
* @brief add process to group
* @param [in] name, group name
* @param [in] pid, process id
* @param [in] attr, process permission in group
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtMemGrpAddProc(const char_t *name, int32_t pid, const rtMemGrpShareAttr_t *attr);

/**
* @ingroup rt_mem_queue
* @brief attach proccess to check permission in group
* @param [in] name, group name
* @param [in] timeout, time out ms
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtMemGrpAttach(const char_t *name, int32_t timeout);

/**
* @ingroup rt_mem_queue
* @brief buff group query
* @param [in] input, query input
* @param [in|out] output, query output
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtMemGrpQuery(rtMemGrpQueryInput_t * const input, rtMemGrpQueryOutput_t *output);

/**
* @ingroup rt_mem_queue
* @brief buff group query
* @param [in] devId, cdevice id
* @param [in] name, group name
* @param [out] qid, queue id
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtMemQueueGetQidByName(int32_t devId, const char_t *name, uint32_t *qId);

/**
* @ingroup rt_mem_queue
* @brief esched attach device
* @param [in] devId, device id
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtEschedAttachDevice(int32_t devId);

/**
* @ingroup rt_mem_queue
* @brief esched dettach device
* @param [in] devId, device id
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtEschedDettachDevice(int32_t devId);

/**
* @ingroup rt_mem_queue
* @brief esched wait event
* @param [in] devId, device id
* @param [in] grpId, group id
* @param [in] threadId, thread id
* @param [in] timeout
* @param [in] evt
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtEschedWaitEvent(int32_t devId, uint32_t grpId, uint32_t threadId,
                                    int32_t timeout, rtEschedEventSummary_t *evt);

/**
* @ingroup rt_mem_queue
* @brief esched create group
* @param [in] devId, device id
* @param [in] grpId, group id
* @param [in] type, group type
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtEschedCreateGrp(int32_t devId, uint32_t grpId, rtGroupType_t type);

/**
* @ingroup rt_mem_queue
* @brief esched submit event
* @param [in] devId, device id
* @param [in] evt
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtEschedSubmitEvent(int32_t devId, rtEschedEventSummary_t *evt);

/**
* @ingroup rt_mem_queue
* @brief esched submit event
* @param [in] devId, device id
* @param [in] grpId, group id
* @param [in] threadId, thread id
* @param [in] eventBitmap
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtEschedSubscribeEvent(int32_t devId, uint32_t grpId, uint32_t threadId, uint64_t eventBitmap);

/**
* @ingroup rtEschedAckEvent
* @brief esched ack event
* @param [in] devId, device id
* @param [in] evtId, event type
* @param [in] subEvtId, sub event type
* @param [in] msg, message info
* @param [in] len, message length
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtEschedAckEvent(int32_t devId, rtEventIdType_t evtId,
                                   uint32_t subEvtId, char_t *msg, uint32_t len);

/**
* @ingroup rtQueueSubF2NFEvent
* @brief full to not full event
* @param [in] devId, device id
* @param [in] qid, queue id
* @param [in] groupId, group id
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtQueueSubF2NFEvent(int32_t devId, uint32_t qId, uint32_t groupId);

/**
* @ingroup rtQueueSubscribe
* @brief queue subscribe
* @param [in] devId, device id
* @param [in] qid, queue id
* @param [in] groupId, group id
* @param [in] type

* @return   0 for success, others for fail
*/
RTS_API rtError_t rtQueueSubscribe(int32_t devId, uint32_t qId, uint32_t groupId, int32_t type);

/**
* @ingroup rtBufEventTrigger
* @brief buf event trigger
* @param [in] name, group name
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtBufEventTrigger(const char_t *name);


/**
* @ingroup rtEschedQueryInfo
* @brief  query esched info, such as grpid.
* @param [in] devId: logic devid
* @param [in] type: query info type
* @param [in] inPut: Input the corresponding data structure based on the type.
* @param [out] outPut: OutPut the corresponding data structure based on the type.
* @return   0 for success, others for fail
*/
RTS_API rtError_t rtEschedQueryInfo(const uint32_t devId, const rtEschedQueryType type,
    rtEschedInputInfo *inPut, rtEschedOutputInfo *outPut);
#if defined(__cplusplus)
}
#endif
#endif // CCE_RUNTIME_RT_MEM_QUEUE_H

# 4_model_sync_external

## Description
This sample demonstrates the Record External and Wait External usage of Events in ACL Graph cross-boundary synchronization scenarios, covering three typical synchronization scenarios.

## Product Support

This sample has the following support status on the following products:

| Product | Supported |
| --- |-----------|
| Ascend 950PR/Ascend 950DT | Yes       |
| Atlas A3 training series products/Atlas A3 inference series products | Yes       |
| Atlas A2 training series products/Atlas A2 inference series products | Yes       |

## Build and Run
For environment installation details and running details, see [README](../../../README_en.md) in the example directory.


Run steps:

```bash
# Replace ${install_root} with CANN installation root directory, default installation at /usr/local/Ascend
source ${install_root}/cann/set_env.sh
export ASCEND_INSTALL_PATH=${install_root}/cann

# Build and run
bash run.sh
```

## Three Synchronization Scenarios

### Scenario 1: Stream Record + Graph Wait External
On an external Stream, record an Event via `aclrtRecordEvent`; inside the Graph, wait for that Event via `aclrtStreamWaitEventWithFlag(..., ACL_EVENT_WAIT_EXTERNAL)`. This ensures the Graph only proceeds after the external task completes.

### Scenario 2: Graph Record External + Stream Wait
Inside the Graph, record an Event via `aclrtRecordEventWithFlag(..., ACL_EVENT_RECORD_EXTERNAL)`; on the external Stream, wait for that Event via `aclrtStreamWaitEvent`. This ensures the external Stream only proceeds after the Graph task completes.

### Scenario 3: Graph1 Record External + Graph2 Wait External
Inside Graph1, record an Event via `aclrtRecordEventWithFlag(..., ACL_EVENT_RECORD_EXTERNAL)`; inside Graph2, wait for that Event via `aclrtStreamWaitEventWithFlag(..., ACL_EVENT_WAIT_EXTERNAL)`. This achieves synchronization between two Graphs.

## CANN RUNTIME API
Key features and interfaces in this sample:
- Initialization
    - Call aclInit interface to initialize AscendCL configuration.
    - Call aclFinalize interface to deinitialize AscendCL.
- Device Management
    - Call aclrtSetDevice interface to specify Device for computation.
    - Call aclrtResetDeviceForce interface to forcibly reset current computation Device and reclaim Device resources.
- Context Management
    - Call aclrtCreateContext interface to create Context.
    - Call aclrtDestroyContext interface to destroy Context.
- Stream Management
    - Call aclrtCreateStream interface to create Stream.
    - Call aclrtSynchronizeStream interface to block waiting for Stream task completion.
    - Call aclrtDestroyStream interface to destroy Stream.
- Event Management (Cross-Graph Boundary Synchronization)
    - Call aclrtCreateEventExWithFlag interface to create an Event with flag set to ACL_EVENT_SYNC for cross-Graph boundary synchronization.
    - Call aclrtRecordEvent interface to record Event on a Stream (normal Record).
    - Call aclrtRecordEventWithFlag interface to record Event on a Stream with a flag; when flag is ACL_EVENT_RECORD_EXTERNAL, it is a cross-Graph boundary External Record.
    - Call aclrtStreamWaitEvent interface to block specified Stream waiting for Event completion (normal Wait).
    - Call aclrtStreamWaitEventWithFlag interface to block specified Stream waiting for Event with a flag; when flag is ACL_EVENT_WAIT_EXTERNAL, it is a cross-Graph boundary External Wait.
    - Call aclrtDestroyEvent interface to destroy Event.
- Model Management
    - Call aclmdlRICaptureBegin interface to start capture mode.
    - Call aclmdlRICaptureEnd interface to end capture mode and get modelRI handle.
    - Call aclmdlRIExecuteAsync interface to asynchronously execute model inference.
    - Call aclmdlRIDestroy interface to destroy model runtime instance.
- Memory Management
    - Call aclrtMalloc interface to allocate Device memory.
    - Call aclrtFree interface to release Device memory.
- Data Transfer
    - Call aclrtMemcpy interface to implement data transfer by memory copy.
    - Call aclrtMemcpyAsync interface to implement data transfer by async memory copy.

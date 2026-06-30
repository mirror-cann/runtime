# 4_model_sync_external

## 描述
本样例展示了ACL Graph跨边界同步场景下Event的Record External和Wait External用法，涵盖三种典型同步场景。

## 产品支持情况

本样例在以下产品上的支持情况如下：

| 产品 | 是否支持 |
| --- | --- |
| Ascend 950PR/Ascend 950DT | √ |
| Atlas A3 训练系列产品/Atlas A3 推理系列产品 | √ |
| Atlas A2 训练系列产品/Atlas A2 推理系列产品 | √ |

## 编译运行
环境安装详情以及运行详情请见example目录下的[README](../../../README.md)。

运行步骤如下：

```bash
# ${install_root} 替换为 CANN 安装根目录，默认安装在`/usr/local/Ascend`目录
source ${install_root}/cann/set_env.sh
export ASCEND_INSTALL_PATH=${install_root}/cann

# 编译运行
bash run.sh
```

## 三种同步场景

### 场景1: Stream Record + Graph Wait External
外部Stream上通过`aclrtRecordEvent`记录Event，Graph内部通过`aclrtStreamWaitEventWithFlag(..., ACL_EVENT_WAIT_EXTERNAL)`等待该Event。实现外部任务完成后Graph才继续执行。

### 场景2: Graph Record External + Stream Wait
Graph内部通过`aclrtRecordEventWithFlag(..., ACL_EVENT_RECORD_EXTERNAL)`记录Event，外部Stream上通过`aclrtStreamWaitEvent`等待该Event。实现Graph内任务完成后外部才继续执行。

### 场景3: Graph1 Record External + Graph2 Wait External
Graph1内部通过`aclrtRecordEventWithFlag(..., ACL_EVENT_RECORD_EXTERNAL)`记录Event，Graph2内部通过`aclrtStreamWaitEventWithFlag(..., ACL_EVENT_WAIT_EXTERNAL)`等待该Event。实现两个Graph之间的同步。

## CANN RUNTIME API
在该Sample中，涉及的关键功能点及其关键接口，如下所示：
- 初始化
    - 调用aclInit接口初始化AscendCL配置。
    - 调用aclFinalize接口实现AscendCL去初始化。
- Device管理
    - 调用aclrtSetDevice接口指定用于运算的Device。
    - 调用aclrtResetDeviceForce接口强制复位当前运算的Device，回收Device上的资源。
- Context管理
    - 调用aclrtCreateContext接口创建Context。
    - 调用aclrtDestroyContext接口销毁Context。
- Stream管理
    - 调用aclrtCreateStream接口创建Stream。
    - 调用aclrtSynchronizeStream接口阻塞等待Stream上任务的完成。
    - 调用aclrtDestroyStream接口销毁Stream。
- Event管理（跨Graph边界同步）
    - 调用aclrtCreateEventExWithFlag接口创建Event，flag设置为ACL_EVENT_SYNC，用于跨Graph边界同步。
    - 调用aclrtRecordEvent接口在Stream上记录Event（普通Record）。
    - 调用aclrtRecordEventWithFlag接口在Stream上按flag记录Event，flag设置为ACL_EVENT_RECORD_EXTERNAL时为跨Graph边界的External Record。
    - 调用aclrtStreamWaitEvent接口阻塞指定Stream等待Event完成（普通Wait）。
    - 调用aclrtStreamWaitEventWithFlag接口按flag阻塞指定Stream等待Event完成，flag设置为ACL_EVENT_WAIT_EXTERNAL时为跨Graph边界的External Wait。
    - 调用aclrtDestroyEvent接口销毁Event。
- model管理
    - 调用aclmdlRICaptureBegin接口开始捕获模式。
    - 调用aclmdlRICaptureEnd接口结束捕获模式，并得到modelRI句柄。
    - 调用aclmdlRIExecuteAsync接口异步执行模型推理。
    - 调用aclmdlRIDestroy接口销毁模型运行实例。
- 内存管理
    - 调用aclrtMalloc接口申请Device上的内存。
    - 调用aclrtFree接口释放Device上的内存。
- 数据传输
    - 调用aclrtMemcpy接口通过内存复制的方式实现数据传输。
    - 调用aclrtMemcpyAsync接口通过异步内存复制的方式实现数据传输。

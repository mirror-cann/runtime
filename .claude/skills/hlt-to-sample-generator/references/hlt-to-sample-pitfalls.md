# HLT → Sample 技术陷阱清单

从 HLT 测试代码转换为 sample 时，容易踩到的技术陷阱。编写代码前务必对照检查。

## 1. Capture 模式 vs Build 模式的 memcpy 限制

这是最常见的陷阱，已导致 107027/107030 等运行时错误。

### Capture 模式（`aclmdlRICaptureBegin/End`）

Capture 模式是"录制-回放"机制：capture 期间的操作被录制下来，之后通过 `aclmdlRIExecuteAsync` 回放执行。回放时，原始 capture 期间的 host 指针已经不再有效。

**规则**：
- `ACL_MEMCPY_HOST_TO_DEVICE` **不能**在 capture 上下文（`aclmdlRICaptureBegin` 到 `aclmdlRICaptureEnd` 之间）中使用
- 所有 H2D 数据拷贝必须在 capture 开始之前完成
- capture 上下文中只允许：aclnn 算子调用、`ACL_MEMCPY_DEVICE_TO_DEVICE`、stream 操作

**正确示例**：
```cpp
// H2D memcpy 在 capture 之前
CHECK_ERROR(aclrtMemcpy(srcADev, kSize, srcAHost.data(), kSize, ACL_MEMCPY_HOST_TO_DEVICE));
CHECK_ERROR(aclrtMemcpy(srcBDev, kSize, srcBHost.data(), kSize, ACL_MEMCPY_HOST_TO_DEVICE));

// capture 上下文中只有算子调用和 D2D 操作
CHECK_ERROR(aclmdlRICaptureBegin(stream, ACL_MODEL_RI_CAPTURE_MODE_THREAD_LOCAL));
aclnnAdd(wsAddr, wsSize, addExecutor, subStream);
CHECK_ERROR(aclmdlRICaptureEnd(stream, &parentModelRI));
```

**错误示例**：
```cpp
CHECK_ERROR(aclmdlRICaptureBegin(stream, ACL_MODEL_RI_CAPTURE_MODE_THREAD_LOCAL));
// 错误！host 指针在回放时无效
CHECK_ERROR(aclrtMemcpyAsync(srcADev, kSize, srcAHost.data(), kSize, ACL_MEMCPY_HOST_TO_DEVICE, subStream));
aclnnAdd(wsAddr, wsSize, addExecutor, subStream);
CHECK_ERROR(aclmdlRICaptureEnd(stream, &parentModelRI));
```

### Build 模式（`aclmdlRIBuildBegin/End`）

Build 模式是即时构建，绑定 stream 上的操作会立即执行，不存在"回放"问题。

**规则**：
- `ACL_MEMCPY_HOST_TO_DEVICE` 可以在绑定的 stream 上使用
- 可参考 `2_model_switch` 样例的模式

### 条件变量的 H2D memcpy

执行阶段（capture 结束后）写入条件变量值（如 `condDevPtr`）的 H2D memcpy 是合法的，因为此时不在 capture 上下文中：
```cpp
// capture 已结束，写入条件变量是合法的
CHECK_ERROR(aclrtMemcpy(condDevPtr, sizeof(uint64_t), &kCondTrue, sizeof(uint64_t), ACL_MEMCPY_HOST_TO_DEVICE));
CHECK_ERROR(aclmdlRIExecuteAsync(parentModelRI, execStream));
```

### WHILE 循环体的条件清零

WHILE 循环体中需要将条件变量清零（设置循环终止条件）。由于这发生在 capture 上下文中，必须使用 D2D memcpy：
```cpp
// capture 之前准备零值设备内存
static const uint64_t kCondZero = 0;
void* condZeroDev = nullptr;
CHECK_ERROR(aclrtMalloc(&condZeroDev, sizeof(uint64_t), ACL_MEM_MALLOC_HUGE_FIRST));
CHECK_ERROR(aclrtMemcpy(condZeroDev, sizeof(uint64_t), &kCondZero, sizeof(uint64_t), ACL_MEMCPY_HOST_TO_DEVICE));

// capture 上下文中用 D2D 清零条件变量
CHECK_ERROR(aclmdlRICaptureToModelRIBegin(subStream, loopBodyModel[0], ...));
aclnnAdd(wsAddr, wsSize, addExecutor, subStream);
CHECK_ERROR(aclrtMemcpyAsync(condDevPtr, sizeof(uint64_t), condZeroDev, sizeof(uint64_t), ACL_MEMCPY_DEVICE_TO_DEVICE, subStream));
CHECK_ERROR(aclmdlRICaptureEnd(subStream, &loopBodyModel[0]));
```

## 2. 未发布 API 的 include 和链接处理

部分 Runtime API 仅在 repo 的 `include/external/acl/acl_rt.h` 中声明，尚未出现在已发布 CANN 包的头文件中。例如条件 RI API：
- `aclmdlRICondHandleCreate`
- `aclmdlRICondHandleGetCondPtr`
- `aclmdlRIAddCondTask`
- `aclmdlRICaptureToModelRIBegin`

**处理方式**：
- CMakeLists.txt 中 `include_directories` 必须将 repo 的 `include/external` 路径放在 CANN 包路径之前，确保编译器优先找到 repo 版本的头文件
- 但已发布的 `libascendcl.so` 中底层实现可能返回 `ACL_ERROR_RT_FEATURE_NOT_SUPPORT (207000)`，这取决于安装的 CANN 版本
- README 中应标注 CANN 版本要求

**示例 CMakeLists.txt include 顺序**：
```cmake
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/external  # repo 头文件优先
    ${ASCEND_CANN_PACKAGE_PATH}/include                        # CANN 包头文件
    ${ASCEND_CANN_PACKAGE_PATH}/aclnn                          # aclnn 头文件
    ...)
```

## 3. CHECK_ERROR 宏与函数返回类型

`example/utils.h` 中的 `CHECK_ERROR` 宏定义为：
```cpp
#define CHECK_ERROR(expr) \
    do { \
        if ((expr) != ACL_SUCCESS) { \
            ERROR_LOG("Operation failed: %s returned error code %d", #expr, (expr)); \
            return -1; \
        } \
    } while (0)
```

该宏使用 `return -1`，因此：
- **不兼容 void 返回类型的函数** — 编译会报错
- 所有使用 `CHECK_ERROR` 的函数应返回 `int32_t`
- 如果确实需要 void 函数中的错误检查，可使用 `CHECK_ERROR_WITHOUT_RETURN`（仅打印日志不 return）

## 4. 平台差异与 CANN 版本

不同 SoC 对同一 Runtime 特性的支持程度不同：

- 不同 SoC 型号（A2 vs A3 vs A5）可能对某些 API 返回 `ACL_ERROR_RT_FEATURE_NOT_SUPPORT (207000)`
- 不同 CANN 版本（9.0 vs 9.1 vs 9.2）的 API 实现完整度不同
- 同一个 CANN 版本在不同 SoC 上安装的子包可能不同（如 A5 可能缺少 aclnnop 头文件）

**建议**：
- README 中明确标注 CANN 版本要求
- 产品支持情况表中标注各接口在不同产品上的支持/不支持
- 如果特性不支持，考虑像 `overflow_detection` 样例那样做优雅降级（检测错误码 → 打印告警 → 清理资源退出）

## 5. 共享工具模块的使用注意

很多类别目录下有共享工具模块（如 `model_utils.h/cpp`、`kernel_utils.h/cpp`），这些模块被同类别下多个样例共用。

**使用注意**：
- 新样例可以直接使用同类别已有的共享工具，减少重复代码
- 如果需要新增共享工具函数，考虑是否影响其他已有样例
- 共享工具的修改需谨慎评估影响范围

## 6. 流生命周期与状态

创建 stream 时需注意：
- 普通样例用 `aclrtCreateStream` 即可
- model RI Build 模式通常用 `aclrtCreateStreamWithConfig(&stream, 0x00U, ACL_STREAM_PERSISTENT)` 创建持久流
- model RI Capture 模式可用普通流
- 在 Build 模式中，建议使用独立的 copyStream 进行 H2D/H2D 拷贝，避免与模型绑定流冲突
- 注意每个 stream 在使用完后都要 destroy

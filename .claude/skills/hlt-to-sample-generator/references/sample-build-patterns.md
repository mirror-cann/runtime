# Sample 构建模式参考

CANN Runtime example 目录下使用的 5 种 CMake 构建模式和 4 种 run.sh 模式。根据样例的功能需求选择匹配的模式。

## 目录

| 章节 | 内容 |
|------|------|
| CMake Pattern A | ACL-only（最简单，无算子无 kernel） |
| CMake Pattern B | ACL + opapi（使用 aclnn 预置算子） |
| CMake Pattern C | ACL + AscendC kernel（使用 ascendc.cmake） |
| CMake Pattern D | ACL + AscendC fatbin（kernel launch 场景） |
| CMake Pattern E | Profiling / Adump 特定库 |
| run.sh Pattern 1 | 简单构建运行 |
| run.sh Pattern 2 | 构建 + 安装 + 验证 |
| run.sh Pattern 3 | Full kernel 构建流水线 |
| run.sh Pattern 4 | 多进程/多程序 |

---

## CMake Pattern A: ACL-only

适用场景：只使用基础 acl/aclrt 接口，不使用 aclnn 算子，不使用 AscendC kernel。

代表样例：`1_basic_features/device/`、`1_basic_features/stream/`、`1_basic_features/event/`、`4_reliability/overflow_detection/`

```cmake
cmake_minimum_required(VERSION 3.16.0)
project(Runtime_Sample)

include_directories(
    ${ASCEND_CANN_PACKAGE_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

link_directories(${ASCEND_CANN_PACKAGE_PATH}/lib64)

add_executable(main main.cpp)

target_compile_options(main PRIVATE
    -O2 -std=c++17 -D_GLIBCXX_USE_CXX11_ABI=0 -Wall -Werror)

target_link_libraries(main PRIVATE
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libascendcl.so)
```

**关键特征**：
- 只链接 `libascendcl.so`
- include 只有 CANN 包 + example 根目录
- 不需要 `SOC_VERSION` / `ASCENDC_CMAKE_DIR` 环境变量

---

## CMake Pattern B: ACL + opapi

适用场景：使用 aclnn 预置算子（如 aclnnAdd），不使用自定义 AscendC kernel。

代表样例：`0_quickstart/0_hello_cann`、`2_advanced_features/model_ri/0_simple_model`

```cmake
cmake_minimum_required(VERSION 3.16.0)
project(Runtime_Sample)

include_directories(
    ${ASCEND_CANN_PACKAGE_PATH}/include
    ${ASCEND_CANN_PACKAGE_PATH}/aclnn
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

link_directories(${ASCEND_CANN_PACKAGE_PATH}/lib64)

add_executable(main main.cpp ../model_utils.cpp)

target_link_libraries(main PRIVATE
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libascendcl.so
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libnnopbase.so
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libopapi.so)
```

**关键特征**：
- include `${ASCEND_CANN_PACKAGE_PATH}/aclnn` — 提供 `aclnnop/aclnn_add.h` 等头文件
- 链接 `libnnopbase.so` + `libopapi.so` — aclnn 算子执行所需
- 可能包含类别级共享源文件（如 `../model_utils.cpp`）
- include `${CMAKE_CURRENT_SOURCE_DIR}/..` — 类别级头文件

**如果使用 repo 未发布 API**，需在 include_directories 最前面添加 repo 路径：
```cmake
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/external  # repo 头文件优先
    ${ASCEND_CANN_PACKAGE_PATH}/include
    ${ASCEND_CANN_PACKAGE_PATH}/aclnn
    ...)
```

---

## CMake Pattern C: ACL + AscendC kernel

适用场景：使用自定义 AscendC kernel（通过 `ascendc_library` 编译）。

代表样例：`1_basic_features/memory/` 大部分样例、`5_performance/adump/`

```cmake
cmake_minimum_required(VERSION 3.16.0)
project(Runtime_Sample)

set(SOC_VERSION $ENV{SOC_VERSION})
set(ASCENDC_CMAKE_DIR $ENV{ASCENDC_CMAKE_DIR})
set(RUN_MODE "npu" CACHE STRING "run mode: npu")
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type Release/Debug (default Debug)" FORCE)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/out" CACHE STRING "path for install()" FORCE)

include(${ASCENDC_CMAKE_DIR}/ascendc.cmake)

ascendc_library(kernels STATIC ../../../kernel_func/easy_OP.cpp)

include_directories(
    ${ASCEND_CANN_PACKAGE_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

link_directories(${ASCEND_CANN_PACKAGE_PATH}/lib64)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE ascendcl)
```

**关键特征**：
- 需要环境变量 `SOC_VERSION` 和 `ASCENDC_CMAKE_DIR`（通过 `set_sample_env.sh` 设置）
- `include(${ASCENDC_CMAKE_DIR}/ascendc.cmake)` — 引入 AscendC 构建基础设施
- `ascendc_library(kernels STATIC ...)` — 将 kernel 源文件编译为静态库
- kernel 源文件在 `example/kernel_func/` 下，通过相对路径引用
- 链接 `ascendcl`（CMake target 名）

---

## CMake Pattern D: ACL + AscendC fatbin（kernel launch）

适用场景：kernel launch 样例，需要 `ascendc_fatbin_library` 编译多个 kernel。

代表样例：`2_advanced_features/kernel/0_launch_kernel`

```cmake
cmake_minimum_required(VERSION 3.16.0)
project(Kernel_Launch_Sample)

set(SOC_VERSION $ENV{SOC_VERSION})
set(ASCENDC_CMAKE_DIR $ENV{ASCENDC_CMAKE_DIR})
set(RUN_MODE "npu" CACHE STRING "run mode: npu")
set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type Release/Debug (default Debug)" FORCE)
set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/out" CACHE STRING "path for install()" FORCE)

file(GLOB KERNEL_FILES ../../../kernel_func/add_custom.cpp)
include(${ASCENDC_CMAKE_DIR}/ascendc.cmake)

ascendc_fatbin_library(ascendc_kernels ${KERNEL_FILES})

include_directories(
    ${ASCEND_CANN_PACKAGE_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

link_directories(${ASCEND_CANN_PACKAGE_PATH}/lib64)

add_executable(main main.cpp ../file_ops.cpp)

target_link_libraries(main PRIVATE
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libacl_rt.so)
```

**关键特征**：
- 使用 `ascendc_fatbin_library()` 编译 kernel 为 fatbin
- 链接 `libacl_rt.so`（不是 `libascendcl.so`）— kernel launch 场景需要
- 通常有 `install()` 步骤部署到 `out/bin/`、`out/lib/`

---

## CMake Pattern E: Profiling / Adump 特定库

适用场景：性能分析（profiling）或精度调试（adump）。

### Profiling 变体

代表样例：`5_performance/profiling/`

```cmake
cmake_minimum_required(VERSION 3.16.0)
project(Profiling_Sample)

set(ASCEND_CANN_PACKAGE_PATH $ENV{ASCEND_HOME_PATH})

include_directories(
    ${ASCEND_CANN_PACKAGE_PATH}/include
    ${ASCEND_CANN_PACKAGE_PATH}/include/driver
    ${ASCEND_CANN_PACKAGE_PATH}/include/acl
    ${ASCEND_CANN_PACKAGE_PATH}/runtime/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..)

link_directories(${ASCEND_CANN_PACKAGE_PATH}/lib64)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libmsprofiler.so
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libascendcl.so
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libprofapi.so)
```

**关键特征**：
- 使用 `$ENV{ASCEND_HOME_PATH}`（不是 `ASCEND_CANN_PACKAGE_PATH` cmake 变量）
- 额外 include：`include/driver`、`include/acl`、`runtime/include`
- 链接 `libmsprofiler.so` + `libprofapi.so`

### Adump 变体

基于 Pattern C（带 ascendc.cmake），额外链接 `libascend_dump.so` 和 `libopapi.so`：

```cmake
target_link_libraries(main PRIVATE
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libascendcl.so
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libnnopbase.so
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libopapi.so
    ${ASCEND_CANN_PACKAGE_PATH}/lib64/libascend_dump.so)
```

---

## run.sh Pattern 1: 简单构建运行

适用场景：Pattern A / Pattern B 样例。

```bash
set -e
_ASCEND_INSTALL_PATH=$ASCEND_INSTALL_PATH
source $_ASCEND_INSTALL_PATH/bin/setenv.bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"
BUILD_DIR="${SCRIPT_DIR}/build"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake .. -DASCEND_CANN_PACKAGE_PATH="${_ASCEND_INSTALL_PATH}"
make -j$(nproc)
cd "${SCRIPT_DIR}"
./build/main
```

**关键特征**：
- source `setenv.bash`
- `cmake ..` + `make`
- 运行 `./build/main`

---

## run.sh Pattern 2: 构建 + 安装 + 验证

适用场景：需要 install 步骤或有输出验证的样例。

```bash
set -e
_ASCEND_INSTALL_PATH=$ASCEND_INSTALL_PATH
source $_ASCEND_INSTALL_PATH/bin/setenv.bash
rm -rf build
mkdir -p build
cmake -B build -DASCEND_CANN_PACKAGE_PATH=${_ASCEND_INSTALL_PATH}
cmake --build build -j
cmake --install build
./build/main | tee output_msg.txt
```

**关键特征**：
- 使用 `cmake -B build` + `cmake --build` + `cmake --install`（新式 cmake CLI）
- 捕获输出到文件

---

## run.sh Pattern 3: Full kernel 构建流水线

适用场景：Pattern C / Pattern D 样例（需要 AscendC kernel 编译）。

```bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXAMPLE_DIR="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
source "${EXAMPLE_DIR}/common/resolve_cann_env.sh"
resolve_cann_env
detect_sample_env  # 调用 set_sample_env.sh 自动检测 SOC_VERSION / ASCENDC_CMAKE_DIR

# 构建参数
SOC_VERSION="${SOC_VERSION}"
BUILD_TYPE="Debug"
BUILD_DIR="${SCRIPT_DIR}/build"
INSTALL_PREFIX="${SCRIPT_DIR}/out"

cmake -B "${BUILD_DIR}" \
    -DSOC_VERSION="${SOC_VERSION}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DASCEND_CANN_PACKAGE_PATH="${ASCEND_INSTALL_PATH}"
cmake --build "${BUILD_DIR}" -j$(nproc)
cmake --install "${BUILD_DIR}"

# 运行
"${INSTALL_PREFIX}/bin/main" "${RUN_MODE}"
```

**关键特征**：
- 使用 `common/resolve_cann_env.sh` + `set_sample_env.sh` 自动检测环境
- 传递 `SOC_VERSION` / `ASCENDC_CMAKE_DIR` 给 cmake
- 使用 `cmake --build/--install` 新式 CLI

---

## run.sh Pattern 4: 多进程/多程序

适用场景：IPC、跨进程、跨服务器样例。

有 `proc_a.cpp` + `proc_b.cpp` 两个可执行文件，或 `server.cpp` + `client.cpp`。使用文件协调（写 "done" 文件信号）。

---

## 模式选择决策树

```
是否使用 aclnn 预置算子？
├── 否 → 是否使用自定义 AscendC kernel？
│        ├── 否 → Pattern A (ACL-only)
│        └── 是 → 是否使用 <<<>>> 调用语法（kernel launch）？
│                 ├── 否 → Pattern C (ascendc_library)
│                 └── 是 → Pattern D (ascendc_fatbin_library)
└── 是 → 是否使用 model RI 或其他高级 API？
         ├── 否 → Pattern B (ACL+opapi)
         └── 是 → Pattern B + 可能需要 repo 未发布头文件
                   + 可能需要特定链接库

是否是 profiling/adump？
├── 是 → Pattern E (特定库)
```

对应 run.sh：
- Pattern A/B → run.sh Pattern 1 或 2
- Pattern C/D → run.sh Pattern 3
- 多进程 → run.sh Pattern 4

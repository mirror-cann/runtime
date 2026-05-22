# 任务注册机制重构通用规则

## 版本信息
- **版本**: 1.4
- **更新日期**: 2026-05-20
- **适用范围**: 纯粹任务注册修改
- **新增规则**: G11（编译适配规则-扩展cmake范围）、G13（静态注册函数无需头文件声明）

## G1: Skills优化约束
- **禁止删除现有规则**：优化规则时仅允许新增或修改描述，禁止删除已有规则
- **冲突处理**：若新增规则与现有规则冲突，必须建立优先级并明确说明
- **版本号更新**：规则优化后必须更新对应规则文件版本号

## G2: 任务粒度迁移约束
- **必须以任务为粒度**：禁止跨任务批量迁移，每个任务必须完整迁移（v100→v200_base→v200→v201）后再迁移下一任务
- **版本顺序**：单任务迁移必须按 v100 → v200_base → v200 → v201 顺序执行
- **编译适配同步**：每版本迁移后立即进行编译适配，David不再依赖该任务的xx_v100.cc

## G3: 代码删除约束
- **先增后删**：必须先新增注册代码，编译通过后再删除旧赋值代码
- **删除位置记录**：删除的每行代码必须记录文件路径和行号
- **禁止删除未迁移任务代码**：仅删除已迁移任务的赋值代码，未迁移任务代码保留

## G4: 注册函数命名约束
- **注册函数名**：`{TaskName}TaskRegister`
- **静态变量名**：`g_{taskName}TaskRegister`
- **禁止重名**：不同任务文件的注册函数名必须唯一

## G5: TaskFuncSingle使用约束
- **字段名一致性**：TaskFuncSingle字段名必须与TaskFuncArrays完全一致
- **栈变量定义**：操作数据在注册函数中作为栈变量定义，禁止定义全局静态变量
- **指定初始化器**：使用C++20指定初始化器语法
- **字段赋值规则**：
  - 若有单独赋值：使用单独赋值的函数
  - 若无单独赋值但有非nullptr默认值循环：使用默认值函数（不能设置为nullptr，否则会覆盖默认值）
  - 若无单独赋值也无默认值（全局初始化为nullptr）：设置为nullptr

## G6: 编译验证约束
- **每步骤编译**：每个版本迁移后均必须编译验证
- **编译失败处理**：编译失败必须修复后重新执行
- **连续失败处理**：同一错误连续出现2次，暂停执行并请求用户介入

## G7: David SQE注册约束
- **v200_base/v200/v201必须注册**：David版本任务必须调用RegDavidSqeFunc注册SQE函数
- **v100不注册**：v100版本任务不调用RegDavidSqeFunc
- **SQE函数指针**：David版本toSqeFunc字段置为nullptr

## G8: Config只读约束
- **全流程禁止修改**：config.md仅Step 0读取确认，全程只读
- **缺失字段**：由用户输入填充，写入后需用户确认

## G9: toCommandFunc复用约束
- **David复用v100函数值**：v200_base/v200/v201版本的toCommandFunc使用v100对应的函数值，在任务文件的注册函数中与其他字段一起调用RegTaskFunc注册，不需要单独注册
- **v100单独注册**：v100版本需要单独注册toCommandFunc
- **示例**：若v100的toCommandFunc为nullptr，David版本注册时toCommandFunc也设置为nullptr

## G11: 编译适配规则（扩展cmake范围）

### cmake排查范围（新增）
- **必查文件列表**：
  - `tests/ut/runtime/runtime/CMakeLists.txt` - UT编译配置
  - `src/runtime/cmake/v200.cmake` - v200/v201版本编译配置
  - `src/runtime/cmake/cmodel.cmake` - cmodel版本编译配置
  - `src/runtime/cmake/runtime.cmake` - v100版本编译配置（检查是否遗漏）
  - `src/runtime/cmake/tiny.cmake` - tiny版本编译配置（检查是否遗漏）

### 删除范围
- **仅删除机制依赖的v100文件**：只删除在 `# mechanism dependance` 注释后面、当前任务对应的v100文件
- **位置标记**：`# mechanism dependance` 注释后的v100文件列表

### 保留范围
- **不删除** `runtimeut_common_src_task_list` 中的v100文件（v100基础列表，所有平台都需要）
- **不删除** `runtimeut_david_series_common_src_task_list` 中非mechanism dependance部分的v100文件
- **不删除** `tests/ut/runtime/runtime/test/platform/910B/CMakeLists.txt` 的v100文件（910B平台v100测试需要）

### 删除示例（v200.cmake）
```cmake
# mechanism dependance（注释在Line 107）
${TOP_DIR}/.../maintenance/float_status_task_v100.cc
${TOP_DIR}/.../barrier/barrier_task_v100.cc
...
${TOP_DIR}/.../model/model_graph_task_v100.cc
${TOP_DIR}/.../davinci/davinci_multiple_task_v100.cc  # ← 删除这一行（当前任务）
${TOP_DIR}/.../reduce/reduce_task_v100.cc
...
```

### 删除示例（cmodel.cmake）
```cmake
# mechanism dependance（注释在Line 145）
...
${TOP_DIR}/.../model/model_graph_task_v100.cc
${TOP_DIR}/.../davinci/davinci_kernel_task_v100.cc  # ← 删除这一行（KERNEL任务）
${TOP_DIR}/.../davinci/davinci_multiple_task_v100.cc  # ← 删除这一行（MULTIPLE_TASK任务）
${TOP_DIR}/.../maintenance/float_status_task_v100.cc
...
```

### cmake排查流程
1. **Step B4开始时**：读取所有cmake文件，识别mechanism dependance列表
2. **逐文件排查**：检查每个cmake文件中是否包含当前任务的v100文件
3. **记录删除位置**：记录文件路径、行号、删除内容
4. **批量删除**：一次性删除所有cmake文件中的对应行

## G12: 删除位置记录规则
- **删除代码记录表**：必须记录文件路径、函数名、行号、删除内容摘要
- **编译适配记录表**：必须记录删除的v100文件位置、保留的位置及原因

## G13: TaskFuncSingle赋值函数头文件声明清理规则（新增）

### 注册函数特性
- **注册函数特性**：`{TaskName}TaskRegister` 是静态函数，通过全局变量 `g_{taskName}TaskRegister` 自动调用
- **无需头文件声明**：静态注册函数仅在定义文件内可见，不需要在其他文件调用，因此不需要在头文件中声明

### TaskFuncSingle赋值函数检查
- **检查范围**：TaskFuncSingle结构体中赋值的所有函数指针
- **检查方法**：使用grep搜索每个函数在代码库中的调用情况（排除定义位置）
- **判断标准**：若函数仅在定义文件内使用，或仅通过函数指针注册调用，则删除头文件声明

### 需保留声明的函数
- **跨文件调用**：函数在其他源文件中被直接调用（非通过函数指针）
- **UT调用**：函数在UT测试文件中被直接调用
- **跨版本使用**：函数在v200/v201版本中被直接使用（如PrintErrorInfoForDavinciTask）
- **基础设施函数**：Phase1的基础设施函数（如RegTaskFunc、GetV100Chips等）需要在头文件声明

### 检查流程（Step 3.4新增）
1. **提取函数列表**：从注册函数中提取所有TaskFuncSingle赋值的函数名
2. **逐函数检查**：使用grep搜索每个函数的跨文件调用情况
3. **判断是否删除**：若无跨文件调用，则删除对应头文件声明
4. **记录删除位置**：记录删除的头文件路径、行号、函数名

### 删除示例（davinci_multiple_task.h）
```cpp
// 删除前（Line 18）
void DavinciMultipleTaskUnInit(TaskInfo* taskInfo);  // 仅在定义文件内使用

// 删除后
// DavinciMultipleTaskUnInit仅在davinci_multiple_task_v100.cc内定义和使用
// 通过taskUnInitFunc函数指针注册，无需头文件声明
```

### 保留示例（davinci_kernel_task.h）
```cpp
// 保留声明（Line 27-28）
void ToCommandBodyForAicpuTask(TaskInfo* taskInfo, rtCommand_t *const command);  // 被v200/v201版本使用
void ToCommandBodyForAicAivTask(TaskInfo* taskInfo, rtCommand_t *const command);  // 被v200/v201版本使用，且被UT调用
```

**说明**：全局变量 `g_taskFuncArrays[CHIP_END] = {}` 初始化时所有函数指针均为nullptr，因此无需循环赋nullptr。

### v100版本默认值（task_manager.cc）

| 函数指针 | 默认值 | 说明 |
|---|---|---|
| toCommandFunc | 无默认值 | 无默认值循环，需单独赋值 |
| toSqeFunc | 无默认值 | 无默认值循环，需单独赋值 |
| taskUnInitFunc | 无默认值 | 无默认值循环（全局初始化为nullptr），需单独赋值 |
| waitAsyncCpCompleteFunc | 无默认值 | 无默认值循环（全局初始化为nullptr），需单独赋值 |
| doCompleteSuccFunc | 无默认值 | 无默认值循环，需单独赋值 |
| printErrorInfoFunc | &PrintErrorInfoCommon | 有默认值循环，先赋默认值再单独赋值 |
| setResultFunc | &SetResultCommon | 有默认值循环，先赋默认值再单独赋值 |
| setStarsResultFunc | &SetStarsResultCommon | 有默认值循环，先赋默认值再单独赋值 |

### v200_base/v200/v201版本默认值（task_manager_david.cc）

| 函数指针 | 默认值 | 说明 |
|---|---|---|
| toCommandFunc | 复用v100函数值 | 在任务文件注册函数中与其他字段一起调用RegTaskFunc注册，使用v100对应的函数值 |
| toSqeFunc | 无默认值 | David使用toDavidSqeFunc，toSqeFunc设置为nullptr |
| taskUnInitFunc | 无默认值 | 无默认值循环（全局初始化为nullptr），需单独赋值 |
| waitAsyncCpCompleteFunc | 无默认值 | 无默认值循环（全局初始化为nullptr），设置为nullptr |
| doCompleteSuccFunc | &DoCompleteSuccess | 有默认值循环，先赋默认值再单独赋值 |
| printErrorInfoFunc | &PrintErrorInfoCommon | 有默认值循环，先赋默认值再单独赋值 |
| setResultFunc | 无默认值 | 无默认值循环（全局初始化为nullptr），设置为nullptr |
| setStarsResultFunc | &SetStarsResultCommonForDavid | 有默认值循环，先赋默认值再单独赋值 |
| toDavidSqeFunc | 无默认值 | 无默认值循环，需单独赋值（stars_david.cc） |

**默认值函数声明位置**：
- `PrintErrorInfoCommon`：task_manager.h已声明
- `SetResultCommon`：task_manager.h已声明
- `SetStarsResultCommon`：task_manager.h已声明
- `DoCompleteSuccess`：task_manager.h已声明
- `SetStarsResultCommonForDavid`：stars_david.hpp已声明

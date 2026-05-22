---
name: task-migrate-refactor
description: CANN Runtime任务注册机制重构-任务迁移（以文件为粒度）
license: MIT
compatibility: opencode
version: "1.0"
parameters:
  - name: task_types
    type: array
    description: 要迁移的任务类型列表（如KERNEL_AICORE、MEMCPY等）
    required: true
  - name: skip_phase1_check
    type: boolean
    description: 是否跳过Phase1基础设施检查（默认false，需确认Phase1已完成）
    required: false
    default: false
---

## 核心工作流
1. **每步确认**：每步完成后需用户确认，**禁止**擅自进入下一步。
2. **强制Compact**：**进入下一步前必须执行 `/compact`**。
    - 保留关键输出：迁移进度表、删除代码位置表。
    - 丢弃中间推理过程。
3. **上下文管理**：若单步上下文过长，**立即触发 `/compact`**，防止上下文溢出导致规则遗忘。
4. **Config只读**：**全流程禁止修改 `config.md`**，仅允许读取。所有中间状态由主流程上下文维护。
5. **规则强制加载**：**每个步骤执行前必须使用 Read 工具读取 `skills/task/refactor/register/rules/` 中对应阶段的规则文件**。禁止凭记忆执行。
6. **执行后自检**：**每个步骤完成后必须对照对应阶段规则逐项自检**，确认无违规后方可进入下一步。自检结果需输出给用户。
7. **违规处理**：若发现未遵守规则，必须回退该步骤重新执行。**连续2次违规需暂停并请求用户介入**。

## 角色
资深C/C++和CMake开发工程师，负责将任务注册机制从"机制层集中赋值"重构为"任务文件静态注册"，严格遵循需求分析文档定义的迁移策略。**迁移必须以任务为粒度，完成该任务所有版本（v100→v200_base→v200→v201）后再迁移下一任务，每版本迁移后同步编译适配。**

## 子Skills

| Skill | 对应步骤 | 调用方式 |
|---|---|---|
| `analysis` | Step 1 | 主流程内加载执行（现状分析） |
| （无） | Step 2 | 主流程内联执行（方案设计） |
| `modify` | Step 3 | 主流程内加载执行（修改实施） |
| `../../shared/build-verify` | Step 4 | 独立Agent（编译验证） |
| `upload` | Step 5 | 独立Agent（代码上库） |

## 公共规则
所有规则定义在 `skills/task/refactor/register/rules/` 中，为本skill和所有子skill的唯一来源。规则按阶段物理隔离：
- **Step 1~3**：`rules.md`（通用约束G1~G8）
- **Step 3检查清单**：`rules/checklists/checklist-migrate.md`
- **Step 4**：`rules.md`（编译验证规则）
- **Step 5**：`rules.md`（通用约束）

## 主流程

### Step 0: 恢复上下文与配置确认
1. **读取 Config**：使用硬编码默认路径读取 Config 文件（`skills/task/shared/config.md`）。
2. **读取/初始化 Param**：读取和主skill同目录的 `param.md`，维护当前执行状态和输出文件路径。
   - 若文件不存在或为空，创建初始模板（包含task_types、当前步骤、各step输出文件路径）。
   - **Param由主流程统一读写**，子skills仅通过参数接收所需路径，不直接读写param.md。
3. **状态重置**：若检测到 `当前步骤` 不为"未开始"或"Step 0"，说明存在上次执行残留状态。**必须询问用户**："检测到上次执行残留状态（当前步骤: {当前步骤}），是否重置为初始状态？"
   - **若用户确认重置**：将 `当前步骤` 设为"Step 0"，保留 `task_types` 和输出文件路径不变。
   - **若用户选择保留**：展示当前状态，确认是否从该步骤继续。
4. **初始化/补全配置**：
   - 若Config文件不存在或为空，创建初始模板。
   - **项目根目录自动获取**：优先使用 `git rev-parse --show-toplevel` 获取当前仓库路径，自动填充到Config中。若获取失败，使用 `question` 工具提示用户手动输入。
   - 若"项目配置"区块其他字段为空（父需求链接、Token等），使用 `question` 工具提示用户输入。
5. **Phase 1基础设施检查**：
   - 若 `skip_phase1_check` 为 false，**必须确认 Phase 1 已完成**：
     - 检查 `task_manager.h` 是否包含 `TaskFuncSingle` 结构定义和 `RegTaskFunc` 接口声明
     - 检查 `task_manager.h` 是否包含 `GetV100Chips/GetDavidChips/GetV200Chips/GetV201Chips` 接口声明
     - 检查 `stars_david.hpp` 是否包含 `RegDavidSqeFunc` 接口声明
     - 若任一项缺失，**中止执行**并提示用户："Phase 1 基础设施未完成，请先执行 Phase 1"
   - 若 `skip_phase1_check` 为 true，跳过检查（适用于Phase1已确认完成的场景）
6. **强制确认机制**：
   - 将当前"项目配置"内容展示给用户（Token需脱敏显示）。
   - **必须询问用户**："请确认上述配置是否准确？"
   - **若用户确认OK**：记录确认状态，继续执行。
   - **若用户指出错误**：使用 `question` 工具获取正确值，更新Config文件，重新展示并再次确认。

### Step 1: 现状分析

> **规则加载**：必须读取 `skills/task/refactor/register/rules/rules.md`，禁止读取其他规则文件。
> **执行方式**：主流程内联执行，分析现有注册代码位置。

**目标**：梳理目标任务在机制层和任务文件中的现状分布。

**分析范围**：
- 注册入口：`task_manager.cc`（8个注册函数）、`task_manager_david.cc`（4个注册函数 + 调用v100的RegTaskToCommandFunc）、`stars_david.cc`（RegTaskToDavidSqefunc）
- 任务文件：版本目录（v100/、v200_base/、v200/、v201/）中的任务实现文件
- 头文件：`task_info.hpp`、`xx_task.h`、`stars_david.hpp` 等

**注册函数清单**：

**说明**：全局变量 `g_taskFuncArrays[CHIP_END] = {}` 初始化时所有函数指针均为nullptr，因此无需循环赋nullptr。

| 文件 | 注册函数 | 默认值机制 |
|---|---|---|
| task_manager.cc | RegTaskToCommandFunc | 无默认值循环 |
| task_manager.cc | RegTaskToSqefunc | 无默认值循环 |
| task_manager.cc | RegDoCompleteSuccFunc | 无默认值循环 |
| task_manager.cc | RegTaskUnInitFunc | 无默认值循环（全局初始化为nullptr） |
| task_manager.cc | RegPrintErrorInfoFunc | 有默认值PrintErrorInfoCommon循环 |
| task_manager.cc | RegSetResultFunc | 有默认值SetResultCommon循环 |
| task_manager.cc | RegSetStarsResultFunc | 有默认值SetStarsResultCommon循环 |
| task_manager.cc | RegWaitAsyncCpCompleteFunc | 无默认值循环（全局初始化为nullptr） |
| task_manager_david.cc | DavidRegDoCompleteSuccFunc | 有默认值DoCompleteSuccess循环 |
| task_manager_david.cc | DavidRegTaskUnInitFunc | 无默认值循环（全局初始化为nullptr） |
| task_manager_david.cc | DavidRegPrintErrorInfoFunc | 有默认值PrintErrorInfoCommon循环 |
| task_manager_david.cc | DavidRegSetStarsResultFunc | 有默认值SetStarsResultCommonForDavid循环 |
| task_manager_david.cc | RegTaskToCommandFunc（调用v100） | 调用task_manager.cc的函数，v100函数值需在任务文件中使用 |
| stars_david.cc | RegTaskToDavidSqefunc | 无默认值循环 |

**输出内容**：

#### 1.1 注册入口分析（含默认值）
| 任务类型 | 注册入口文件 | 注册函数名 | 芯片范围 | 默认值赋值行号 | 单独赋值行号 |
|---|---|---|---|---|---|
| TS_TASK_TYPE_KERNEL_AICORE | task_manager.cc | RegTaskFunc | GetV100Chips() | - | 行号 |
| TS_TASK_TYPE_KERNEL_AICORE | task_manager_david.cc | DavidRegTaskFunc | David芯片 | - | 行号 |

**默认值说明**：
- 全局变量初始化为nullptr：无需循环赋nullptr，未单独赋值的字段保持nullptr
- 非nullptr默认值循环：部分注册函数会先循环给所有任务类型赋非nullptr默认值，再单独赋值特定任务类型
- 默认值赋值行号：记录循环赋默认值的代码位置（如 `for (auto &item : xxxFunc) { item = &func; }`）
- 单独赋值行号：记录对目标任务的单独赋值位置
- 若无默认值循环，则默认值赋值行号记为 "-"
- 若目标任务依赖默认值（无单独赋值），则单独赋值行号记为 "依赖默认值"
- 若无默认值循环且无单独赋值，则赋值内容为 nullptr，单独赋值行号记为 "无赋值(nullptr)"

#### 1.2 任务文件分布
| 任务类型 | v100文件 | v200_base文件 | v200文件 | v201文件 |
|---|---|---|---|---|
| KERNEL | davinci_kernel_task_v100.cc | - | davinci_kernel_task_v200.cc | davinci_kernel_task_v201.cc |

#### 1.3 机制层赋值现状（按版本交付）
**v100版本（task_manager.cc）**：
| 函数指针 | 赋值文件 | 赋值函数 | 默认值 | 默认值行号 | 单独赋值行号 | 赋值内容 |
|---|---|---|---|---|---|---|
| toCommandFunc | task_manager.cc | RegTaskToCommandFunc | 无 | - | 行号 | &func |
| toSqeFunc | task_manager.cc | RegTaskToSqefunc | 无 | - | 行号 | &func |
| taskUnInitFunc | task_manager.cc | RegTaskUnInitFunc | 无（全局为nullptr） | - | 行号 | &func |
| waitAsyncCpCompleteFunc | task_manager.cc | RegWaitAsyncCpCompleteFunc | 无（全局为nullptr） | - | 行号 | &func |
| doCompleteSuccFunc | task_manager.cc | RegDoCompleteSuccFunc | 无 | - | 行号 | &func |
| printErrorInfoFunc | task_manager.cc | RegPrintErrorInfoFunc | &PrintErrorInfoCommon | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |
| setResultFunc | task_manager.cc | RegSetResultFunc | &SetResultCommon | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |
| setStarsResultFunc | task_manager.cc | RegSetStarsResultFunc | &SetStarsResultCommon | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |

**v200版本（task_manager_david.cc + stars_david.cc）**：
| 函数指针 | 赋值文件 | 赋值函数 | 默认值 | 默认值行号 | 单独赋值行号 | 赋值内容 |
|---|---|---|---|---|---|---|
| toCommandFunc | task_manager_david.cc | RegTaskToCommandFunc（调用v100） | 复用v100函数值 | - | - | 使用v100对应函数值，在任务文件注册函数中一起注册 |
| toSqeFunc | task_manager_david.cc | 无注册函数 | 无 | - | - | 无赋值（David使用toDavidSqeFunc） |
| taskUnInitFunc | task_manager_david.cc | DavidRegTaskUnInitFunc | 无（全局为nullptr） | - | 行号 | &func |
| waitAsyncCpCompleteFunc | task_manager_david.cc | 无注册函数 | 无（全局为nullptr） | - | - | 无赋值 |
| doCompleteSuccFunc | task_manager_david.cc | DavidRegDoCompleteSuccFunc | &DoCompleteSuccess | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |
| printErrorInfoFunc | task_manager_david.cc | DavidRegPrintErrorInfoFunc | &PrintErrorInfoCommon | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |
| setResultFunc | task_manager_david.cc | 无注册函数 | 无（全局为nullptr） | - | - | 无赋值 |
| setStarsResultFunc | task_manager_david.cc | DavidRegSetStarsResultFunc | &SetStarsResultCommonForDavid | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |
| toDavidSqeFunc | stars_david.cc | RegTaskToDavidSqefunc | 无 | - | 行号 | &func |

**v201版本（task_manager_david.cc + stars_david.cc）**：
| 函数指针 | 赋值文件 | 赋值函数 | 默认值 | 默认值行号 | 单独赋值行号 | 赋值内容 |
|---|---|---|---|---|---|---|
| toCommandFunc | task_manager_david.cc | RegTaskToCommandFunc（调用v100） | 复用v100函数值 | - | - | 使用v100对应函数值，在任务文件注册函数中一起注册 |
| toSqeFunc | task_manager_david.cc | 无注册函数 | 无 | - | - | 无赋值（David使用toDavidSqeFunc） |
| taskUnInitFunc | task_manager_david.cc | DavidRegTaskUnInitFunc | 无（全局为nullptr） | - | 行号 | &func |
| waitAsyncCpCompleteFunc | task_manager_david.cc | 无注册函数 | 无（全局为nullptr） | - | - | 无赋值 |
| doCompleteSuccFunc | task_manager_david.cc | DavidRegDoCompleteSuccFunc | &DoCompleteSuccess | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |
| printErrorInfoFunc | task_manager_david.cc | DavidRegPrintErrorInfoFunc | &PrintErrorInfoCommon | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |
| setResultFunc | task_manager_david.cc | 无注册函数 | 无（全局为nullptr） | - | - | 无赋值 |
| setStarsResultFunc | task_manager_david.cc | DavidRegSetStarsResultFunc | &SetStarsResultCommonForDavid | 行号 | 行号或"依赖默认值" | &func或依赖默认值 |
| toDavidSqeFunc | stars_david.cc | RegTaskToDavidSqefunc | 无 | - | 行号 | &func |

**输出文件**：`docs/register/step1_migrate_output.md`

**用户确认**：
> "Step 1 完成。请确认现状分析是否覆盖所有目标task_types，注册入口是否均已定位。确认OK后进入Step 2。"

### Step 2: 方案设计（主流程内联执行）

> **规则加载**：必须读取 `skills/task/refactor/register/rules/rules.md`，禁止读取其他规则文件。
> **前置条件**：必须确认 Step 1 已完成。

**目标**：设计任务迁移方案，确定每个版本的迁移步骤。

**设计内容**：

#### 2.1 迁移方案表
| 任务类型 | 版本 | 操作 | 涉及文件 | 注册函数名 | 芯片范围接口 |
|---|---|---|---|---|---|
| KERNEL | v100 | 新增注册 | davinci_kernel_task_v100.cc | DavinciKernelTaskRegister | GetV100Chips() |
| KERNEL | v100 | 删除赋值 | task_manager.cc | - | - |
| KERNEL | v200 | 新增注册 | davinci_kernel_task_v200.cc | DavinciKernelTaskRegister | GetV200Chips() |
| KERNEL | v200 | 删除赋值 | task_manager_david.cc | - | - |
| KERNEL | v200 | 删除SQE赋值 | stars_david.cc | - | - |
| KERNEL | v201 | 新增注册 | davinci_kernel_task_v201.cc | DavinciKernelTaskRegister | GetV201Chips() |
| KERNEL | v201 | 删除赋值 | task_manager_david.cc | - | - |
| KERNEL | v201 | 删除SQE赋值 | stars_david.cc | - | - |

#### 2.2 TaskFuncSingle字段设计
| 任务类型 | toCommandFunc | toSqeFunc | doCompleteSuccFunc | taskUnInitFunc | waitAsyncCpCompleteFunc | printErrorInfoFunc | setResultFunc | setStarsResultFunc |
|---|---|---|---|---|---|---|---|---|
| KERNEL_AICORE | &ToCommandBodyForAicAivTask | nullptr | &DoCompleteSuccessForDavinciTask | &DavinciTaskUnInit | nullptr | &PrintErrorInfoForDavinciTask | nullptr | nullptr |

**TaskFuncSingle字段赋值规则**：
- 若有单独赋值：使用单独赋值的函数
- 若无单独赋值但有默认值循环：使用默认值函数（不能设置为nullptr，否则会覆盖默认值）
- 若无单独赋值也无默认值：设置为nullptr

#### 2.3 编译适配方案
| 任务类型 | David编译配置 | 需移除的v100依赖 | CMake文件 |
|---|---|---|---|
| KERNEL | v200.cmake | davinci_kernel_task_v100.cc | cmake/v200.cmake |

**输出文件**：`docs/register/step2_migrate_output.md`

**用户确认**：
> "Step 2 完成。请确认迁移方案是否正确，TaskFuncSingle字段是否完整。确认OK后进入Step 3。"

**步骤检查**：使用 Read 工具读取 `skills/task/refactor/register/rules/checklists/checklist-migrate.md`，逐项勾选设计相关检查项，将结果写入 `docs/register/step2_migrate_checklist.md`。

### Step 3: 修改实施

> **规则加载**：必须读取 `skills/task/refactor/register/rules/rules.md`，禁止读取其他规则文件。
> **前置条件**：必须确认 `docs/register/step2_migrate_output.md` 已存在。
> **执行方式**：按任务粒度顺序执行，单任务完成所有版本后继续下一任务。

**核心约束**：
- **任务粒度迁移**：必须完成当前任务的所有版本（v100→v200_base→v200→v201）后再迁移下一任务
- **先增后删**：先新增注册代码，编译通过后再删除旧赋值代码
- **每版本编译验证**：每个版本迁移后立即编译验证

#### 3.1 单任务迁移流程

**v100版本迁移**：
1. 在任务文件末尾新增注册函数（按T2规则结构）
2. 编译验证（确保新增代码编译通过）
3. 删除机制层赋值行（仅删除当前任务相关赋值）
4. 删除不必要的头文件声明（按G13规则检查，参照`skills/task/refactor/register/rules/checklists/checklist-g13-header-cleanup.md`）
5. 记录删除位置（文件路径、行号、删除内容摘要）
6. 编译验证
7. UT验证（runtime_ut_910b）

**v200版本迁移**：
1. 在任务文件末尾新增注册函数（按T4规则结构，调用RegDavidSqeFunc）
2. 编译验证
3. 删除机制层赋值行（task_manager_david.cc）
4. 删除SQE赋值行（stars_david.cc）
5. 删除不必要的头文件声明（按G13规则检查，参照`skills/task/refactor/register/rules/checklists/checklist-g13-header-cleanup.md`）
6. 编译适配（移除David对xx_v100.cc依赖）
7. 编译验证
8. UT验证（runtime_ut_david）

**v201版本迁移**：
1. 在任务文件末尾新增注册函数（按T5规则结构，调用RegDavidSqeFunc）
2. 编译验证
3. 删除机制层赋值行（task_manager_david.cc）
4. 删除SQE赋值行（stars_david.cc）
5. 删除不必要的头文件声明（按G13规则检查，参照`skills/task/refactor/register/rules/checklists/checklist-g13-header-cleanup.md`）
6. 编译适配
7. 编译验证
8. UT验证（runtime_ut_v201）

#### 3.2 删除代码记录表
| 文件 | 函数名 | 删除行号 | 删除内容摘要 |
|---|---|---|---|
| task_manager.cc | TaskFuncReg | 行号 | KERNEL任务的toCommandFunc赋值 |
| davinci_multiple_task.h | DavinciMultipleTaskUnInit | Line 18 | 仅在定义文件内使用的taskUnInitFunc |

#### 3.3 迁移进度表
| 任务名称 | v100状态 | v200_base状态 | v200状态 | v201状态 | 编译状态 | UT状态 |
|---|---|---|---|---|---|---|
| KERNEL | 完成 | - | 完成 | 完成 | 通过 | 通过 |

**输出文件**：`docs/register/step3_migrate_output.md`

**用户确认**：
> 每完成一个任务的所有版本迁移后，展示进度表，询问用户："任务{任务名}迁移完成，请确认进度表是否正确。确认OK后继续下一任务。"

**步骤检查**：每完成一个任务，读取 `skills/task/refactor/register/rules/checklists/checklist-migrate.md`，逐项勾选，将结果追加写入 `docs/register/step3_migrate_checklist.md`。

### Step 4: 编译与UT验证（独立Agent）

> **前置拦截**：从 `docs/register/step3_migrate_output.md` 读取迁移进度，确认所有任务编译状态为"通过"。

**隔离执行**：启动**新Agent**调用子skill `skills/task/shared/build-verify.skill`，避免编译日志污染主流程上下文。

**参数传递**：从 `skills/task/shared/config.md` 读取路径，传入子skill：
- `task_types`：从param.md读取
- `config_file`：`skills/task/shared/config.md`
- `build_verify_result_file`：`docs/register/build_verify_migrate_result.md`
- `rules_file`：`skills/task/refactor/register/rules/rules.md`
- `output_dir_prefix`：`docs/register`

**强制UT验证**：子skill必须按顺序执行4个子测试套：
- runtime_ut_common
- runtime_ut_910b
- runtime_ut_david
- runtime_ut_v201

**输出**：子skill将编译和UT结果写入 `docs/register/build_verify_migrate_result.md`。

**同步**：主流程读取结果摘要，上下文记录 `build_verify_status` 和 `ut_verify_status`。

**失败处理**：若编译或UT失败，根据错误日志修复代码后，重新执行Step 3 → Step 4。**若同一错误连续出现2次，暂停执行并请求用户介入决策**。

### Step 5: 代码上库

> **前置拦截**：从 `docs/register/build_verify_migrate_result.md` 读取 `build_verify_status` 和 `ut_verify_status`。若任一不满足'成功'，中止执行并报告错误。

**执行内容**：

#### 5.1 Git提交准备
1. 查看修改：`git status`
2. 添加文件：**仅添加重构涉及的源文件、头文件和CMake文件**，使用 `git add <具体文件路径>` 逐个添加。
   - **禁止提交的路径**：
     - `docs/` 下的所有文件
     - `skills/` 下的所有文件
     - 任何 `.md` 文档（除非是代码注释相关的头文件）
   - **允许提交的路径**：
     - `src/runtime/` 下的 `.cc` 和 `.h` 文件
     - `cmake/` 下的 `.cmake` 文件
     - `tests/ut/` 下的 `.cc` 和 `CMakeLists.txt` 文件
   - **验证**：执行 `git status --short` 认仅包含允许路径，**禁止使用 `git add .` 或 `git add -A`**
3. 提交：
   - 构造 Commit Message：将 `task_types` 数组用逗号连接，格式为 `"refactor(task): {task_types}任务注册机制重构"`
   - 判断 HEAD 是否已 push：`git log origin/<目标分支>..<目标分支> --oneline | wc -l`
     - 若结果 > 0 → HEAD 未 push，可 amend
     - 若结果 = 0 → HEAD 已 push，禁止 amend
   - 检查 HEAD commit message 是否包含当前 `task_types`：
     - 若匹配且 HEAD 未 push → `git commit --amend -m "<Commit Message>"`
     - 否则 → `git commit -m "<Commit Message>"`
4. 上传：`git push origin <目标分支>`
   - **若 push 失败，立即停止，提示用户手动解决**

#### 5.2 创建PR
1. 从 `skills/task/shared/config.md` 读取 `parent_issue_url`、`gitcode_token`、`repo`、`owner`
2. API: `POST https://gitcode.com/api/v5/repos/{owner}/{repo}/pulls`
3. Headers: `{"PRIVATE-TOKEN": "<gitcode_token>"}`
4. Body:
   - `title`：`"refactor(task): {task_types}任务注册机制重构"`
   - `head`：`"turing_yhy/runtime_task1:master"`（从config.md读取实际分支）
   - `base`：`"master"`
   - `body`：PR描述（严格按模板）
5. 解析响应获取 `web_url` 作为 PR URL

**PR描述模板**：
```markdown
# Pull Request

## 描述
将 {task_types} 任务类型的操作函数按硬件版本(v100/v200/v201)分离，实现任务注册机制重构。

**重构范围**：
- {task_types}任务迁移（以文件为粒度）

## 关联的Issue
- **父需求（需求来源）**：[Issue #{parent_issue_number}](<parent_issue_url>)

Related #{parent_issue_number}

## 变更类型
- [x] ♻️ 重构

## 如何测试
1. **编译验证**：`bash build.sh`
2. **UT验证**：依次执行以下4个子测试套：
   - `bash tests/build_ut.sh --ut=runtime --target=runtime_ut_common -c`
   - `bash tests/build_ut.sh --ut=runtime --target=runtime_ut_910b -c`
   - `bash tests/build_ut.sh --ut=runtime --target=runtime_ut_david -c`
   - `bash tests/build_ut.sh --ut=runtime --target=runtime_ut_v201 -c`

## 核对清单
- [x] 我的代码遵循了项目的代码风格
- [x] 我已对代码进行了自测
- [x] 我已更新了相关的文档
- [x] 我在标题中使用了合适的类型标签（refactor:）
- [x] 我已经详细阅读了贡献指南

## 其他信息

### 迁移进度
| 任务名称 | v100状态 | v200_base状态 | v200状态 | v201状态 |
|---|---|---|---|---|
{从step3_migrate_output.md读取进度表}

### 删除代码位置
{从step3_migrate_output.md读取删除记录表}
```

**输出文件**：`docs/register/pr_migrate_url.md`

### Step 6: 更新输出文件
- 更新 `skills/task/refactor/register/param.md` 中的 `当前步骤` 为"已完成"
- **禁止修改 `skills/task/shared/config.md`**（Config仅Step 0读取确认，全程只读）

## 任务迁移顺序表

| 序号 | 任务名称 | v100文件 | v200_base文件 | v200文件 | v201文件 |
|---|---|---|---|---|---|
| 1 | KERNEL (AICORE/AICPU/AIVEC) | davinci_kernel_task_v100.cc | - | davinci_kernel_task_v200.cc | davinci_kernel_task_v201.cc |
| 2 | MEMCPY | memory_task_v100.cc | memory_task_v200_base.cc | - | - |
| 3 | EVENT (RECORD/WAIT) | event_task_v100.cc | event_task_v200_base.cc | - | - |
| 4 | NOTIFY (RECORD/WAIT) | notify_task_v100.cc | notify_task_v200_base.cc | - | - |
| 5 | BARRIER | barrier_task_v100.cc | barrier_task_v200_base.cc（需新增） | - | - |
| 6 | REDUCE | reduce_task_v100.cc | reduce_task_v200_base.cc（需新增） | - | - |
| 7 | RDMA | rdma_task_v100.cc | rdma_task_v200_base.cc（需新增） | - | - |

## 注册函数模板

### v100版本
```cpp
static bool {TaskName}TaskRegister()
{
    TaskFuncSingle funcs = {
        .toCommandFunc = &func1,
        .toSqeFunc = &func2,
        .doCompleteSuccFunc = &func3,
        .taskUnInitFunc = &func4,
        .waitAsyncCpCompleteFunc = nullptr,
        .printErrorInfoFunc = &func5,
        .setResultFunc = nullptr,
        .setStarsResultFunc = nullptr,
    };
    
    const auto& chips = GetV100Chips();
    for (auto chip : chips) {
        RegTaskFunc(chip, TS_TASK_TYPE_XXX, funcs);
    }
    
    return true;
}

static bool g_{taskName}TaskRegister = {TaskName}TaskRegister();
```

### v200/v201版本
```cpp
static bool {TaskName}TaskRegister()
{
    TaskFuncSingle funcs = {
        .toCommandFunc = &func1,
        .toSqeFunc = nullptr,  // David使用g_toDavidSqeFunc
        .doCompleteSuccFunc = &func3,
        .taskUnInitFunc = &func4,
        .waitAsyncCpCompleteFunc = nullptr,
        .printErrorInfoFunc = &func5,
        .setResultFunc = nullptr,
        .setStarsResultFunc = nullptr,
    };
    
    const auto& chips = GetV200Chips();  // 或 GetV201Chips()
    for (auto chip : chips) {
        RegTaskFunc(chip, TS_TASK_TYPE_XXX, funcs);
    }
    
    RegDavidSqeFunc(TS_TASK_TYPE_XXX, &toDavidSqeFunc);
    
    return true;
}

static bool g_{taskName}TaskRegister = {TaskName}TaskRegister();
```

## 错误处理机制

| 错误类型 | 排查方向 | 对应规则 |
|---|---|---|
| 编译失败-undefined reference | CMake遗漏文件或include缺失 | T10 |
| 编译失败-multiple definition | 同名注册函数跨文件共享 | T13 |
| UT失败-旧接口测试失败 | 旧接口UT未删除 | T16 |
| UT失败-新注册未生效 | 注册函数未正确调用 | T2/T4/T5 |
| Git push失败 | 权限或远程冲突 | G6 |

## 强制约束检查（每步骤后必须自检）

- [ ] 以任务为粒度迁移，未跨任务
- [ ] 每版本迁移后立即编译验证
- [ ] 先增后删，先新增注册再删除赋值
- [ ] 删除位置已记录（文件、行号）
- [ ] TaskFuncSingle字段完整（8个字段）
- [ ] v200/v201调用RegDavidSqeFunc
- [ ] 同名注册函数独立定义，不跨文件共享
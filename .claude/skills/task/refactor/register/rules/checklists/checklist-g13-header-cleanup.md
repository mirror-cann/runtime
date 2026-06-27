# G13规则检查清单 - TaskFuncSingle赋值函数头文件声明清理

## 检查流程

### 1. 提取TaskFuncSingle赋值函数列表
从注册函数中提取所有赋值给TaskFuncSingle的函数名：

```bash
# 优先使用rg（推荐）
rg -n "TaskFuncSingle|\\.toCommandFunc|\\.toSqeFunc|\\.doCompleteSuccFunc|\\.taskUnInitFunc|\\.waitAsyncCpCompleteFunc|\\.printErrorInfoFunc|\\.setResultFunc|\\.setStarsResultFunc|RegDavidSqeFunc" <task_file>

# grep fallback示例（KERNEL任务）
grep -A 10 "TaskFuncSingle.*funcs.*=" davinci_kernel_task_v100.cc | grep "&" | sed 's/.*&\([a-zA-Z_]*\).*/\1/'
```

### 2. 逐函数检查跨文件调用

#### 2.1 检查命令
```bash
# 推荐命令
rg -n "FunctionName\\b" src tests

# grep fallback：检查函数跨文件调用（排除定义文件）
grep -r "FunctionName" --include="*.cc" --include="*.cpp" | grep -v "定义文件路径"
```

#### 2.2 判断标准
- **无跨文件调用**：搜索结果为空或仅包含函数指针赋值（机制层已删除）
- **有跨文件调用**：搜索结果包含其他源文件或UT文件的直接调用
- **跨版本使用**：搜索结果包含v200/v201版本文件

### 3. 检查头文件声明

#### 3.1 查找头文件声明
```bash
# 推荐命令
rg -n "FunctionName\\b" src/runtime/core/src/task/inc src/runtime/core/inc src/runtime/feature

# grep fallback：在头文件中搜索函数声明
grep "FunctionName" task/inc/*.h
```

#### 3.2 删除判断
- 若函数无跨文件调用，删除头文件声明
- 若函数有跨文件调用，保留头文件声明
- 若函数被UT、跨版本文件或非注册逻辑直接调用，保留头文件声明
- 删除头文件声明后，若函数只在定义文件内使用，应同步将函数定义改为 `static`

### 3.3 删除声明后的符号收敛
```bash
# 检查定义是否仍为外部链接
rg -n "\\bFunctionName\\s*\\(" <definition_file>
```

说明：该命令用于定位函数名出现位置，再结合上下文判断是定义、声明还是调用，避免因缩进、复杂返回类型、宏或属性导致漏检。

判断标准：
- 已在匿名namespace或已声明为 `static`：无需修改
- 删除声明后只在定义文件内使用：改为 `static`
- 仍被其他源文件、UT或跨版本文件直接调用：不能改 `static`，应保留头文件声明

### 4. 记录删除位置

| 文件 | 函数名 | 行号 | 删除原因 | 定义是否改static |
|---|---|---|---|---|
| davinci_multiple_task.h | DavinciMultipleTaskUnInit | Line 18 | 仅在v100.cc内使用 | 是 |

## 典型案例分析

### KERNEL任务（davinci_kernel_task_v100.cc）

**TaskFuncSingle赋值函数**：
1. `ToCommandBodyForAicAivTask` - 被v200/v201版本使用 + UT调用 → **保留声明**
2. `ToCommandBodyForAicpuTask` - 被v200/v201版本使用 → **保留声明**
3. `ConstructAicAivSqeForDavinciTask` - 被UT调用 → **保留声明**
4. `DoCompleteSuccessForDavinciTask` - 被UT调用 → **无声明（已存在davinci_kernel_task.h无需删除）**
5. `DavinciTaskUnInit` - 被UT调用 → **无声明（已存在无需删除）**
6. `WaitAsyncCopyCompleteForDavinciTask` - 无跨文件调用 → **无声明**
7. `PrintErrorInfoForDavinciTask` - 被v200/v201版本使用 → **保留声明**
8. `SetResultForDavinciTask` - 无跨文件调用 → **无声明**
9. `SetStarsResultForDavinciTask` - 被UT调用 → **无声明**

**检查结果**：davinci_kernel_task.h中所有声明均为必要，无需删除。

### MULTIPLE_TASK任务（davinci_multiple_task_v100.cc）

**TaskFuncSingle赋值函数**：
1. `ConstructSqeForDavinciMultipleTask` - 无跨文件调用 → **删除声明**
2. `DavinciMultipleTaskUnInit` - 无跨文件调用 → **删除声明**
3. `WaitAsyncCopyCompleteForDavinciMultipleTask` - 无跨文件调用 → **删除声明**

**其他函数（非TaskFuncSingle赋值但需检查）**：
1. `ResetCmdList` - 被dvpp_stars.cc等调用 → **保留声明**
2. `IncMultipleTaskCqeNum` - 被多处调用 → **保留声明**
3. 其他辅助函数 - 均有跨文件调用 → **保留声明**

**检查结果**：删除 `DavinciMultipleTaskUnInit` 声明（Line 18）。

## 检查清单模板

### v100版本检查清单

```markdown
## {任务名}任务 - TaskFuncSingle赋值函数检查

### 函数列表
1. [ ] Function1
   - 搜索检查结果：[有/无]跨文件调用
   - 头文件声明：[有/无]
   - 决策：[保留/删除]声明

2. [ ] Function2
   - 搜索检查结果：[有/无]跨文件调用
   - 头文件声明：[有/无]
   - 决策：[保留/删除]声明

...

### 删除操作记录
| 文件 | 函数名 | 行号 | 删除原因 | 定义是否改static |
|---|---|---|---|---|
```

### v200/v201版本检查清单

同v100版本，但需额外检查：
- StarsV2版本函数（如StarsV2DoCompleteSuccessForDavinciTask）
- 是否有跨版本调用（v200调用v201函数等）

## 执行时机

- **v100迁移后**：Step 3.4 执行G13检查
- **v200迁移后**：Step 3.5 执行G13检查
- **v201迁移后**：Step 3.5 执行G13检查

## 错误处理

| 错误类型 | 原因 | 修复方案 |
|---|---|---|
| 编译失败 - undefined reference | 删除了有跨文件调用的函数声明 | 恢复头文件声明 |
| 编译失败 - 多余声明警告 | 保留了无跨文件调用的函数声明 | 删除头文件声明 |
| 编译失败 - 未声明标识符 | 删除了仍被其他文件直接调用的声明 | 恢复头文件声明或补充正确include |
| UT失败 | 删除了UT测试中直接调用的函数声明 | 恢复头文件声明 |
| 符号仍全局暴露 | 删除声明后未将仅文件内使用函数改为static | 将定义改为static |

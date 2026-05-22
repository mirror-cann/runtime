# Skills 架构说明

## 目录结构

```
skills/
├── shared/                          # 共享资源（跨任务复用）
│   ├── build-verify.skill           # 编译验证（分层重构专用）
│   ├── config.md                    # 项目配置（共用）
│   └── README.md                    # 本文件
│
└── {task}/                          # 各任务目录（物理隔离）
    ├── SKILL.md                     # 主Skill入口
    ├── build-verify.skill           # 编译验证（任务定制版）
    ├── {子skill}.skill              # 子Skills
    ├── rules/                       # 规则（本任务专属）
    │   ├── rules.md                 # 通用规则
    │   ├── active_constraints.md    # 活跃约束
    │   └── checklists/              # 检查清单
    ├── param.md                     # 执行状态
    └── ...
```

## 角色定义

| 文件类型 | 角色 | 核心职责 | 比喻 |
|---|---|---|---|
| **Main Skill** | 项目经理 | 定义整体工作流、全局约束、步骤流转、用户交互点、输入输出文件管理 | 管流程、管交付物 |
| **子 Skills** | 施工队长 | 定义具体步骤的任务拆解、执行顺序/依赖、输入输出格式、规则引用 | 管具体怎么干、谁先谁后 |
| **Rules** | 技术规范 | 定义判断逻辑、填写规范、命名规则、修改约束等 | 管对错标准、法律依据 |

## 三者关联

1. **Main Skill** 是入口，规定"现在进入 Step X"，并划定范围（`task_types`）
2. **Main Skill** 指挥去读对应**子 Skill** 获取具体的执行计划
3. **子 Skill** 在执行过程中，指挥读取对应 **Rules** 获取判断标准和操作规范

## 共享资源

| 文件 | 用途 | 说明 |
|---|---|---|
| `config.md` | 项目配置（根目录、Token、仓库等） | 所有任务共用，仅Step 0读取确认 |
| `build-verify.skill` | 编译验证 + UT测试（分层重构版） | 分层重构任务专用，其他任务需自建 |

## 维护原则

1. **Main Skill** 禁止包含具体的任务定义、判断逻辑、分布表模板、规则编号
2. **子 Skills** 的执行流程必须显式引用 Rules 并强调强制性
3. **Rules** 禁止包含流程控制（如"先执行A再执行B"）
4. **规则优化**：禁止删除现有内容，冲突需建立优先级，优化后必须更新版本号
5. **物理隔离**：不同任务的skills和rules严格物理隔离，禁止跨目录引用

## 文件命名规范

| 类型 | 命名格式 | 示例 |
|---|---|---|
| Main Skill | `SKILL.md` | `SKILL.md` |
| 子 Skill | `{功能}.skill` | `analysis.skill`, `modify.skill` |
| 规则 | `rules.md` | `rules.md` |
| 检查清单 | `checklist-{功能}.md` | `checklist-migrate.md` |
| 状态文件 | `param.md` | `param.md` |

## 禁止事项

- 跨任务目录引用规则
- 修改 `shared/config.md`（仅Step 0读取确认）
- Main Skill 直接定义规则编号
- Rules 包含流程控制语句
- shared目录放置任务定制内容
---
name: runtime-llt-run
description: 对本次代码修改执行精准 LLT（Low Level Test）验收。筛选相关测试用例并定向运行，以构建和用例执行结果判断是否通过；默认不采集覆盖率，只有用户明确要求覆盖率、增量覆盖率或 coverage 时才采集 changed-line 增量覆盖率且仅报告不参与 PASS/FAIL 判定。当用户请求运行 LLT、运行 /runtime-llt-run、或需要验证代码修改的测试覆盖时触发。
---

# Runtime LLT 精准验收

LLT 全量运行不稳定（易超时/coredump），采用变更驱动的精准用例筛选 + 定向运行策略，避免全量执行。

## 默认入口

日常验收只需要在 Runtime 仓根目录执行：

```bash
python3 .claude/skills/runtime-llt-run/scripts/runtime_llt_select_and_run.py
```

该入口默认值如下，非特殊场景不要显式传参：

- `--repo .`
- `--base $BASE_REF`，未设置时自动使用 `git merge-base HEAD origin/master`，再依次回退到 `origin/main`、`master`、`main`、`HEAD~1`
- `--out-dir llt_verify`
- `--test-root tests/ut/runtime/runtime/test`
- `--build-dir build_ut`
- `--build-target auto`，使用一组本地稳定 gtest 小目标，避免直接拉起全量 `runtime_ut`
- `--max-tests 120`
- `--reruns 1`

只查看用例推荐和门禁计划，不构建、不执行时：

```bash
python3 .claude/skills/runtime-llt-run/scripts/runtime_llt_select_and_run.py --plan-only
```

复用已有构建产物快速执行时：

```bash
python3 .claude/skills/runtime-llt-run/scripts/runtime_llt_select_and_run.py --skip-build
```

用户明确要求覆盖率或增量覆盖率时：

```bash
python3 .claude/skills/runtime-llt-run/scripts/runtime_llt_select_and_run.py --coverage
```

此时 `--coverage-threshold 80.0` 仅用于增量覆盖率报告展示，不参与 PASS/FAIL 判定。

仅当需要覆盖特定基线、平台目标或测试目录时才传入参数，例如：

```bash
python3 .claude/skills/runtime-llt-run/scripts/runtime_llt_select_and_run.py \
  --base origin/master \
  --build-target runtime_utest_api_910B
```

## 自动化闭环

统一脚本必须完成以下动作：

1. 基于 `git diff` 识别变更文件、变更行、关键符号和接口名。
2. 仅当变更命中 Runtime 源码、头文件或 Runtime UT 时推荐用例；纯文档、example、配置、skill 等非 Runtime LLT 变更必须明确跳过。
3. 建立源码到 UT 的候选映射，综合符号命中、include 命中、文件名/路径邻近关系和可选历史覆盖映射，不只依赖函数名搜索。
4. 自动生成精确 `gtest_filter`，默认最多选择 120 个高分候选；auto 模式下最终只保留本地已发现 gtest 可执行文件中真实存在的用例，避免平台专用候选误报 missing；候选过少时自动补同 suite 级别兜底 filter。
5. 本地默认只构建一组稳定 gtest 小目标并执行定向用例；平台专用目标由 `--build-target` 显式指定。
6. 自动保存 gtest XML，失败时自动复跑并把复跑通过标记为 flaky 风险，不把单次复跑通过直接当作稳定通过。
7. 默认不收集覆盖率；仅当用户明确要求覆盖率、增量覆盖率或 coverage 时，使用 `--coverage` 收集 `lcov` 并按本次 changed-line 计算增量覆盖率结果。覆盖率仅用于报告展示，不参与 PASS/FAIL 判定。
8. 输出 `llt_verify/scope.json`、`llt_verify/result.json`、`llt_verify/xml/` 和中文 Markdown 报告 `llt_verify/report_<timestamp>.md`；只有执行 `--coverage` 时才额外输出 `llt_verify/coverage.info` 和 `llt_verify/coverage_html/`。
9. `report_<timestamp>.md` 必须使用中文标题、中文章节和中文字段说明；`PASS`、`FAIL`、`SKIPPED`、`reason`、路径、命令和 gtest 用例名等机器可消费或定位信息可保留原文。
10. 如果用户没有明确要求覆盖率、增量覆盖率或 coverage，最终回复和 `report_<timestamp>.md` 都不要出现覆盖率相关章节或说明；只报告变更范围、用例推荐、构建、用例执行、结论和产物。

## 结论处理

读取 `llt_verify/result.json` 和最新 `llt_verify/report_<timestamp>.md`，对外只使用三种结论：

- `PASS`：相关用例全部稳定通过。
- `FAIL`：找到 Runtime LLT 相关变更，但用例推荐、构建或执行未满足通过条件；具体原因看 `reason` 和报告详情。
- `SKIPPED`：仅文档、example、配置、skill 或非 Runtime 组件等非 Runtime LLT 相关变更，或仅执行 `--plan-only`。

`reason` 用于保留细分诊断，例如 `no_related_tests`、`build_failed`、`test_not_run`、`selected_tests_missing`、`tests_failed`、`flaky_risk`、`non_llt_change`、`plan_only`。

## 缺口整改

当结论不是 `PASS` 或明确可跳过时，按报告中的具体缺口整改：

- 没有候选用例：先基于 `scope.json` 的变更符号、include 和变更文件补充映射；仍缺失时新增 UT。
- 用例失败：先确认失败是否由本次变更引入；修复代码 bug 或同步调整 UT，再重新运行统一入口。
- 增量覆盖率不足：作为改进建议记录，优先覆盖本次新增/修改行、错误路径和边界分支；不要用全仓总覆盖率替代 changed-line 增量覆盖率。
- 偶发失败：保留 XML、stdout/stderr tail 和复跑记录；不能仅用“单独运行通过”覆盖失败证据。

## 通过标准

仅当以下条件同时满足时判定 LLT 通过：

- `llt_verify/scope.json` 保留变更范围、候选依据和最终 `gtest_filter`。
- 相关 gtest 可执行文件已定向执行，XML 结果已保存，且没有失败、超时或未归因异常。
- `llt_verify/result.json` 的 `conclusion` 为 `PASS`。
- 最新 `llt_verify/report_<timestamp>.md` 使用中文可复现地说明改动识别、用例推荐、执行结果和最终门禁结论；若用户要求覆盖率，还需说明 changed-line 增量覆盖率参考结果。

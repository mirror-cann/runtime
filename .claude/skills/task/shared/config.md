# 任务管理器重构项目配置

## 项目配置
- **项目根目录**: <Step 0 时通过 `git rev-parse --show-toplevel` 自动获取，或手动填写>
- **父需求链接**: <Step 0 时由用户输入填充>
- **Token**: <Step 0 时由用户输入填充，GitCode API 私有令牌>
- **仓库**: runtime
- **Owner**: cann
- **分支**: master

## 编译配置
- **编译命令**: bash build.sh

## UT测试配置
- **runtime_ut_common命令**: bash tests/build_ut.sh --ut=runtime --target=runtime_ut_common -c
- **runtime_ut_910b命令**: bash tests/build_ut.sh --ut=runtime --target=runtime_ut_910b -c
- **runtime_ut_david命令**: bash tests/build_ut.sh --ut=runtime --target=runtime_ut_david -c
- **runtime_ut_v201命令**: bash tests/build_ut.sh --ut=runtime --target=runtime_ut_v201 -c

## 备注
- 本文件由Step 0读取确认，全程只读，禁止修改
- 缺失字段由用户输入填充
- 项目根目录建议使用 `git rev-parse --show-toplevel` 自动获取当前仓库路径
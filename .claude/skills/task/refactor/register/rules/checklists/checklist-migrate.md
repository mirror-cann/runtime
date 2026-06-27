# 任务迁移检查清单

## 版本信息
- **版本**: 1.1
- **更新日期**: 2026-06-25

---

## 任务粒度迁移
- [ ] 已完成当前任务所有版本迁移（v100→v200_base→v200→v201）
- [ ] 未跨任务迁移
- [ ] 已确认实际注册落点：有v200/v201叶子文件时不在v200_base重复注册；只有v200_base时才使用GetDavidChips()
- [ ] 若为feature特例（DQS/XPU/FFTS等），已记录特例原因和注册方式

## v100版本迁移
- [ ] 注册函数已添加到任务文件末尾
- [ ] 调用GetV100Chips()获取芯片范围
- [ ] task_manager.cc中对应赋值行已删除
- [ ] 仅删除当前任务赋值行，未删除整个函数
- [ ] v100编译验证通过

## v200版本迁移
- [ ] 注册函数已添加到任务文件末尾
- [ ] 调用GetV200Chips()获取芯片范围
- [ ] 旧机制层存在David SQE赋值时已调用RegDavidSqeFunc注册David SQE；无旧SQE赋值时已记录“不新增”
- [ ] RegDavidSqeFunc位于芯片循环外
- [ ] task_manager_david.cc中对应赋值行已删除
- [ ] stars_david.cc中对应SQE赋值行已删除
- [ ] David编译配置已移除xx_v100.cc依赖
- [ ] v200编译验证通过

## v201版本迁移
- [ ] 注册函数已添加到任务文件末尾
- [ ] 调用GetV201Chips()获取芯片范围
- [ ] 旧机制层存在David SQE赋值时已调用RegDavidSqeFunc注册David SQE；无旧SQE赋值时已记录“不新增”
- [ ] RegDavidSqeFunc位于芯片循环外
- [ ] task_manager_david.cc中对应赋值行已删除（如有）
- [ ] stars_david.cc中对应SQE赋值行已删除（如有）
- [ ] v201编译验证通过

## 删除代码记录
- [ ] 每删除行已记录文件路径和行号
- [ ] 仅删除当前迁移任务的赋值行
- [ ] 未删除任务名称/描述表、日志诊断映射、非注册功能逻辑
- [ ] include删除前已确认不存在非注册路径直接调用

## TaskFuncSingle填充
- [ ] 使用指定初始化器语法
- [ ] 字段完整性（8个字段）
- [ ] 空指针字段填充nullptr
- [ ] 非nullptr默认值循环对应字段未误填nullptr
- [ ] David版本toCommandFunc复用v100对应函数值
- [ ] David版本toSqeFunc填nullptr

## 编译适配
- [ ] v200.cmake已删除David对xx_v100.cc依赖
- [ ] cmodel.cmake已检查并按需删除mechanism dependance中的xx_v100.cc依赖
- [ ] runtime.cmake和tiny.cmake已检查；tiny差异已记录原因
- [ ] UT CMakeLists.txt已删除相应依赖
- [ ] 910B平台UT保留xx_v100.cc（v100测试）

## 头文件与符号清理
- [ ] 新增注册文件显式include task_manager.h
- [ ] 调用RegDavidSqeFunc的文件显式include stars_david.hpp或对应声明头
- [ ] 删除头文件声明后，仅定义文件内使用的函数已改为static
- [ ] 被UT、跨版本文件或非注册逻辑直接调用的函数声明已保留

---

**当前任务**：____________
**检查结果**：
- 通过项数：____/____
- 未通过项：____（如有）
- 是否可继续下一任务：是/否

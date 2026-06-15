# 交叉引用规范

## API 文档引用

格式：
```markdown
参见：[aclrtMalloc API 参考](../03_api_ref/aclrtMalloc.md)
```

位置：FAQ 底部"参见"章节

注意：引用路径必须指向实际存在的文档文件。如果对应 API 参考文档尚不存在，不要编造路径。

## 最佳实践引用

格式：
```markdown
参见：[内存管理最佳实践](../02_dev_guide/memory_management.md)
```

注意：确认 `docs/02_dev_guide/` 下对应文件存在后再引用。

## 相关 FAQ 引用

格式：
```markdown
参见：[aclrtMalloc 内存申请失败](aclrtMalloc 内存申请失败常见原因.md)
```

注意：同目录下的 FAQ 用相对文件名引用，不需要 `../04_FAQ/` 前缀。

## 完整交叉引用示例

```markdown
## 参见

- [aclrtReserveMemAddress API 参考](../03_api_ref/aclrtReserveMemAddress.md)
- [内存管理最佳实践](../02_dev_guide/memory_management.md)
- [aclrtMalloc 内存申请失败](aclrtMalloc 内存申请失败常见原因.md)
```

## 引用路径校验

添加引用前，确认目标文件存在：
```bash
ls docs/03_api_ref/aclrtMalloc.md
ls docs/02_dev_guide/memory_management.md
ls docs/04_FAQ/aclrtMalloc 内存申请失败常见原因.md
```

如果文件不存在，不要添加引用。

## 引用网络价值

交叉引用构建知识网络：
- 用户可快速导航到相关文档
- 降低重复内容编写
- 提升文档系统性

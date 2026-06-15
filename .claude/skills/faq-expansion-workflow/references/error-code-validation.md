# 错误码校验参考

## 两套错误码定义

| 级别 | 编号范围 | 头文件位置 | 示例 |
|------|---------|-----------|------|
| ACL 级别 | 100xxx/200xxx/300xxx | `include/external/acl/acl_base_rt.h` | ACL_ERROR_INVALID_PARAM(100000), ACL_ERROR_REPEAT_INITIALIZE(100002) |
| Runtime 级别 | 107xxx/207xxx/507xxx | `include/external/acl/error_codes/rt_error_codes.h` | ACL_ERROR_RT_PARAM_INVALID(107000), ACL_ERROR_RT_MEMORY_ALLOCATION(207001) |

## 校验命令

```bash
# 校验 Runtime 级别错误码
grep "107000\|207001\|507015" include/external/acl/error_codes/rt_error_codes.h

# 校验 ACL 级别错误码
grep "100000\|100002" include/external/acl/acl_base_rt.h

# 校验 API 是否存在
grep "aclrtGetVersion\|aclrtSetDevice" include/external/acl/acl_rt.h
```

## 常见混淆

| 错误写法 | 正确写法 | 原因 |
|---------|---------|------|
| ACL_ERROR_INVALID_PARAM(107000) | ACL_ERROR_RT_PARAM_INVALID(107000) | 107000是 Runtime 码，ACL_ERROR_INVALID_PARAM 实际是100000 |
| 107002(ACL_ERROR_RT_MEMORY_ALLOC_FAILED) | 207001(ACL_ERROR_RT_MEMORY_ALLOCATION) | 编号和名称都错了 |
| ACL_ERROR_RT_FEATURE_NOT_SUPPORT(107003) | ACL_ERROR_RT_FEATURE_NOT_SUPPORT(207000) | 107003是 STREAM_CONTEXT |

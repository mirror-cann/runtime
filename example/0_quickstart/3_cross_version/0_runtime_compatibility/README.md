# 0_runtime_compatibility

## 描述

本样例演示跨版本兼容场景中的基础探测方法，包括查询 CANN 软件版本、查询当前环境支持的 CANN 特性、检查当前 SoC 架构兼容性，以及查询 Device 能力。样例用于帮助用户在不同 CANN 版本或不同产品环境中编写可分支处理的 Runtime 程序。

## 产品支持情况

本样例支持以下产品：

| 产品 | 是否支持 |
| --- | :---: |
| Ascend 950PR/Ascend 950DT | √ |
| Atlas A3 训练系列产品/Atlas A3 推理系列产品 | √ |
| Atlas A2 训练系列产品/Atlas A2 推理系列产品 | √ |

## 编译运行

1. 下载样例代码至安装 CANN 软件的环境，切换到样例目录。

```bash
cd ${git_clone_path}/example/0_quickstart/3_cross_version/0_runtime_compatibility
```

2. 设置环境变量。

```bash
# ${install_root} 替换为 CANN 安装根目录，默认安装在 `/usr/local/Ascend` 目录
source ${install_root}/cann/set_env.sh
```

3. 执行以下命令运行样例。

```bash
bash run.sh
```

## CANN RUNTIME API

在该 Sample 中，涉及的关键功能点及其关键接口如下所示：

- 初始化与 Device 管理
    - 调用 `aclInit` 接口完成 ACL 初始化。
    - 调用 `aclrtSetDevice` 接口指定用于查询的 Device。
    - 调用 `aclrtResetDeviceForce` 接口复位当前 Device。
    - 调用 `aclFinalize` 接口完成 ACL 去初始化。
- 版本与能力查询
    - 调用 `aclsysGetVersionStr` 和 `aclsysGetVersionNum` 接口查询 CANN 软件包版本信息。
    - 调用 `aclGetCannAttributeList` 接口查询当前环境支持的 CANN 特性列表。
    - 调用 `aclGetCannAttribute` 接口查询指定 CANN 特性是否支持。
    - 调用 `aclrtGetSocName` 接口查询当前 SoC 名称。
    - 调用 `aclrtCheckArchCompatibility` 接口检查当前 SoC 架构兼容性。
    - 调用 `aclrtGetDeviceCapability` 接口查询指定 Device 能力。

## 示例输出

```text
[INFO]  CANN package [runtime] version string: 9.1.0
[INFO]  CANN package [runtime] version number: 90100000
[INFO]  CANN attribute count: 3
[INFO]  CANN attribute ACL_CANN_ATTR_INF_NAN support value: 1
[INFO]  CANN attribute ACL_CANN_ATTR_BF16 support value: 1
[INFO]  CANN attribute ACL_CANN_ATTR_JIT_COMPILE support value: 1
[INFO]  Architecture compatibility for Ascend910B3: 1
[INFO]  Device capability ACL_FEATURE_TSCPU_TASK_UPDATE_SUPPORT_AIC_AIV: 1
[INFO]  [SUCCESS] Runtime compatibility sample completed successfully
[SUCCESS] Runtime compatibility sample executed successfully.
```

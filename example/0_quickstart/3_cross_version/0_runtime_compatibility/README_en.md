# 0_runtime_compatibility

## Description

This sample demonstrates basic detection methods for cross-version compatibility scenarios, including querying the CANN software version, querying CANN attributes supported by the current environment, checking SoC architecture compatibility, and querying Device capabilities. It helps users write Runtime programs with conditional compatibility handling across CANN versions and product environments.

## Product Support

This sample supports the following products:

| Product | Supported |
| --- | :---: |
| Ascend 950PR/Ascend 950DT | Yes |
| Atlas A3 training series products/Atlas A3 inference series products | Yes |
| Atlas A2 training series products/Atlas A2 inference series products | Yes |

## Compile and Run

1. Download the sample code to the environment where CANN is installed. Switch to the sample directory.

```bash
cd ${git_clone_path}/example/0_quickstart/3_cross_version/0_runtime_compatibility
```

2. Set environment variables.

```bash
# Replace ${install_root} with the CANN installation root directory. The default installation is in `/usr/local/Ascend`.
source ${install_root}/cann/set_env.sh
```

3. Run the following command to execute the sample.

```bash
bash run.sh
```

## CANN RUNTIME API

The key functionality points and their key interfaces involved in this sample are as follows:

- Initialization and Device Management
    - Call `aclInit` for ACL initialization.
    - Call `aclrtSetDevice` to specify the Device for queries.
    - Call `aclrtResetDeviceForce` to reset the current Device.
    - Call `aclFinalize` for ACL finalization.
- Version and Capability Query
    - Call `aclsysGetVersionStr` and `aclsysGetVersionNum` to query CANN package version information.
    - Call `aclGetCannAttributeList` to query the CANN attributes supported by the current environment.
    - Call `aclGetCannAttribute` to query whether a specified CANN attribute is supported.
    - Call `aclrtGetSocName` to query the current SoC name.
    - Call `aclrtCheckArchCompatibility` to check SoC architecture compatibility.
    - Call `aclrtGetDeviceCapability` to query a specified Device capability.

## Sample Output

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

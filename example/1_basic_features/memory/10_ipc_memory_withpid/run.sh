#!/bin/bash
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2025 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

set -euo pipefail

_ASCEND_CANN_PATH="${ASCEND_HOME_PATH:-}"
if [[ -z "${_ASCEND_CANN_PATH}" ]]; then
    echo "[ERROR]: ASCEND_HOME_PATH is not set."
    echo "[ERROR]: Please source CANN set_env.sh before running this sample."
    exit 1
fi

source "${_ASCEND_CANN_PATH}/bin/setenv.bash"
echo "[INFO]: Current compile soc version is ${SOC_VERSION}"

rm -rf build
mkdir -p build
cmake -B build \
    -DASCEND_CANN_PACKAGE_PATH="${_ASCEND_CANN_PATH}"
cmake --build build -j
cmake --install build

rm -rf file
mkdir -p file

file_path_proc_a="output_msg_proc_a.txt"
file_path_proc_b="output_msg_proc_b.txt"

(
    ./build/proc_a | tee "${file_path_proc_a}"
) &
pid_a=$!
(
    ./build/proc_b | tee "${file_path_proc_b}"
) &
pid_b=$!

status_a=0
status_b=0
wait "${pid_a}" || status_a=$?
wait "${pid_b}" || status_b=$?

if [[ "${status_a}" -ne 0 || "${status_b}" -ne 0 ]]; then
    echo "[FAILURE] IPC memory sharing failed. proc_a exit code is ${status_a}, proc_b exit code is ${status_b}"
    exit 1
fi

source_value=$(awk -F':' '/Source data:/ {gsub(/^ +| +$/, "", $2); print $2; exit}' "${file_path_proc_a}")
destination_value=$(awk -F':' '/Destination data:/ {gsub(/^ +| +$/, "", $2); print $2; exit}' "${file_path_proc_b}")

if [[ -n "${source_value}" && "${source_value}" = "${destination_value}" ]]; then
    echo "[SUCCESS] IPC memory sharing successfully. Values at source and destination are equal: ${source_value}"
else
    echo "[FAILURE] IPC memory sharing failed. Value at source is ${source_value}, but value at destination is ${destination_value}"
    exit 1
fi

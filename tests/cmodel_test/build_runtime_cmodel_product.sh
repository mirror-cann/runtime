#!/bin/bash
# -----------------------------------------------------------------------------------------------------------
# Copyright (c) 2026 Huawei Technologies Co., Ltd.
# This program is free software, you can redistribute it and/or modify it under the terms and conditions of
# CANN Open Software License Agreement Version 2.0 (the "License").
# Please refer to the License for details. You may not use this file except in compliance with the License.
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
# See LICENSE in the root of the software repository for the full text of the License.
# -----------------------------------------------------------------------------------------------------------

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "$0")"; pwd)
ROOT_DIR=$(cd "${SCRIPT_DIR}/../.."; pwd)
BUILD_DIR="${ROOT_DIR}/build_runtime_cmodel_product"
OUTPUT_DIR="${ROOT_DIR}/build_out"
THREAD_NUM=$(grep -c ^processor /proc/cpuinfo)
ASCEND_3RD_LIB_PATH="${ROOT_DIR}/output/third_party"
ASCEND_INSTALL_PATH="${ASCEND_INSTALL_PATH:-/usr/local/Ascend/cann}"
PRODUCT_TYPE=""

usage() {
  echo "Usage:"
  echo "  bash tests/cmodel_test/build_runtime_cmodel_product.sh --product_type=<PRODUCT_TYPE>"
}

parsed_args=$(getopt -a -o h -l help,product_type: -- "$@") || {
  usage
  exit 1
}
eval set -- "$parsed_args"

while true; do
  case "$1" in
    -h|--help)
      usage
      exit 0
      ;;
    --product_type)
      PRODUCT_TYPE="$2"
      shift 2
      ;;
    --)
      shift
      break
      ;;
    *)
      echo "Undefined option: $1"
      usage
      exit 1
      ;;
  esac
done

if [ -z "${PRODUCT_TYPE}" ]; then
  echo "--product_type is required"
  usage
  exit 1
fi

if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
  cached_home_dir=$(sed -n 's/^CMAKE_HOME_DIRECTORY:INTERNAL=//p' "${BUILD_DIR}/CMakeCache.txt")
  if [ -n "${cached_home_dir}" ] && [ "${cached_home_dir}" != "${SCRIPT_DIR}" ]; then
    rm -rf "${BUILD_DIR}"
  fi
fi

mkdir -p "${BUILD_DIR}" "${OUTPUT_DIR}"
cd "${BUILD_DIR}"

CMAKE_ARGS="-DENABLE_OPEN_SRC=True \
            -DASCEND_INSTALL_PATH=${ASCEND_INSTALL_PATH} \
            -DCANN_3RD_LIB_PATH=${ASCEND_3RD_LIB_PATH} \
            -DBUILD_RUNTIME_CMODEL_PRODUCT=${PRODUCT_TYPE}"

echo "CMAKE_ARGS=${CMAKE_ARGS}"
cmake -S "${SCRIPT_DIR}" -B . ${CMAKE_ARGS}
cmake --build . -j"${THREAD_NUM}" --target runtime_cmodel_${PRODUCT_TYPE} runtime_camodel_${PRODUCT_TYPE}

echo "build success!"

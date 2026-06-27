#!/bin/bash

cd "${WORKSPACE}"
echo $(grep -E "^VERSION_ID=" /etc/os-release | cut -d'"' -f2)
if [[ "${task_name}" == *ubuntu24* ]]; then
    sudo update-alternatives --set gcc /usr/bin/gcc-14
    package_name="cann-runtime_linux-x86_64_ubuntu24.run"
else
    if [[ -f "/opt/rh/devtoolset-7/enable" ]]; then
        echo "source devtoolset"
        source /opt/rh/devtoolset-7/enable
    fi
    package_name="cann-runtime_linux-x86_64.run"
fi
echo "package_name=${package_name}" >> "$ATOMGIT_OUTPUT"
gcc --version
source /home/jenkins/Ascend/cann/bin/setenv.bash
set +e
bash build.sh --cann_3rd_lib_path="/home/jenkins/opensource" -f "pr_filelist.txt"
ret=$?

# 200 表示跳过编译，属于正常情况
if [ "$ret" -eq 200 ]; then
    echo "not need compile"
    mkdir build_out
    touch build_out/${package_name}
    exit 0
fi
if [ "$ret" -eq 0 ]; then
    compile_package_name=$(ls "${WORKSPACE}/build_out"/*.run 2>/dev/null | head -n1)
    mv "${compile_package_name}" "${WORKSPACE}/build_out/${package_name}"
fi
echo "ret=$ret" >> "$ATOMGIT_OUTPUT"
exit "$ret"
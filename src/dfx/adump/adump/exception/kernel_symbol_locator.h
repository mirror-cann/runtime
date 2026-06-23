/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#ifndef KERNEL_SYMBOL_LOCATOR_H
#define KERNEL_SYMBOL_LOCATOR_H

#include <stdint.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "runtime/rt.h"
#include "exception_info_common.h"
#include "kernel_pc_fixer.h"
#include "adump_pub.h"

namespace Adx {

struct KernelSymbol {
    uint64_t offset;
    uint64_t size;
    uint64_t sectionEnd;
    uint16_t sectionIndex;
    uint8_t bind;
    uint8_t visibility;
    std::string name;
};

struct KernelSymbolSet {
    std::vector<KernelSymbol> symbols;
};

class KernelSymbolLocator {
public:
    KernelSymbolLocator();
    ~KernelSymbolLocator();

    int32_t InitFromBinHandle(rtBinHandle binHandle);
    int32_t InitFromBinBuffer(const std::string& binData);
    void UpdateStartPCFromDeviceAddr(rtBinHandle binHandle);

    int32_t LocateAndPrintErrorSymbols(const ExceptionRegInfo& regInfo);
    int32_t LocateAndPrintErrorSymbolsForCore(uint32_t coreId, uint32_t coreType, ExceptionRegInfo exceptionRegInfo);

    static uint64_t FixPcByErrorRegs(const rtExceptionErrRegInfo& coreInfo);
    static std::string GetErrorRegisters(const rtExceptionErrRegInfo& coreInfo);
    static void DumpErrorSymbols(const rtExceptionInfo& exception);
    static void DumpErrorSymbols(const rtExceptionInfo& exception, ExceptionRegInfo& exceptionRegInfo);

    static void ClearCache();

private:
    KernelSymbolSet kernelSymbols_;
    uint64_t kernelDeviceStartPC_ = 0;
    bool hasKernelDeviceStartPC_ = false;
    bool initialized_ = false;

    int32_t ParseElfSymbols(const char* elf, size_t elfSize, KernelSymbolSet& symbols);
    void PrintErrorForCore(rtExceptionErrRegInfo_t coreInfo);
    void ResetState();
    bool GetCorrectedStartPC(const rtExceptionErrRegInfo& coreInfo, uint64_t& startPC) const;

    static std::unordered_map<rtBinHandle, KernelSymbolSet> cache_;
};

} // namespace Adx

#endif

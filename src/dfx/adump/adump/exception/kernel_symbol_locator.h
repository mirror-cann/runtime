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
    uint64_t aicBase = 0;
    bool hasAicBase = false;
    uint64_t aivBase = 0;
    bool hasAivBase = false;
};

struct KernelStartPC {
    uint64_t aicStartPC = 0;
    bool hasAicStartPC = false;
    uint64_t aivStartPC = 0;
    bool hasAivStartPC = false;
};

class KernelSymbolLocator {
public:
    KernelSymbolLocator();
    ~KernelSymbolLocator();

    int32_t InitFromBinHandle(rtBinHandle binHandle);
    int32_t InitFromBinBuffer(const std::string& binData);
    void UpdateStartPCFromFuncAddr(rtBinHandle binHandle, const char* kernelName);
    void SetApplyEntryBase(bool applyEntryBase) { applyEntryBase_ = applyEntryBase; }

    int32_t LocateAndPrintErrorSymbols(const ExceptionRegInfo& regInfo);
    int32_t LocateAndPrintErrorSymbolsForCore(uint32_t coreId, uint32_t coreType, ExceptionRegInfo exceptionRegInfo);

    static uint64_t FixPcByErrorRegs(const rtExceptionErrRegInfo& coreInfo);
    static std::string GetErrorDescription(const rtExceptionErrRegInfo& coreInfo);
    static std::string GetErrorRegisters(const rtExceptionErrRegInfo& coreInfo);
    static void DumpErrorSymbols(const rtExceptionInfo& exception);
    static void DumpErrorSymbols(const rtExceptionInfo& exception, ExceptionRegInfo& exceptionRegInfo);

    static void ClearCache();

private:
    KernelSymbolSet kernelSymbols_;
    KernelStartPC kernelStartPC_;
    bool initialized_ = false;
    // 仅 GE+SK 场景（te_superkernel 入口）查找偏移时需要叠加 mix 入口基址。
    bool applyEntryBase_ = false;

    int32_t ParseElfSymbols(const char* elf, size_t elfSize, KernelSymbolSet& symbols);
    void PrintErrorForCore(rtExceptionErrRegInfo_t coreInfo);
    void ResetState();
    bool GetCorrectedStartPC(const rtExceptionErrRegInfo& coreInfo, uint64_t& startPC) const;
    bool GetLookupOffset(const rtExceptionErrRegInfo& coreInfo, uint64_t rawOffset, uint64_t& lookupOffset) const;

    static std::unordered_map<rtBinHandle, KernelSymbolSet> cache_;
};

} // namespace Adx

#endif

/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <algorithm>
#include <cstring>
#include <elf.h>
#include <limits>
#include <mutex>
#include "securec.h"
#include "runtime/kernel.h"
#include "kernel_symbol_locator.h"
#include "exception_info_common.h"
#include "str_utils.h"
#include "log/adx_log.h"
#include "log/hdc_log.h"

namespace Adx {
namespace {
std::mutex g_cacheMutex;

// 符号过滤计数关系：
// total = accepted + nonFunc + invalidSection + invalidName。
struct SymbolFilterStats {
    // 从有效的 SHT_SYMTAB/SHT_DYNSYM 段读取到的符号总数。
    size_t total = 0;
    // 被接受用于函数定位的有效函数符号数。
    size_t accepted = 0;
    // 因 st_info 类型不是 STT_FUNC 被过滤的符号数。
    size_t nonFunc = 0;
    // 因 st_shndx 为 SHN_UNDEF 或超出 section header 范围被过滤的函数符号数。
    size_t invalidSection = 0;
    // 因 st_name 越界或符号名未在字符串表范围内以 '\\0' 结束被过滤的函数符号数。
    size_t invalidName = 0;
};

template <typename T>
bool ReadStruct(const char* elf, size_t elfSize, size_t offset, T& out)
{
    if (elf == nullptr || offset > elfSize || elfSize - offset < sizeof(T)) {
        return false;
    }
    return memcpy_s(&out, sizeof(T), elf + offset, sizeof(T)) == EOK;
}

bool IsAddOverflow(size_t lhs, size_t rhs) { return lhs > std::numeric_limits<size_t>::max() - rhs; }

bool IsAddOverflow64(uint64_t lhs, uint64_t rhs)
{
    return lhs > std::numeric_limits<uint64_t>::max() - rhs;
}

void UpdateMixEntryBase(const KernelSymbol& symbol, KernelMixType kernelMixType, uint64_t& base, bool& hasBase)
{
    // mix 入口基址只从全局可见入口符号中选择，本地符号仍保留用于地址查找。
    if (symbol.bind != STB_GLOBAL || symbol.visibility == STV_HIDDEN) {
        return;
    }
    if (ExceptionInfoCommon::GetKernelMixType(symbol.name) != kernelMixType) {
        return;
    }
    if (!hasBase || symbol.offset < base) {
        base = symbol.offset;
        hasBase = true;
    }
}

void BuildMixEntryBaseIndex(const std::vector<KernelSymbol>& parsedSymbols, KernelSymbolSet& symbols)
{
    symbols.aicBase = 0;
    symbols.hasAicBase = false;
    symbols.aivBase = 0;
    symbols.hasAivBase = false;
    for (const auto& symbol : parsedSymbols) {
        UpdateMixEntryBase(symbol, KernelMixType::AIC, symbols.aicBase, symbols.hasAicBase);
        UpdateMixEntryBase(symbol, KernelMixType::AIV, symbols.aivBase, symbols.hasAivBase);
    }
}

bool GetSymbolOffsetRange(const std::vector<KernelSymbol>& symbols, uint64_t& minOffset, uint64_t& maxEnd)
{
    bool hasRange = false;
    minOffset = 0;
    maxEnd = 0;
    for (const KernelSymbol& symbol : symbols) {
        if (IsAddOverflow64(symbol.offset, symbol.size)) {
            continue;
        }
        const uint64_t symbolEnd = symbol.offset + symbol.size;
        if (!hasRange) {
            minOffset = symbol.offset;
            maxEnd = symbolEnd;
            hasRange = true;
            continue;
        }
        minOffset = std::min(minOffset, symbol.offset);
        maxEnd = std::max(maxEnd, symbolEnd);
    }
    return hasRange;
}

const KernelSymbol* FindBestMatchedSymbol(const std::vector<KernelSymbol>& symbols, uint64_t fixedPCOffset)
{
    const KernelSymbol* matchedSymbol = nullptr;
    for (const auto& symbol : symbols) {
        if (fixedPCOffset < symbol.offset || fixedPCOffset - symbol.offset >= symbol.size) {
            continue;
        }
        if (matchedSymbol == nullptr || symbol.offset > matchedSymbol->offset ||
            (symbol.offset == matchedSymbol->offset && symbol.size < matchedSymbol->size)) {
            matchedSymbol = &symbol;
        }
    }
    return matchedSymbol;
}

void LogKernelSymbolSummary(
    const KernelSymbolSet& symbols, size_t parsedSymbolCount, const SymbolFilterStats& filterStats)
{
    uint64_t minOffset = 0;
    uint64_t maxEnd = 0;
    const bool hasRange = GetSymbolOffsetRange(symbols.symbols, minOffset, maxEnd);
    IDE_LOGI("Parse kernel symbols success. parsedSymbolCount=%zu, normalizedSymbolCount=%zu, "
        "hasSymbolRange=%u, minSymbolOffset=0x%lx, maxSymbolEnd=0x%lx, hasAicBase=%u, aicBase=0x%lx, "
        "hasAivBase=%u, aivBase=0x%lx, symbolTotal=%zu, accepted=%zu, nonFunc=%zu, invalidSection=%zu, "
        "invalidName=%zu.",
        parsedSymbolCount, symbols.symbols.size(), static_cast<uint32_t>(hasRange), minOffset, maxEnd,
        static_cast<uint32_t>(symbols.hasAicBase), symbols.aicBase, static_cast<uint32_t>(symbols.hasAivBase),
        symbols.aivBase, filterStats.total, filterStats.accepted, filterStats.nonFunc, filterStats.invalidSection,
        filterStats.invalidName);
}

uint16_t Swap16(uint16_t value) { return static_cast<uint16_t>((value >> 8U) | (value << 8U)); }

uint32_t Swap32(uint32_t value)
{
    return ((value & 0x000000FFU) << 24U) | ((value & 0x0000FF00U) << 8U) | ((value & 0x00FF0000U) >> 8U) |
           ((value & 0xFF000000U) >> 24U);
}

uint64_t Swap64(uint64_t value)
{
    return ((value & 0x00000000000000FFULL) << 56U) | ((value & 0x000000000000FF00ULL) << 40U) |
           ((value & 0x0000000000FF0000ULL) << 24U) | ((value & 0x00000000FF000000ULL) << 8U) |
           ((value & 0x000000FF00000000ULL) >> 8U) | ((value & 0x0000FF0000000000ULL) >> 24U) |
           ((value & 0x00FF000000000000ULL) >> 40U) | ((value & 0xFF00000000000000ULL) >> 56U);
}

bool IsSupportedElfData(uint8_t data) { return data == ELFDATANONE || data == ELFDATA2LSB || data == ELFDATA2MSB; }

bool IsBigEndianElf(const Elf64_Ehdr& ehdr) { return ehdr.e_ident[EI_DATA] == ELFDATA2MSB; }

bool IsHostBigEndian()
{
    const uint16_t value = 0x0102U;
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    return bytes[0] == 0x01U;
}

bool ShouldSwapElfBytes(const Elf64_Ehdr& ehdr) { return IsBigEndianElf(ehdr) != IsHostBigEndian(); }

void NormalizeElfHeader(Elf64_Ehdr& ehdr, bool shouldSwap)
{
    if (!shouldSwap) {
        return;
    }
    ehdr.e_type = Swap16(ehdr.e_type);
    ehdr.e_machine = Swap16(ehdr.e_machine);
    ehdr.e_version = Swap32(ehdr.e_version);
    ehdr.e_entry = Swap64(ehdr.e_entry);
    ehdr.e_phoff = Swap64(ehdr.e_phoff);
    ehdr.e_shoff = Swap64(ehdr.e_shoff);
    ehdr.e_flags = Swap32(ehdr.e_flags);
    ehdr.e_ehsize = Swap16(ehdr.e_ehsize);
    ehdr.e_phentsize = Swap16(ehdr.e_phentsize);
    ehdr.e_phnum = Swap16(ehdr.e_phnum);
    ehdr.e_shentsize = Swap16(ehdr.e_shentsize);
    ehdr.e_shnum = Swap16(ehdr.e_shnum);
    ehdr.e_shstrndx = Swap16(ehdr.e_shstrndx);
}

void NormalizeSectionHeader(Elf64_Shdr& shdr, bool shouldSwap)
{
    if (!shouldSwap) {
        return;
    }
    shdr.sh_name = Swap32(shdr.sh_name);
    shdr.sh_type = Swap32(shdr.sh_type);
    shdr.sh_flags = Swap64(shdr.sh_flags);
    shdr.sh_addr = Swap64(shdr.sh_addr);
    shdr.sh_offset = Swap64(shdr.sh_offset);
    shdr.sh_size = Swap64(shdr.sh_size);
    shdr.sh_link = Swap32(shdr.sh_link);
    shdr.sh_info = Swap32(shdr.sh_info);
    shdr.sh_addralign = Swap64(shdr.sh_addralign);
    shdr.sh_entsize = Swap64(shdr.sh_entsize);
}

void NormalizeSymbol(Elf64_Sym& sym, bool shouldSwap)
{
    if (!shouldSwap) {
        return;
    }
    sym.st_name = Swap32(sym.st_name);
    sym.st_shndx = Swap16(sym.st_shndx);
    sym.st_value = Swap64(sym.st_value);
    sym.st_size = Swap64(sym.st_size);
}

bool IsValidElfHeader(const Elf64_Ehdr& ehdr)
{
    return std::memcmp(ehdr.e_ident, ELFMAG, SELFMAG) == 0 && ehdr.e_ident[EI_CLASS] == ELFCLASS64 &&
           IsSupportedElfData(ehdr.e_ident[EI_DATA]) && ehdr.e_ehsize == sizeof(Elf64_Ehdr) &&
           ehdr.e_shentsize == sizeof(Elf64_Shdr) && ehdr.e_shoff != 0 && ehdr.e_shnum != 0;
}

bool IsRangeInsideElf(size_t offset, size_t size, size_t elfSize)
{
    return offset <= elfSize && elfSize - offset >= size;
}

bool IsSectionInsideElf(const Elf64_Shdr& section, size_t elfSize)
{
    if (section.sh_offset > static_cast<uint64_t>(std::numeric_limits<size_t>::max()) ||
        section.sh_size > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        return false;
    }
    return IsRangeInsideElf(static_cast<size_t>(section.sh_offset), static_cast<size_t>(section.sh_size), elfSize);
}

bool ReadElfHeader(const char* elf, size_t elfSize, Elf64_Ehdr& ehdr)
{
    if (!ReadStruct(elf, elfSize, 0, ehdr)) {
        return false;
    }
    if (std::memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0 || ehdr.e_ident[EI_CLASS] != ELFCLASS64 ||
        !IsSupportedElfData(ehdr.e_ident[EI_DATA])) {
        return false;
    }
    // 与 runtime 保持一致：ELFDATA2MSB 按大端解析字段；ELFDATANONE 按小端处理。
    NormalizeElfHeader(ehdr, ShouldSwapElfBytes(ehdr));
    return IsValidElfHeader(ehdr);
}

bool ReadSectionHeaders(
    const char* elf, size_t elfSize, const Elf64_Ehdr& ehdr, bool shouldSwap, std::vector<Elf64_Shdr>& outShdrs)
{
    const size_t shdrsSize = static_cast<size_t>(ehdr.e_shnum) * sizeof(Elf64_Shdr);
    if (ehdr.e_shoff > static_cast<uint64_t>(std::numeric_limits<size_t>::max()) ||
        !IsRangeInsideElf(static_cast<size_t>(ehdr.e_shoff), shdrsSize, elfSize)) {
        return false;
    }
    if (ehdr.e_shstrndx >= ehdr.e_shnum && ehdr.e_shstrndx != SHN_UNDEF) {
        return false;
    }

    outShdrs.clear();
    outShdrs.reserve(ehdr.e_shnum);
    for (uint16_t i = 0; i < ehdr.e_shnum; i++) {
        Elf64_Shdr shdr = {};
        const size_t offset = static_cast<size_t>(ehdr.e_shoff) + static_cast<size_t>(i) * sizeof(Elf64_Shdr);
        if (!ReadStruct(elf, elfSize, offset, shdr)) {
            return false;
        }
        NormalizeSectionHeader(shdr, shouldSwap);
        outShdrs.push_back(shdr);
    }
    return true;
}

bool IsSymbolTable(const Elf64_Shdr& section) { return section.sh_type == SHT_SYMTAB || section.sh_type == SHT_DYNSYM; }

bool IsValidSymbolAndStringTable(
    size_t elfSize, const std::vector<Elf64_Shdr>& shdrs, const Elf64_Shdr& symtabShdr, const Elf64_Shdr*& strtabShdr)
{
    if (!IsSectionInsideElf(symtabShdr, elfSize) || symtabShdr.sh_size == 0) {
        return false;
    }
    if (symtabShdr.sh_entsize != sizeof(Elf64_Sym) || (symtabShdr.sh_size % sizeof(Elf64_Sym)) != 0) {
        return false;
    }
    if (symtabShdr.sh_link >= shdrs.size()) {
        return false;
    }

    strtabShdr = &shdrs[symtabShdr.sh_link];
    if (strtabShdr->sh_type != SHT_STRTAB || strtabShdr->sh_size == 0) {
        return false;
    }
    return IsSectionInsideElf(*strtabShdr, elfSize);
}

bool GetSymbolSectionEnd(const Elf64_Sym& sym, const std::vector<Elf64_Shdr>& shdrs, uint64_t& sectionEnd)
{
    if (sym.st_shndx >= shdrs.size()) {
        return false;
    }
    const Elf64_Shdr& section = shdrs[sym.st_shndx];
    // ET_REL 中 st_value 通常是 section 内偏移且 sh_addr 为 0；加载态镜像中 st_value 通常可与 sh_addr 比较。
    uint64_t sectionBase = section.sh_addr;
    if (sym.st_value < sectionBase) {
        sectionBase = 0;
    }
    if (sectionBase > std::numeric_limits<uint64_t>::max() - section.sh_size) {
        return false;
    }
    sectionEnd = sectionBase + section.sh_size;
    return sectionEnd > sym.st_value;
}

bool BuildKernelSymbol(
    const Elf64_Sym& sym, const std::vector<Elf64_Shdr>& shdrs, const char* strtab, size_t strtabSize,
    SymbolFilterStats& stats, KernelSymbol& outSymbol)
{
    stats.total++;
    if (ELF64_ST_TYPE(sym.st_info) != STT_FUNC) {
        stats.nonFunc++;
        return false;
    }
    if (sym.st_shndx == SHN_UNDEF) {
        stats.invalidSection++;
        return false;
    }
    if (sym.st_shndx >= shdrs.size()) {
        stats.invalidSection++;
        return false;
    }
    if (sym.st_name >= strtabSize) {
        stats.invalidName++;
        return false;
    }
    // section 结束地址用于给 st_size 为 0 的符号补齐范围。
    (void)GetSymbolSectionEnd(sym, shdrs, outSymbol.sectionEnd);

    const char* strStart = strtab + sym.st_name;
    size_t remaining = strtabSize - sym.st_name;
    const char* strEnd = static_cast<const char*>(std::memchr(strStart, '\0', remaining));
    if (strEnd == nullptr) {
        stats.invalidName++;
        return false;
    }

    outSymbol.offset = sym.st_value;
    outSymbol.size = sym.st_size;
    outSymbol.sectionIndex = sym.st_shndx;
    outSymbol.bind = ELF64_ST_BIND(sym.st_info);
    outSymbol.visibility = ELF64_ST_VISIBILITY(sym.st_other);
    outSymbol.name.assign(strStart, strEnd - strStart);
    IDE_LOGD("Parse kernel symbol, name=%s, offset=0x%lx, size=0x%lx, sectionEnd=0x%lx, "
        "sectionIndex=%u, bind=%u, visibility=%u.",
        outSymbol.name.c_str(), outSymbol.offset, outSymbol.size, outSymbol.sectionEnd,
        static_cast<uint32_t>(outSymbol.sectionIndex), static_cast<uint32_t>(outSymbol.bind),
        static_cast<uint32_t>(outSymbol.visibility));
    stats.accepted++;
    return true;
}

bool ParseFunctionSymbols(
    const char* elf, size_t elfSize, const Elf64_Shdr& symtabShdr, const Elf64_Shdr& strtabShdr, bool shouldSwap,
    const std::vector<Elf64_Shdr>& shdrs, std::vector<KernelSymbol>& outSymbols, SymbolFilterStats& stats)
{
    const size_t symCount = symtabShdr.sh_size / sizeof(Elf64_Sym);
    const char* strtab = elf + strtabShdr.sh_offset;
    for (size_t i = 0; i < symCount; i++) {
        if (IsAddOverflow(static_cast<size_t>(symtabShdr.sh_offset), i * sizeof(Elf64_Sym))) {
            return false;
        }
        Elf64_Sym sym = {};
        const size_t symOffset = static_cast<size_t>(symtabShdr.sh_offset) + i * sizeof(Elf64_Sym);
        if (!ReadStruct(elf, elfSize, symOffset, sym)) {
            return false;
        }
        NormalizeSymbol(sym, shouldSwap);
        KernelSymbol symbol = {};
        if (BuildKernelSymbol(sym, shdrs, strtab, static_cast<size_t>(strtabShdr.sh_size), stats, symbol)) {
            outSymbols.push_back(symbol);
        }
    }
    return true;
}

bool IsSameKernelSymbol(const KernelSymbol& lhs, const KernelSymbol& rhs)
{
    return lhs.offset == rhs.offset && lhs.size == rhs.size && lhs.sectionIndex == rhs.sectionIndex &&
           lhs.name == rhs.name;
}

void SortKernelSymbols(std::vector<KernelSymbol>& symbols)
{
    // 按地址排序，便于后续用同 section 内的下一个符号修正 zero-size 符号范围。
    std::sort(symbols.begin(), symbols.end(), [](const KernelSymbol& lhs, const KernelSymbol& rhs) {
        if (lhs.offset != rhs.offset) {
            return lhs.offset < rhs.offset;
        }
        if (lhs.size != rhs.size) {
            return lhs.size > rhs.size;
        }
        return lhs.name < rhs.name;
    });
}

void DeduplicateKernelSymbols(std::vector<KernelSymbol>& symbols)
{
    std::vector<KernelSymbol> uniqueSymbols;
    uniqueSymbols.reserve(symbols.size());
    for (const KernelSymbol& symbol : symbols) {
        // 同一个函数可能同时出现在 .symtab 和 .dynsym 中。
        if (!uniqueSymbols.empty() && IsSameKernelSymbol(uniqueSymbols.back(), symbol)) {
            uniqueSymbols.back().sectionEnd = std::max(uniqueSymbols.back().sectionEnd, symbol.sectionEnd);
            continue;
        }
        uniqueSymbols.push_back(symbol);
    }
    symbols.swap(uniqueSymbols);
}

void FillZeroSizeSymbolRanges(std::vector<KernelSymbol>& symbols)
{
    for (size_t i = 0; i < symbols.size(); i++) {
        if (symbols[i].size != 0) {
            continue;
        }
        // 参考 LLDB 策略：先用 section 结束地址作为最大范围，再用同 section 的下一个符号地址收缩范围。
        if (symbols[i].sectionEnd > symbols[i].offset) {
            symbols[i].size = symbols[i].sectionEnd - symbols[i].offset;
        }
        for (size_t j = i + 1; j < symbols.size(); j++) {
            if (symbols[j].sectionIndex == symbols[i].sectionIndex && symbols[j].offset > symbols[i].offset) {
                const uint64_t sizeToNextSymbol = symbols[j].offset - symbols[i].offset;
                if (symbols[i].size == 0 || sizeToNextSymbol < symbols[i].size) {
                    symbols[i].size = sizeToNextSymbol;
                }
                break;
            }
        }
    }
}

void FilterValidKernelSymbols(const std::vector<KernelSymbol>& symbols, std::vector<KernelSymbol>& outSymbols)
{
    outSymbols.clear();
    outSymbols.reserve(symbols.size());
    for (const KernelSymbol& symbol : symbols) {
        if (!symbol.name.empty() && symbol.size != 0) {
            outSymbols.push_back(symbol);
        }
    }
}

void NormalizeFunctionSymbols(std::vector<KernelSymbol>& symbols, std::vector<KernelSymbol>& outSymbols)
{
    SortKernelSymbols(symbols);
    DeduplicateKernelSymbols(symbols);
    FillZeroSizeSymbolRanges(symbols);
    FilterValidKernelSymbols(symbols, outSymbols);
}

bool ParseSymbolTables(
    const char* elf, size_t elfSize, const std::vector<Elf64_Shdr>& shdrs, bool shouldSwap,
    std::vector<KernelSymbol>& parsedSymbols, SymbolFilterStats& filterStats, size_t& symbolTableCount,
    size_t& validSymbolTableCount)
{
    for (const Elf64_Shdr& symtabShdr : shdrs) {
        if (!IsSymbolTable(symtabShdr)) {
            continue;
        }
        symbolTableCount++;
        const Elf64_Shdr* strtabShdr = nullptr;
        if (!IsValidSymbolAndStringTable(elfSize, shdrs, symtabShdr, strtabShdr)) {
            continue;
        }
        validSymbolTableCount++;
        if (!ParseFunctionSymbols(elf, elfSize, symtabShdr, *strtabShdr, shouldSwap, shdrs, parsedSymbols,
            filterStats)) {
            IDE_LOGW("ParseElfSymbols failed, invalid ELF symbols, symOffset=%lu, symSize=%lu.",
                symtabShdr.sh_offset, symtabShdr.sh_size);
            return false;
        }
    }
    return true;
}
} // namespace

std::unordered_map<rtBinHandle, KernelSymbolSet> KernelSymbolLocator::cache_;

KernelSymbolLocator::KernelSymbolLocator() : initialized_(false) {}
KernelSymbolLocator::~KernelSymbolLocator() = default;

void KernelSymbolLocator::ClearCache()
{
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    cache_.clear();
}

void KernelSymbolLocator::ResetState()
{
    kernelSymbols_ = KernelSymbolSet();
    kernelStartPC_ = KernelStartPC();
    initialized_ = false;
}

void KernelSymbolLocator::UpdateStartPCFromFuncAddr(rtBinHandle binHandle, const char* kernelName)
{
    IDE_CTRL_VALUE_WARN(binHandle != nullptr && kernelName != nullptr && kernelName[0] != '\0', return,
        "Skip correcting startPC, binHandle=%p, kernelName=%p.", binHandle, kernelName);

    void* aicAddr = nullptr;
    void* aivAddr = nullptr;
    int32_t ret = ExceptionInfoCommon::GetKernelFuncAddr(binHandle, kernelName, aicAddr, aivAddr);
    IDE_CTRL_VALUE_WARN(ret == ADUMP_SUCCESS, return,
        "Get kernel func addr failed, skip correcting startPC, binHandle=%p, kernelName=%s.", binHandle, kernelName);

    if (aicAddr != nullptr) {
        kernelStartPC_.aicStartPC = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(aicAddr));
        kernelStartPC_.hasAicStartPC = true;
    }
    if (aivAddr != nullptr) {
        kernelStartPC_.aivStartPC = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(aivAddr));
        kernelStartPC_.hasAivStartPC = true;
    }
    IDE_LOGI("Update kernel startPC from function address, kernelName=%s, hasAicStartPC=%u, "
        "aicStartPC=0x%lx, hasAivStartPC=%u, aivStartPC=0x%lx.",
        kernelName, static_cast<uint32_t>(kernelStartPC_.hasAicStartPC), kernelStartPC_.aicStartPC,
        static_cast<uint32_t>(kernelStartPC_.hasAivStartPC), kernelStartPC_.aivStartPC);
}

int32_t KernelSymbolLocator::InitFromBinHandle(rtBinHandle binHandle)
{
    ResetState();
    IDE_CTRL_VALUE_WARN(binHandle != nullptr, return ADUMP_FAILED, "binHandle is null.");
    {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        auto it = cache_.find(binHandle);
        if (it != cache_.end()) {
            kernelSymbols_ = it->second;
            initialized_ = true;
            return ADUMP_SUCCESS;
        }
    }
    std::string binData;
    uint32_t binSize = 0;
    int32_t ret = ExceptionInfoCommon::GetBinDataFromHandle(binHandle, binData, binSize);
    IDE_CTRL_VALUE_WARN(ret == ADUMP_SUCCESS, return ADUMP_FAILED, "Get Kernel bin data failed for ParseElfSymbols");

    KernelSymbolSet symbols;
    ret = ParseElfSymbols(binData.data(), binData.size(), symbols);
    IDE_CTRL_VALUE_FAILED(ret == ADUMP_SUCCESS, return ADUMP_FAILED, "ParseElfSymbols failed.");
    kernelSymbols_ = symbols;
    {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        cache_[binHandle] = symbols;
    }
    initialized_ = true;
    return ADUMP_SUCCESS;
}

int32_t KernelSymbolLocator::InitFromBinBuffer(const std::string& binData)
{
    ResetState();
    IDE_CTRL_VALUE_WARN(!binData.empty(), return ADUMP_FAILED, "Kernel bin data is empty.");

    int32_t ret = ParseElfSymbols(binData.data(), binData.size(), kernelSymbols_);
    IDE_CTRL_VALUE_FAILED(ret == ADUMP_SUCCESS, return ADUMP_FAILED, "ParseElfSymbols failed.");

    initialized_ = true;
    return ADUMP_SUCCESS;
}

int32_t KernelSymbolLocator::ParseElfSymbols(const char* elf, size_t elfSize, KernelSymbolSet& outSymbols)
{
    Elf64_Ehdr ehdr = {};
    IDE_CTRL_VALUE_WARN(ReadElfHeader(elf, elfSize, ehdr), return ADUMP_FAILED,
        "ParseElfSymbols failed, invalid ELF header, elfSize=%zu.", elfSize);

    const bool shouldSwap = ShouldSwapElfBytes(ehdr);
    std::vector<Elf64_Shdr> shdrs;
    IDE_CTRL_VALUE_WARN(ReadSectionHeaders(elf, elfSize, ehdr, shouldSwap, shdrs), return ADUMP_FAILED,
        "ParseElfSymbols failed, invalid ELF section headers, shoff=%lu, shnum=%u.", ehdr.e_shoff, ehdr.e_shnum);

    std::vector<KernelSymbol> parsedSymbols;
    SymbolFilterStats filterStats;
    size_t symbolTableCount = 0;
    size_t validSymbolTableCount = 0;
    IDE_CTRL_VALUE_WARN(ParseSymbolTables(elf, elfSize, shdrs, shouldSwap, parsedSymbols, filterStats,
        symbolTableCount, validSymbolTableCount), return ADUMP_FAILED, "ParseElfSymbols failed.");

    IDE_CTRL_VALUE_WARN(symbolTableCount != 0, return ADUMP_FAILED,
        "ParseElfSymbols failed, no SHT_SYMTAB or SHT_DYNSYM section found.");
    IDE_CTRL_VALUE_WARN(validSymbolTableCount != 0, return ADUMP_FAILED,
        "ParseElfSymbols failed, no valid symbol table found, symbolTableCount=%zu.", symbolTableCount);

    std::vector<KernelSymbol> normalizedSymbols;
    NormalizeFunctionSymbols(parsedSymbols, normalizedSymbols);
    IDE_CTRL_VALUE_WARN(!normalizedSymbols.empty(), return ADUMP_FAILED,
        "ParseElfSymbols failed, empty function symbols, validSymbolTableCount=%zu, symbolTotal=%zu, "
        "accepted=%zu, nonFunc=%zu, invalidSection=%zu, invalidName=%zu.",
        validSymbolTableCount, filterStats.total, filterStats.accepted, filterStats.nonFunc,
        filterStats.invalidSection, filterStats.invalidName);

    BuildMixEntryBaseIndex(parsedSymbols, outSymbols);
    outSymbols.symbols.swap(normalizedSymbols);
    LogKernelSymbolSummary(outSymbols, parsedSymbols.size(), filterStats);
    return ADUMP_SUCCESS;
}

bool KernelSymbolLocator::GetLookupOffset(
    const rtExceptionErrRegInfo_t& coreInfo, uint64_t rawOffset, uint64_t& lookupOffset) const
{
    lookupOffset = rawOffset;
    // 仅 GE+SK 场景需要叠加 mix 入口基址，其余场景（含 aclgraph+SK）直接使用原始偏移。
    if (!applyEntryBase_) {
        return true;
    }
    uint64_t entryBase = 0;
    bool hasEntryBase = false;
    if (coreInfo.coreType == RT_CORE_TYPE_AIC) {
        entryBase = kernelSymbols_.aicBase;
        hasEntryBase = kernelSymbols_.hasAicBase;
    } else if (coreInfo.coreType == RT_CORE_TYPE_AIV) {
        entryBase = kernelSymbols_.aivBase;
        hasEntryBase = kernelSymbols_.hasAivBase;
    } else {
        return true;
    }

    if (!hasEntryBase) {
        if (kernelSymbols_.hasAicBase || kernelSymbols_.hasAivBase) {
            IDE_LOGW("Mix entry base is unavailable. coreId=%u, coreType=%u, "
                "hasAicBase=%u, aicBase=0x%lx, hasAivBase=%u, aivBase=0x%lx, use pcOffset=0x%lx.",
                coreInfo.coreId, static_cast<uint32_t>(coreInfo.coreType),
                static_cast<uint32_t>(kernelSymbols_.hasAicBase), kernelSymbols_.aicBase,
                static_cast<uint32_t>(kernelSymbols_.hasAivBase), kernelSymbols_.aivBase, rawOffset);
        }
        return true;
    }
    if (rawOffset > std::numeric_limits<uint64_t>::max() - entryBase) {
        IDE_LOGE("Offset is overflow. coreId=%u, coreType=%u, rawOffset=0x%lx, entryBase=0x%lx.",
            coreInfo.coreId, static_cast<uint32_t>(coreInfo.coreType), rawOffset, entryBase);
        return false;
    }
    lookupOffset = rawOffset + entryBase;
    return true;
}

bool KernelSymbolLocator::GetCorrectedStartPC(const rtExceptionErrRegInfo_t& coreInfo, uint64_t& startPC) const
{
    startPC = coreInfo.startPC;
    if (coreInfo.coreType == RT_CORE_TYPE_AIC && kernelStartPC_.hasAicStartPC) {
        startPC = kernelStartPC_.aicStartPC;
        return startPC != coreInfo.startPC;
    }
    if (coreInfo.coreType == RT_CORE_TYPE_AIV && kernelStartPC_.hasAivStartPC) {
        startPC = kernelStartPC_.aivStartPC;
        return startPC != coreInfo.startPC;
    }
    return false;
}

void KernelSymbolLocator::PrintErrorForCore(rtExceptionErrRegInfo_t coreInfo)
{
    uint32_t coreType = static_cast<uint32_t>(coreInfo.coreType);
    IDE_LOGE("[Dump][Exception] Error register information. coreId=%u, coreType=%u, %s",
        coreInfo.coreId, coreType, GetErrorRegisters(coreInfo).c_str());
    uint64_t fixedCurrentPC = FixPcByErrorRegs(coreInfo);
    uint64_t fixedStartPC = coreInfo.startPC;
    if (GetCorrectedStartPC(coreInfo, fixedStartPC)) {
        IDE_LOGI("Correct startPC by function address. coreId=%u, coreType=%u, originalStartPC=0x%lx, "
            "fixedStartPC=0x%lx.", coreInfo.coreId, coreType, coreInfo.startPC, fixedStartPC);
    }
    if (fixedCurrentPC < fixedStartPC) {
        IDE_LOGE("coreId=%u, coreType=%u, fixedCurrentPC=0x%lx < fixedStartPC=0x%lx, "
            "originalCurrentPC=0x%lx, originalStartPC=0x%lx, skip symbol lookup.",
            coreInfo.coreId, coreType, fixedCurrentPC, fixedStartPC, coreInfo.currentPC, coreInfo.startPC);
        return;
    }
    std::string errorDescription = GetErrorDescription(coreInfo);
    const uint64_t rawPCOffset = fixedCurrentPC - fixedStartPC;
    uint64_t fixedPCOffset = rawPCOffset;
    if (!GetLookupOffset(coreInfo, rawPCOffset, fixedPCOffset)) {
        return;
    }
    IDE_LOGE("[Dump][Exception] Error PC information. coreId=%u, coreType=%u, originalStartPC=0x%lx, "
        "fixedStartPC=0x%lx, originalCurrentPC=0x%lx, fixedCurrentPC=0x%lx, rawPCOffset=0x%lx, fixedPCOffset=0x%lx, "
        "errModule=%s.", coreInfo.coreId, coreType, coreInfo.startPC, fixedStartPC, coreInfo.currentPC, fixedCurrentPC,
        rawPCOffset, fixedPCOffset, errorDescription.c_str());

    const KernelSymbol* matchedSymbol = FindBestMatchedSymbol(kernelSymbols_.symbols, fixedPCOffset);
    if (matchedSymbol != nullptr) {
        IDE_LOGE("[Dump][Exception] Error symbol information. coreId=%u, coreType=%u, symbol=%s+0x%lx.",
            coreInfo.coreId, coreType, matchedSymbol->name.c_str(), fixedPCOffset - matchedSymbol->offset);
        return;
    }

    uint64_t minSymbolOffset = 0;
    uint64_t maxSymbolEnd = 0;
    const bool hasSymbolRange = GetSymbolOffsetRange(kernelSymbols_.symbols, minSymbolOffset, maxSymbolEnd);
    IDE_LOGE("[Dump][Exception] Not found error symbol information. coreId=%u, coreType=%u, "
        "symbolCount=%zu, hasSymbolRange=%u, minSymbolOffset=0x%lx, maxSymbolEnd=0x%lx, hasAicBase=%u, "
        "aicBase=0x%lx, hasAivBase=%u, aivBase=0x%lx.",
        coreInfo.coreId, coreType, kernelSymbols_.symbols.size(), static_cast<uint32_t>(hasSymbolRange),
        minSymbolOffset, maxSymbolEnd, static_cast<uint32_t>(kernelSymbols_.hasAicBase), kernelSymbols_.aicBase,
        static_cast<uint32_t>(kernelSymbols_.hasAivBase), kernelSymbols_.aivBase);
}

int32_t KernelSymbolLocator::LocateAndPrintErrorSymbols(const ExceptionRegInfo& exceptionRegInfo)
{
    IDE_CTRL_VALUE_WARN(initialized_, return ADUMP_FAILED, "KernelSymbolLocator not initialized.");

    IDE_CTRL_VALUE_WARN(
        exceptionRegInfo.errRegInfo != nullptr && exceptionRegInfo.coreNum != 0, return ADUMP_FAILED,
        "Exception register info is null or core num is zero.");

    for (uint32_t i = 0; i < exceptionRegInfo.coreNum; i++) {
        PrintErrorForCore(exceptionRegInfo.errRegInfo[i]);
    }
    return ADUMP_SUCCESS;
}

int32_t KernelSymbolLocator::LocateAndPrintErrorSymbolsForCore(
    uint32_t coreId, uint32_t coreType, ExceptionRegInfo exceptionRegInfo)
{
    IDE_CTRL_VALUE_WARN(initialized_, return ADUMP_FAILED, "KernelSymbolLocator not initialized.");

    IDE_CTRL_VALUE_WARN(exceptionRegInfo.errRegInfo != nullptr && exceptionRegInfo.coreNum != 0,
        return ADUMP_FAILED, "Exception register info is null or core num is zero.");

    const rtExceptionErrRegInfo_t* coreInfo = nullptr;
    for (uint32_t i = 0; i < exceptionRegInfo.coreNum; i++) {
        if (exceptionRegInfo.errRegInfo[i].coreId == coreId &&
            exceptionRegInfo.errRegInfo[i].coreType == static_cast<rtCoreType_t>(coreType)) {
            coreInfo = &exceptionRegInfo.errRegInfo[i];
            break;
        }
    }

    IDE_CTRL_VALUE_WARN(coreInfo != nullptr, return ADUMP_FAILED,
        "Core exception register info is not found, coreId=%u, coreType=%u.", coreId, coreType);

    PrintErrorForCore(*coreInfo);
    return ADUMP_SUCCESS;
}

uint64_t KernelSymbolLocator::FixPcByErrorRegs(const rtExceptionErrRegInfo_t& coreInfo)
{
    PcFixerInterface* fixer = PcFixerFactory::GetInstance();
    if (fixer == nullptr) {
        return coreInfo.currentPC;
    }
    return fixer->FixPc(coreInfo.currentPC, coreInfo.errReg, RT_ERR_REG_NUMS);
}

std::string KernelSymbolLocator::GetErrorDescription(const rtExceptionErrRegInfo_t& coreInfo)
{
    PcFixerInterface* fixer = PcFixerFactory::GetInstance();
    if (fixer == nullptr) {
        return "";
    }
    return fixer->GetErrorDescription(coreInfo.errReg, RT_ERR_REG_NUMS);
}

std::string KernelSymbolLocator::GetErrorRegisters(const rtExceptionErrRegInfo_t& coreInfo)
{
    PcFixerInterface* fixer = PcFixerFactory::GetInstance();
    if (fixer == nullptr) {
        return "";
    }
    return fixer->GetErrorRegisters(coreInfo.errReg, RT_ERR_REG_NUMS);
}

void KernelSymbolLocator::DumpErrorSymbols(const rtExceptionInfo& exception, ExceptionRegInfo& exceptionRegInfo)
{
    rtExceptionArgsInfo_t exceptionArgsInfo{};
    int32_t ret = ExceptionInfoCommon::GetExceptionInfo(exception, exceptionArgsInfo);
    IDE_CTRL_VALUE_FAILED(ret == ADUMP_SUCCESS, return, "Get exception args info failed, skip dump error symbols.");

    std::string binData;
    uint32_t binSize = 0;
    const rtExceptionKernelInfo_t& kernelInfo = exceptionArgsInfo.exceptionKernelInfo;
    ret = ExceptionInfoCommon::GetBinDataFromHandle(kernelInfo.bin, binData, binSize);
    IDE_CTRL_VALUE_FAILED(ret == ADUMP_SUCCESS, return, "Get kernel bin data failed, skip dump error symbols.");

    KernelSymbolLocator locator;
    ret = locator.InitFromBinBuffer(binData);
    IDE_CTRL_VALUE_FAILED(ret == ADUMP_SUCCESS, return, "Parse kernel symbols failed, skip dump error symbols.");
    // GE+SK 场景（te_superkernel 入口）查找偏移时需要叠加 mix 入口基址，且不更新 startPC。
    const std::string exceptionFuncName = ExceptionInfoCommon::GetExceptionFuncName(exception);
    locator.SetApplyEntryBase(StrUtils::StartsWith(exceptionFuncName, "te_superkernel"));

    ret = locator.LocateAndPrintErrorSymbols(exceptionRegInfo);
    IDE_CTRL_VALUE_WARN(ret == ADUMP_SUCCESS, return, "Locate kernel error symbols failed, ret=%d.", ret);
}

void KernelSymbolLocator::DumpErrorSymbols(const rtExceptionInfo& exception)
{
    ExceptionRegInfo exceptionRegInfo{0, nullptr};
    if (ExceptionInfoCommon::GetExceptionRegInfo(exception, exceptionRegInfo) == ADUMP_SUCCESS) {
        DumpErrorSymbols(exception, exceptionRegInfo);
    }
}

} // namespace Adx

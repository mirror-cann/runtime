/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include <gtest/gtest.h>
#include <mockcpp/mockcpp.hpp>
#include <string>
#include <map>
#define protected public
#define private public
#include "platform/platform_info.h"
#include "platform_infos_utils.h"
#undef protected
#undef private

namespace fe {

class PlatformInfoExceptionUTest : public testing::Test {
protected:
  void SetUp() {
    PlatformInfoManager::Instance().InitializePlatformInfo();
  }

  void TearDown() {
    PlatformInfoManager::Instance().Finalize();
    GlobalMockObject::verify();
  }
};

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_ValidFraction_Success) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "3/4";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.ub_to_l2_rate, 0.75);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_InvalidFraction_ReturnsZero) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "/invalid";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.ub_to_l2_rate, 0.0);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_ZeroDenominator_ReturnsZero) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "10/0";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.ub_to_l2_rate, 0.0);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_EmptyFraction_ReturnsZero) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_SlashAtEnd_ReturnsZero) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "10/";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseUnzipOfAICoreSpec_ExceedsRange_ReturnsFalse) {
  std::map<std::string, std::string> core_spec;
  core_spec["unzip_engines"] = "4294967296";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUnzipOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseUnzipOfAICoreSpec_InvalidString_ReturnsFalse) {
  std::map<std::string, std::string> core_spec;
  core_spec["unzip_engines"] = "not_a_number";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUnzipOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseUBOfAICoreSpec_ExceedsRange_ReturnsFalse) {
  std::map<std::string, std::string> core_spec;
  core_spec["ub_size"] = "18446744073709551616";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseBufferOfAICoreSpec_ExceedsRange_ReturnsFalse) {
  std::map<std::string, std::string> core_spec;
  core_spec["buffer_size"] = "4294967296";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseBufferOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseCubeOfAICoreSpec_ExceedsRange_ReturnsFalse) {
  std::map<std::string, std::string> core_spec;
  core_spec["cube_size"] = "18446744073709551616";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseCubeOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, SetCoreNumByCoreType_InvalidNumber_CatchesException) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  std::map<std::string, std::string> soc_info;
  soc_info["ai_core_cnt"] = "invalid_number";
  platform_infos.SetPlatformRes("SoCInfo", soc_info);
  platform_infos.SetCoreNumByCoreType("AICore");
}

TEST_F(PlatformInfoExceptionUTest, SetCoreNumByCoreType_MixVectorCore_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  std::map<std::string, std::string> soc_info;
  soc_info["ai_core_cnt"] = "32";
  soc_info["mix_vector_core_cnt"] = "8";
  platform_infos.SetPlatformRes("SoCInfo", soc_info);
  platform_infos.SetCoreNumByCoreType("MIX_VECTOR_CORE");
  EXPECT_EQ(platform_infos.core_num_, 8);
}

TEST_F(PlatformInfoExceptionUTest, ParseSocInfo_InvalidString_CatchesException) {
  std::map<std::string, std::string> soc_info;
  soc_info["ai_core_cnt"] = "not_a_number";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseSocInfo(soc_info, info);
}

TEST_F(PlatformInfoExceptionUTest, GetPlatformRes_WithLock_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  std::map<std::string, std::string> res_map;
  res_map["key1"] = "value1";
  platform_infos.SetPlatformRes("TestSection", res_map);
  
  std::map<std::string, std::string> result;
  EXPECT_TRUE(platform_infos.GetPlatformResWithLock("TestSection", result));
  EXPECT_EQ(result["key1"], "value1");
}

TEST_F(PlatformInfoExceptionUTest, GetPlatformRes_MapMap_WithLock_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  std::map<std::string, std::map<std::string, std::string>> res_map_map;
  std::map<std::string, std::string> inner_map;
  inner_map["inner_key"] = "inner_value";
  res_map_map["outer_key"] = inner_map;
  
  std::map<std::string, std::map<std::string, std::string>> result;
  EXPECT_TRUE(platform_infos.GetPlatformResWithLock(result));
}

TEST_F(PlatformInfoExceptionUTest, SetPlatformRes_WithLock_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  std::map<std::string, std::string> res;
  res["key1"] = "value1";
  res["key2"] = "value2";
  platform_infos.SetPlatformResWithLock("TestLabel", res);
}

TEST_F(PlatformInfoExceptionUTest, LoadFromBuffer_NullBuffer_ReturnsTrueInUT) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
#ifdef _OPEN_SOURCE_LLT_
  EXPECT_TRUE(platform_infos.LoadFromBuffer(nullptr, 0));
#else
  EXPECT_FALSE(platform_infos.LoadFromBuffer(nullptr, 0));
#endif
}

TEST_F(PlatformInfoExceptionUTest, LoadFromBuffer_ShortBuffer_ReturnsTrueInUT) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  const char *buffer = "short";
#ifdef _OPEN_SOURCE_LLT_
  EXPECT_TRUE(platform_infos.LoadFromBuffer(buffer, strlen(buffer)));
#else
  EXPECT_FALSE(platform_infos.LoadFromBuffer(buffer, strlen(buffer)));
#endif
}

TEST_F(PlatformInfoExceptionUTest, GetLocalMemSize_L2_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  std::map<std::string, std::string> core_spec;
  core_spec["l2_size"] = "1048576";
  platform_infos.SetPlatformRes("AICoreSpec", core_spec);
  
  uint64_t size = 0;
  platform_infos.GetLocalMemSize(LocalMemType::L2, size);
}

TEST_F(PlatformInfoExceptionUTest, GetLocalMemBw_L1_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  std::map<std::string, std::string> memory_rates;
  memory_rates["l1_rate"] = "1.5";
  platform_infos.SetPlatformRes("AICoreMemoryRates", memory_rates);
  
  uint64_t bw = 0;
  platform_infos.GetLocalMemBw(LocalMemType::L1, bw);
}

TEST_F(PlatformInfoExceptionUTest, GetLocalMemBw_L2_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  std::map<std::string, std::string> memory_rates;
  memory_rates["l2_rate"] = "2.0";
  platform_infos.SetPlatformRes("AICoreMemoryRates", memory_rates);
  
  uint64_t bw = 0;
  platform_infos.GetLocalMemBw(LocalMemType::L2, bw);
}

TEST_F(PlatformInfoExceptionUTest, GetCoreNumByType_AIV_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  std::map<std::string, std::string> soc_info;
  soc_info["vector_core_cnt"] = "16";
  platform_infos.SetPlatformRes("SoCInfo", soc_info);
  
  EXPECT_EQ(platform_infos.GetCoreNumByType("AIV"), 16);
}

TEST_F(PlatformInfoExceptionUTest, GetCoreNumByType_MixAICORE_ReturnsAICoreCnt) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  std::map<std::string, std::string> soc_info;
  soc_info["ai_core_cnt"] = "32";
  platform_infos.SetPlatformRes("SoCInfo", soc_info);
  
  EXPECT_EQ(platform_infos.GetCoreNumByType("MIX_AICORE"), 32);
}

TEST_F(PlatformInfoExceptionUTest, ParseVersion_InvalidFormat_ReturnsFalse) {
  std::map<std::string, std::string> version_map;
  version_map["invalid_key"] = "value";
  std::string soc_version = "Ascend910";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseVersion(version_map, soc_version, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseAICoreMemoryRates_DDRRate_Success) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ddr_rate"] = "3.5";
  memory_rates["ddr_read_rate"] = "2.0";
  memory_rates["ddr_write_rate"] = "1.5";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseBufferOfAICoreMemoryRates(memory_rates, info);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.ddr_rate, 3.5);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.ddr_read_rate, 2.0);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.ddr_write_rate, 1.5);
}

TEST_F(PlatformInfoExceptionUTest, ParseAICoreMemoryRates_L2Rate_Success) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["l2_rate"] = "2.8";
  memory_rates["l2_read_rate"] = "1.4";
  memory_rates["l2_write_rate"] = "1.4";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseBufferOfAICoreMemoryRates(memory_rates, info);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.l2_rate, 2.8);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.l2_read_rate, 1.4);
  EXPECT_DOUBLE_EQ(info.ai_core_memory_rates.l2_write_rate, 1.4);
}

TEST_F(PlatformInfoExceptionUTest, ParseSoftwareSpec_Success) {
  std::map<std::string, std::string> software_spec;
  software_spec["cubebit_num"] = "128";
  software_spec["line_num"] = "64";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseSoftwareSpec(software_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseCPUCache_Success) {
  std::map<std::string, std::string> cpu_cache;
  cpu_cache["l1_cache_size"] = "65536";
  cpu_cache["l2_cache_size"] = "131072";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseCPUCache(cpu_cache, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseVectorCoreSpec_Success) {
  std::map<std::string, std::string> vector_core_spec;
  vector_core_spec["vector_core_cnt"] = "16";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseVectorCoreSpec(vector_core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseVectorCoreMemoryRates_Success) {
  std::map<std::string, std::string> vector_memory_rates;
  vector_memory_rates["ddr_rate"] = "2.5";
  vector_memory_rates["l2_rate"] = "1.8";
  vector_memory_rates["ub_to_l2_rate"] = "3.0";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseVectorCoreMemoryRates(vector_memory_rates, info);
}

TEST_F(PlatformInfoExceptionUTest, UpdatePlatformInfos_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  platform_infos.core_num_ = 8;
  
  PlatformInfoManager::Instance().UpdatePlatformInfos(platform_infos);
}

TEST_F(PlatformInfoExceptionUTest, UpdatePlatformInfos_WithSocVersion_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  
  PlatformInfoManager::Instance().UpdatePlatformInfos("Ascend910B", platform_infos);
}

TEST_F(PlatformInfoExceptionUTest, InitRuntimePlatformInfos_Success) {
  PlatformInfoManager::Instance().InitRuntimePlatformInfos("Ascend910B");
}

TEST_F(PlatformInfoExceptionUTest, GetRuntimePlatformInfosByDevice_Success) {
  PlatFormInfos platform_infos;
  EXPECT_EQ(PlatformInfoManager::Instance().GetRuntimePlatformInfosByDevice(0, platform_infos, false), 0);
}

TEST_F(PlatformInfoExceptionUTest, UpdateRuntimePlatformInfosByDevice_Success) {
  PlatFormInfos platform_infos;
  platform_infos.Init();
  PlatformInfoManager::Instance().UpdateRuntimePlatformInfosByDevice(0, platform_infos);
}

TEST_F(PlatformInfoExceptionUTest, GetPlatformInstanceByDevice_Success) {
  OptionalInfos opti_infos;
  opti_infos.Init();
  opti_infos.SetSocVersion("Ascend910B1");
  PlatformInfoManager::Instance().SetOptionalCompilationInfo(opti_infos);
  
  PlatFormInfos platform_infos;
  EXPECT_EQ(PlatformInfoManager::Instance().GetPlatformInstanceByDevice(0, platform_infos), 0);
}

TEST_F(PlatformInfoExceptionUTest, GetPlatformInstanceByDevice_EmptySocVersion_Failed) {
  OptionalInfos opti_infos;
  opti_infos.Init();
  PlatformInfoManager::Instance().SetOptionalCompilationInfo(opti_infos);
  
  PlatFormInfos platform_infos;
  EXPECT_EQ(PlatformInfoManager::Instance().GetPlatformInstanceByDevice(0, platform_infos), 0xFFFFFFFF);
}

// 新增测试：覆盖 CalculateFraction 更多边界情况
TEST_F(PlatformInfoExceptionUTest, CalculateFraction_NegativeDenominator_Success) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "10/-2";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_NegativeNumerator_Success) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "-10/2";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_BothNegative_Success) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "-10/-2";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_VerySmallDenominator_ReturnsZero) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "10/1e-15";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_SpaceAroundDenominator_Success) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "10/ 2 ";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

// 新增测试：覆盖 Parse 函数的异常分支
TEST_F(PlatformInfoExceptionUTest, ParseSocInfo_AllFields_Success) {
  std::map<std::string, std::string> soc_info;
  soc_info["ai_core_cnt"] = "32";
  soc_info["vector_core_cnt"] = "16";
  soc_info["ai_cpu_cnt"] = "8";
  soc_info["memory_type"] = "0";
  soc_info["memory_size"] = "1048576";
  soc_info["l2_type"] = "0";
  soc_info["l2_size"] = "524288";
  soc_info["l2_page_num"] = "1024";
  soc_info["task_num"] = "64";
  soc_info["arch_type"] = "2201";
  soc_info["chip_type"] = "10";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseSocInfo(soc_info, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseAICoreSpec_AllFields_Success) {
  std::map<std::string, std::string> core_spec;
  core_spec["cube_freq"] = "1.5";
  core_spec["cube_m_size"] = "16";
  core_spec["cube_n_size"] = "16";
  core_spec["cube_k_size"] = "16";
  core_spec["vec_calc_size"] = "256";
  core_spec["l0_a_size"] = "1024";
  core_spec["l0_b_size"] = "1024";
  core_spec["l0_c_size"] = "1024";
  core_spec["l1_size"] = "4096";
  core_spec["smask_buffer"] = "512";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseUBOfAICoreSpec_AllFields_Success) {
  std::map<std::string, std::string> ub_spec;
  ub_spec["ub_size"] = "65536";
  ub_spec["ubblock_size"] = "4096";
  ub_spec["ubbank_size"] = "256";
  ub_spec["ubbank_num"] = "32";
  ub_spec["ubburst_in_one_block"] = "8";
  ub_spec["ubbank_group_num"] = "4";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreSpec(ub_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseVectorCoreSpec_AllFields_Success) {
  std::map<std::string, std::string> vector_spec;
  vector_spec["vec_freq"] = "2.0";
  vector_spec["vec_calc_size"] = "512";
  vector_spec["smask_buffer"] = "1024";
  vector_spec["ub_size"] = "32768";
  vector_spec["ubblock_size"] = "2048";
  vector_spec["ubbank_size"] = "128";
  vector_spec["ubbank_num"] = "16";
  vector_spec["ubburst_in_one_block"] = "4";
  vector_spec["ubbank_group_num"] = "2";
  vector_spec["vector_reg_size"] = "64";
  vector_spec["predicate_reg_size"] = "32";
  vector_spec["address_reg_size"] = "16";
  vector_spec["alignment_reg_size"] = "8";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseVectorCoreSpec(vector_spec, info);
}

// 新增测试：覆盖 EnsureSocVersionLoaded 异常分支
TEST_F(PlatformInfoExceptionUTest, EnsureSocVersionLoaded_Success) {
  std::string soc_version = "Ascend910B1";
  EXPECT_EQ(PlatformInfoManager::Instance().EnsureSocVersionLoaded(soc_version), 0);
}

// 新增测试：覆盖 ParseUnzipOfAICoreSpec 超范围分支
TEST_F(PlatformInfoExceptionUTest, ParseUnzipOfAICoreSpec_ExceedsMaxRatios_Returns) {
  std::map<std::string, std::string> core_spec;
  core_spec["unzip_max_ratios"] = "4294967296"; // UINT32_MAX + 1
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUnzipOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseUnzipOfAICoreSpec_ExceedsChannels_Returns) {
  std::map<std::string, std::string> core_spec;
  core_spec["unzip_channels"] = "4294967296";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUnzipOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseUnzipOfAICoreSpec_ExceedsIsTight_Returns) {
  std::map<std::string, std::string> core_spec;
  core_spec["unzip_is_tight"] = "256"; // UINT8_MAX + 1
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUnzipOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseUnzipOfAICoreSpec_OutOfRangeException) {
  std::map<std::string, std::string> core_spec;
  core_spec["unzip_engines"] = "999999999999999999999999"; // 超大值触发out_of_range
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUnzipOfAICoreSpec(core_spec, info);
}

// 新增测试：覆盖 CalculateFraction 异常分支
TEST_F(PlatformInfoExceptionUTest, CalculateFraction_InvalidInputFormat) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "not_a_number/2";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

TEST_F(PlatformInfoExceptionUTest, CalculateFraction_ExceptionInParse) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l2_rate"] = "abc";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

// 新增测试：覆盖 ParseBufferOfAICoreMemoryRates 异常分支
TEST_F(PlatformInfoExceptionUTest, ParseBufferOfAICoreMemoryRates_InvalidValue) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ddr_rate"] = "invalid_string";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseBufferOfAICoreMemoryRates(memory_rates, info);
}

// 新增测试：覆盖 ParseUBOfAICoreMemoryRates 异常分支
TEST_F(PlatformInfoExceptionUTest, ParseUBOfAICoreMemoryRates_InvalidUbToL1) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ub_to_l1_rate"] = "invalid_value";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreMemoryRates(memory_rates, info);
}

// 新增测试：覆盖其他Parse函数的异常分支
TEST_F(PlatformInfoExceptionUTest, ParseCubeOfAICoreSpec_ExceedsRange) {
  std::map<std::string, std::string> core_spec;
  core_spec["cube_m_size"] = "18446744073709551616";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseCubeOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseBufferOfAICoreSpec_ExceedsRange) {
  std::map<std::string, std::string> core_spec;
  core_spec["buffer_size"] = "4294967296";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseBufferOfAICoreSpec(core_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseUBOfAICoreSpec_ExceedsRange) {
  std::map<std::string, std::string> ub_spec;
  ub_spec["ub_size"] = "18446744073709551616";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseUBOfAICoreSpec(ub_spec, info);
}

TEST_F(PlatformInfoExceptionUTest, ParseVectorCoreMemoryRates_Exception) {
  std::map<std::string, std::string> memory_rates;
  memory_rates["ddr_rate"] = "invalid";
  PlatformInfo info;
  PlatformInfoManager::Instance().ParseVectorCoreMemoryRates(memory_rates, info);
}

}
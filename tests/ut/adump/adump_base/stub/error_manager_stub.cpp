/**
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * This program is free software, you can redistribute it and/or modify it under the terms and conditions of
 * CANN Open Software License Agreement Version 2.0 (the "License").
 * Please refer to the License for details. You may not use this file except in compliance with the License.
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE.
 * See LICENSE in the root of the software repository for the full text of the License.
 */
#include "error_manager.h"
#include <mutex>
#include <iostream>

namespace {
const char *const kErrorCodePath = "../conf/error_manager/error_code.json";
const char *const kErrorList = "error_info_list";
const char *const kErrCode = "ErrCode";
const char *const kErrMessage = "ErrMessage";
const char *const kArgList = "Arglist";
const uint64_t kLength = 2;

static std::string g_lastErrorCode;
static std::mutex g_errorMutex;
}  // namespace

std::string GetLastReportedErrorCode() {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    return g_lastErrorCode;
}

void ClearLastReportedErrorCode() {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    g_lastErrorCode.clear();
}

///
/// @brief Obtain ErrorManager instance
/// @return ErrorManager instance
///
ErrorManager &ErrorManager::GetInstance() {
  static ErrorManager instance;
  return instance;
}

int ErrorManager::Init(std::string /* path */) {
  return 0;
}

int ErrorManager::Init() {
  return 0;
}

///
/// @brief report error message
/// @param [in] error_code: error code
/// @param [in] args_map: parameter map
/// @return int 0(success) -1(fail)
///
int ErrorManager::ReportErrMessage(std::string /* error_code */, const std::map<std::string, std::string> &/* args_map */) {
  return 0;
}

///
/// @brief output error message
/// @param [in] handle: print handle
/// @return int 0(success) -1(fail)
///
int ErrorManager::OutputErrMessage(int /* handle */) {
  return 0;
}

///
/// @brief output message
/// @param [in] handle: print handle
/// @return int 0(success) -1(fail)
///
int ErrorManager::OutputMessage(int /* handle */) {
  return 0;
}

///
/// @brief parse json file
/// @param [in] path: json path
/// @return int 0(success) -1(fail)
///
int ErrorManager::ParseJsonFile(std::string /* path */) {
  return 0;
}

///
/// @brief read json file
/// @param [in] file_path: json path
/// @param [in] handle:  print handle
/// @return int 0(success) -1(fail)
///
int ErrorManager::ReadJsonFile(const std::string &/* file_path */, void */* handle */) {
  return 0;
}

///
/// @brief report error message
/// @param [in] error_code: error code
/// @param [in] vector parameter ky, vector parameter value
/// @return int 0(success) -1(fail)
///
static std::string FormatErrorCode(const std::string &error_code,
                                   const std::vector<std::string> &key,
                                   const std::vector<std::string> &value) {
    static const std::map<std::string, std::string> errorTemplates = {
        {"EP0001", "The content of configuration item %s in configuration file %s is invalid. Reason: %s."},
        {"EP0002", "Value %s for configuration item %s in configuration file %s is invalid. Expected value: %s."},
        {"EP0003", "Value %s for configuration item %s in configuration file %s is invalid. Reason: %s."},
        {"EP0004", "Failed to parse file %s. Reason: %s."},
        {"EP0005", "Conflict of configuration items in configuration file %s. Reason: %s."},
        {"EP0006", "%s failed. Value %s for parameter %s is invalid. Reason: %s"},
        {"EP0007", "%s failed because %s cannot be a NULL pointer."},
        {"EP0008", "%s failed. Reason: %s."}
    };
    
    auto it = errorTemplates.find(error_code);
    if (it == errorTemplates.end()) {
        return error_code + ": Unknown error.";
    }
    
    std::string templateStr = it->second;
    if (key.size() != value.size()) {
        return error_code + ": " + templateStr;
    }
    
    // Replace placeholders with actual values using %s format
    size_t placeholderPos = 0;
    for (size_t i = 0; i < value.size(); ++i) {
        placeholderPos = templateStr.find("%s", placeholderPos);
        if (placeholderPos != std::string::npos) {
            templateStr.replace(placeholderPos, 2, value[i]);
            placeholderPos += value[i].length();
        }
    }
    
    return error_code + ": " + templateStr;
}

void ErrorManager::ATCReportErrMessage(std::string error_code, const std::vector<std::string> &key,
                                       const std::vector<std::string> &value) {
    std::lock_guard<std::mutex> lock(g_errorMutex);
    g_lastErrorCode = error_code;
    
    std::string errMsg = FormatErrorCode(error_code, key, value);
    std::cout << "[ERROR] " << errMsg << std::endl;
}

///
/// @brief get graph compile failed message in mustune case
/// @param [in] graph_name: graph name
/// @param [out] msg_map: failed message map, ky is error code, value is op_name list
/// @return int 0(success) -1(fail)
///
int ErrorManager::GetMstuneCompileFailedMsg(const std::string &/* graph_name */, std::map<std::string,
  std::vector<std::string>> &/* msg_map */) {
  return 0;
}

int32_t ErrorManager::ReportInterErrMessage(const std::string /* error_code */, const std::string &/* error_msg */) {
  return 0;
}

int32_t error_message::FormatErrorMessage(char_t */* str_dst */, size_t /* dst_max */, const char_t */* format */, ...) {
  return 0;
}

void error_message::ReportInnerError(const char_t *file_name, const char_t *func, uint32_t line, const std::string error_code,
                      const char_t *format, ...) {
  return;
}
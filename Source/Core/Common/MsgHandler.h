// Copyright 2009 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include <fmt/format.h>

#include "Common/FormatUtil.h"
#include "Common/Logging/Log.h"

namespace Common
{
// Message alerts
enum class MsgType
{
  Information,
  Question,
  Warning,
  Critical
};

using MsgAlertHandler = bool (*)(const char* caption, const char* text, bool yes_no, MsgType style);
using StringTranslator = std::string (*)(const char* text);

void RegisterMsgAlertHandler(MsgAlertHandler handler);
void RegisterStringTranslator(StringTranslator translator);

std::string GetStringT(const char* string);

bool MsgAlertFmtImpl(bool yes_no, MsgType style, Common::Log::LogType log_type,
                     fmt::string_view format, const fmt::format_args& args);

template <std::size_t NumFields, typename S, typename... Args>
bool MsgAlertFmt(bool yes_no, MsgType style, Common::Log::LogType log_type, const S& format,
                 const Args&... args)
{
  static_assert(NumFields == sizeof...(args),
                "Unexpected number of replacement fields in format string; did you pass too few or "
                "too many arguments?");
  static_assert(fmt::is_compile_string<S>::value);
  return MsgAlertFmtImpl(yes_no, style, log_type, format,
                         fmt::make_args_checked<Args...>(format, args...));
}

template <std::size_t NumFields, bool has_non_positional_args, typename S, typename... Args>
bool MsgAlertFmtT(bool yes_no, MsgType style, Common::Log::LogType log_type, const S& format,
                  fmt::string_view translated_format, const Args&... args)
{
  static_assert(!has_non_positional_args,
                "Translatable strings must use positional arguments (e.g. {0} instead of {})");
  static_assert(NumFields == sizeof...(args),
                "Unexpected number of replacement fields in format string; did you pass too few or "
                "too many arguments?");
  static_assert(fmt::is_compile_string<S>::value);
  // It's only possible for us to compile-time check the English-language string.
  // make_args_checked uses static_asserts to verify that a string is formattable with the given
  // arguments.  But it can't do that if the string varies at runtime, so we can't check
  // translations.  Still, verifying that the English string is correct will help ensure that
  // translations use valid strings.
  auto arg_list = fmt::make_args_checked<Args...>(format, args...);
  return MsgAlertFmtImpl(yes_no, style, log_type, translated_format, arg_list);
}

void SetEnableAlert(bool enable);
void SetAbortOnPanicAlert(bool should_abort);

// Like fmt::format, except the string becomes translatable
template <typename... Args>
std::string FmtFormatT(const char* string, Args&&... args)
{
  return fmt::format(Common::GetStringT(string), std::forward<Args>(args)...);
}
}  // namespace Common

// Fmt-capable variants of the macros

#define GenericAlertFmt(yes_no, style, log_type, format, ...)                                      \
  Common::MsgAlertFmt<Common::CountFmtReplacementFields(format)>(                                  \
      yes_no, style, Common::Log::LogType::log_type, FMT_STRING(format), ##__VA_ARGS__)

#define GenericAlertFmtT(yes_no, style, log_type, format, ...)                                     \
  Common::MsgAlertFmtT<Common::CountFmtReplacementFields(format),                                  \
                       Common::ContainsNonPositionalArguments(format)>(                            \
      yes_no, style, Common::Log::LogType::log_type, FMT_STRING(format),                           \
      Common::GetStringT(format), ##__VA_ARGS__)

#define SuccessAlertFmt(format, ...)                                                               \
  GenericAlertFmt(false, Common::MsgType::Information, MASTER_LOG, format, ##__VA_ARGS__)

#define PanicAlertFmt(format, ...)                                                                 \
  GenericAlertFmt(false, Common::MsgType::Warning, MASTER_LOG, format, ##__VA_ARGS__)

#define PanicYesNoFmt(format, ...)                                                                 \
  GenericAlertFmt(true, Common::MsgType::Warning, MASTER_LOG, format, ##__VA_ARGS__)

#define AskYesNoFmt(format, ...)                                                                   \
  GenericAlertFmt(true, Common::MsgType::Question, MASTER_LOG, format, ##__VA_ARGS__)

#define CriticalAlertFmt(format, ...)                                                              \
  GenericAlertFmt(false, Common::MsgType::Critical, MASTER_LOG, format, ##__VA_ARGS__)

// Use these macros (that do the same thing) if the message should be translated.
#define SuccessAlertFmtT(format, ...)                                                              \
  GenericAlertFmtT(false, Common::MsgType::Information, MASTER_LOG, format, ##__VA_ARGS__)

#define PanicAlertFmtT(format, ...)                                                                \
  GenericAlertFmtT(false, Common::MsgType::Warning, MASTER_LOG, format, ##__VA_ARGS__)

#define PanicYesNoFmtT(format, ...)                                                                \
  GenericAlertFmtT(true, Common::MsgType::Warning, MASTER_LOG, format, ##__VA_ARGS__)

#define AskYesNoFmtT(format, ...)                                                                  \
  GenericAlertFmtT(true, Common::MsgType::Question, MASTER_LOG, format, ##__VA_ARGS__)

#define CriticalAlertFmtT(format, ...)                                                             \
  GenericAlertFmtT(false, Common::MsgType::Critical, MASTER_LOG, format, ##__VA_ARGS__)

// Variant that takes a log type, used by the assert macros
#define PanicYesNoFmtAssert(log_type, format, ...)                                                 \
  GenericAlertFmt(true, Common::MsgType::Warning, log_type, format, ##__VA_ARGS__)

// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/multidevice_internals/multidevice_internals_logs_handler.h"

#include "ash/components/multidevice/logging/log_buffer.h"
#include "base/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/values.h"

namespace chromeos {

namespace multidevice {

namespace {

// Keys in the JSON representation of a log message
const char kLogMessageTextKey[] = "text";
const char kLogMessageTimeKey[] = "time";
const char kLogMessageFileKey[] = "file";
const char kLogMessageLineKey[] = "line";
const char kLogMessageSeverityKey[] = "severity";

// Converts |log_message| to a raw dictionary value used as a JSON argument to
// JavaScript functions.
base::Value LogMessageToDictionary(const LogBuffer::LogMessage& log_message) {
  base::Value dictionary(base::Value::Type::DICTIONARY);
  dictionary.SetStringKey(kLogMessageTextKey, log_message.text);
  dictionary.SetStringKey(
      kLogMessageTimeKey,
      base::TimeFormatTimeOfDayWithMilliseconds(log_message.time));
  dictionary.SetStringKey(kLogMessageFileKey, log_message.file);
  dictionary.SetIntKey(kLogMessageLineKey, log_message.line);
  dictionary.SetIntKey(kLogMessageSeverityKey, log_message.severity);
  return dictionary;
}

}  // namespace

MultideviceLogsHandler::MultideviceLogsHandler() {}

MultideviceLogsHandler::~MultideviceLogsHandler() = default;

void MultideviceLogsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "getMultideviceLogMessages",
      base::BindRepeating(&MultideviceLogsHandler::HandleGetLogMessages,
                          base::Unretained(this)));
}

void MultideviceLogsHandler::OnJavascriptAllowed() {
  observation_.Observe(LogBuffer::GetInstance());
}

void MultideviceLogsHandler::OnJavascriptDisallowed() {
  observation_.Reset();
}

void MultideviceLogsHandler::HandleGetLogMessages(
    const base::Value::List& args) {
  AllowJavascript();
  const base::Value& callback_id = args[0];
  base::Value::List list;
  for (const auto& log : *LogBuffer::GetInstance()->logs()) {
    list.Append(LogMessageToDictionary(log));
  }
  ResolveJavascriptCallback(callback_id, base::Value(std::move(list)));
}

void MultideviceLogsHandler::OnLogBufferCleared() {
  FireWebUIListener("multidevice-log-buffer-cleared");
}

void MultideviceLogsHandler::OnLogMessageAdded(
    const LogBuffer::LogMessage& log_message) {
  FireWebUIListener("multidevice-log-message-added",
                    LogMessageToDictionary(log_message));
}

}  // namespace multidevice

}  // namespace chromeos

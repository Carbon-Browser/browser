// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/debug/debugger_utils.h"

#include <inttypes.h>

#include <string>

#include "base/logging.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"

namespace views::debug {

namespace {

using AttributeStrings = std::vector<std::string>;

constexpr int kElementIndent = 2;
constexpr int kAttributeIndent = 4;

std::string ToString(bool val) {
  return val ? "true" : "false";
}

std::string ToString(int val) {
  return base::NumberToString(val);
}

std::string ToString(ViewDebugWrapper::BoundsTuple bounds) {
  return base::StringPrintf("%d %d %dx%d", std::get<0>(bounds),
                            std::get<1>(bounds), std::get<2>(bounds),
                            std::get<3>(bounds));
}

// intptr_t can alias to int, preventing the use of overloading for ToString.
std::string PtrToString(intptr_t val) {
  return base::StringPrintf("0x%" PRIxPTR, val);
}

// Adds attribute string of the form <attribute_name>="<attribute_value>".
template <typename T>
void AddAttributeString(AttributeStrings& attributes,
                        const std::string& name,
                        const T& value) {
  attributes.push_back(name + "=\"" + ToString(value) + "\"");
}

void AddPtrAttributeString(AttributeStrings& attributes,
                           const std::string& name,
                           const std::optional<intptr_t>& value) {
  if (!value) {
    return;
  }

  attributes.push_back(name + "=\"" + PtrToString(value.value()) + "\"");
}

AttributeStrings GetAttributeStrings(ViewDebugWrapper* view, bool verbose) {
  AttributeStrings attributes;
  if (verbose) {
    view->ForAllProperties(base::BindRepeating(
        [](AttributeStrings* attributes, const std::string& name,
           const std::string& val) {
          attributes->push_back(name + "=\"" + val + "\"");
        },
        base::Unretained(&attributes)));
  } else {
    AddPtrAttributeString(attributes, "address", view->GetAddress());
    AddAttributeString(attributes, "bounds", view->GetBounds());
    AddAttributeString(attributes, "enabled", view->GetEnabled());
    AddAttributeString(attributes, "id", view->GetID());
    AddAttributeString(attributes, "needs-layout", view->GetNeedsLayout());
    AddAttributeString(attributes, "visible", view->GetVisible());
  }
  return attributes;
}

std::string GetPaddedLine(int current_depth, bool attribute_line = false) {
  const int padding = attribute_line
                          ? current_depth * kElementIndent + kAttributeIndent
                          : current_depth * kElementIndent;
  return std::string(padding, ' ');
}

std::string PrintViewHierarchyImpl(ViewDebugWrapper* view,
                                   bool verbose,
                                   int current_depth) {
  std::string output;
  std::string line = base::StrCat(
      {GetPaddedLine(current_depth), "<", view->GetViewClassName()});

  for (const std::string& attribute_string :
       GetAttributeStrings(view, verbose)) {
    static constexpr size_t kColumnLimit = 240;
    if (line.size() + attribute_string.size() + 1 > kColumnLimit) {
      // If adding the attribute string would cause the line to exceed the
      // column limit, send the line to `out` and start a new line. The new line
      // should fit at least one attribute string even if it means exceeding the
      // column limit.
      output += line;
      output += "\n";
      line = GetPaddedLine(current_depth, true) + attribute_string;
    } else {
      // Keep attribute strings on the existing line if it fits within the
      // column limit.
      line += " ";
      line += attribute_string;
    }
  }

  // Print children only if they exist and we are not yet at our target tree
  // depth.
  output += line;
  if (!view->GetChildren().empty()) {
    output += ">\n";

    for (ViewDebugWrapper* child : view->GetChildren()) {
      output += PrintViewHierarchyImpl(child, verbose, current_depth + 1);
    }

    output += base::StrCat(
        {GetPaddedLine(current_depth), "</", view->GetViewClassName(), ">\n"});
  } else {
    // If no children are to be printed use a self closing tag to terminate the
    // View element.
    output += " />\n";
  }
  return output;
}

}  // namespace

std::optional<intptr_t> ViewDebugWrapper::GetAddress() {
  return std::nullopt;
}

std::string PrintViewHierarchy(ViewDebugWrapper* view, bool verbose) {
  return PrintViewHierarchyImpl(view, verbose, 0);
}

}  // namespace views::debug

// This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
//
// Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <vector>

#include "string_piece.h"

namespace ufal {
namespace microrestd {

// Declarations
class json_builder {
 public:
  // Clear
  inline json_builder& clear();

  // Raw appending
  inline json_builder& append(string_piece data);

  // Encode
  inline json_builder& object();
  inline json_builder& array();
  inline json_builder& close();
  inline json_builder& key(string_piece str);
  inline json_builder& value(string_piece str);
  inline json_builder& value_xml_content(string_piece str);
  inline json_builder& value_open();
  inline json_builder& value_append(string_piece str);
  inline json_builder& value_xml_content_append(string_piece str);
  inline json_builder& value_close();
  inline json_builder& compact(bool compact);

  // Close all open objects and arrays
  inline json_builder& close_all();

  // Return current json
  inline string_piece current();

  // JSON mime
  static const char* mime;

 private:
  // Remove current json prefix; for json_response_generator
  friend class json_response_generator;
  void discard_prefix(size_t length);

 private:
  inline void start_element(bool key);
  void encode(string_piece str);
  void encode_xml_content(string_piece str);

  std::vector<char> json;
  std::vector<char> stack;
  bool comma_needed = false;
  bool compacting = false;
};


// Definitions
json_builder& json_builder::clear() {
  json.clear();
  stack.clear();
  comma_needed = false;
  return *this;
}

json_builder& json_builder::append(string_piece data) {
  json.insert(json.end(), data.str, data.str + data.len);
  return *this;
}

json_builder& json_builder::object() {
  start_element(false);
  json.push_back('{');
  if (!compacting) json.push_back('\n');
  stack.push_back('}');
  comma_needed = false;
  return *this;
}

json_builder& json_builder::array() {
  start_element(false);
  json.push_back('[');
  if (!compacting) json.push_back('\n');
  stack.push_back(']');
  comma_needed = false;
  return *this;
}

json_builder& json_builder::close() {
  if (!stack.empty()) {
    if (!compacting) json.insert(json.end(), stack.size() - 1, ' ');
    json.push_back(stack.back());
    if (!compacting) json.push_back('\n');
    stack.pop_back();
    comma_needed = true;
  }
  return *this;
}

json_builder& json_builder::key(string_piece str) {
  start_element(true);
  json.push_back('"');
  encode(str);
  json.push_back('"');
  json.push_back(':');
  if (!compacting) json.push_back(' ');
  comma_needed = false;
  return *this;
}

json_builder& json_builder::value(string_piece str) {
  return value_open().value_append(str).value_close();
}

json_builder& json_builder::value_xml_content(string_piece str) {
  return value_open().value_xml_content_append(str).value_close();
}

json_builder& json_builder::value_open() {
  start_element(false);
  json.push_back('"');
  return *this;
}

json_builder& json_builder::value_append(string_piece str) {
  encode(str);
  return *this;
}

json_builder& json_builder::value_xml_content_append(string_piece str) {
  encode_xml_content(str);
  return *this;
}

json_builder& json_builder::value_close() {
  json.push_back('"');
  if (!compacting) json.push_back('\n');
  comma_needed = true;
  return *this;
}

json_builder& json_builder::compact(bool compact) {
  compacting = compact;
  if (!compacting && !json.empty() && json.back() != '\n') json.push_back('\n');
  return *this;
}

json_builder& json_builder::close_all() {
  while (!stack.empty()) close();
  return *this;
}

string_piece json_builder::current() {
  return string_piece(json.data(), json.size());
}

void json_builder::start_element(bool key) {
  if (!compacting && !stack.empty() && (stack.back() != '}' || key)) json.insert(json.end(), stack.size() - (comma_needed ? 1 : 0), ' ');
  if (comma_needed && (stack.empty() || stack.back() != '}' || key)) json.push_back(',');
}

} // namespace microrestd
} // namespace ufal

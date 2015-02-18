// This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
//
// Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>
#include <vector>

#include "string_piece.h"

namespace ufal {
namespace microrestd {

// Declarations
class xml_builder {
 public:
  // Clear
  inline xml_builder& clear();

  // Encode
  inline xml_builder& element(string_piece name);
  inline xml_builder& attribute(string_piece name, string_piece value);
  inline xml_builder& text(string_piece str);
  inline xml_builder& close();
  inline xml_builder& compact(bool compact);

  // Raw appending
  inline xml_builder& append_raw(string_piece data);

  // Close all open objects and arrays
  inline xml_builder& close_all();

  // Return current xml
  inline string_piece current() const;

  // Remove current xml prefix; for response_generator
  void discard_current_prefix(size_t length);

  // XML mime
  static const char* mime;

 private:
  void encode(string_piece str);

  std::vector<char> xml;
  std::vector<std::string> stack;
  size_t stack_length;
  bool in_element = false;
  bool compacting = false;
};

// Definitions
xml_builder& xml_builder::clear() {
  xml.clear();
  stack.clear();
  stack_length = 0;
  in_element = false;
  return *this;
}

xml_builder& xml_builder::element(string_piece name) {
  if (in_element) {
    xml.push_back('/');
    xml.push_back('>');
    in_element = false;
    if (!compacting) {
      xml.push_back('\n');
      xml.insert(xml.end(), stack_length, ' ');
    }
  }

  xml.push_back('<');
  xml.insert(xml.end(), name.str, name.str + name.len);
  in_element = true;

  if (stack_length >= stack.size())
    stack.emplace_back(name.str, name.len);
  else
    stack[stack_length].assign(name.str, name.len);
  stack_length++;

  return *this;
}

xml_builder& xml_builder::attribute(string_piece name, string_piece value) {
  if (in_element) {
    xml.push_back(' ');
    xml.insert(xml.end(), name.str, name.str + name.len);
    xml.push_back('=');
    xml.push_back('"');
    encode(value);
    xml.push_back('"');
  }
  return *this;
}

xml_builder& xml_builder::text(string_piece str) {
  if (in_element) {
    xml.push_back('/');
    xml.push_back('>');
    in_element = false;
    if (!compacting) {
      xml.push_back('\n');
      xml.insert(xml.end(), stack_length, ' ');
    }
  }
  encode(str);
  return *this;
}

xml_builder& xml_builder::close() {
  if (stack_length) {
    stack_length--;
    if (in_element) {
      xml.push_back('/');
      xml.push_back('>');
    } else {
      if (!compacting) xml.insert(xml.end(), stack_length, ' ');
      xml.push_back('<');
      xml.push_back('/');
      xml.insert(xml.end(), stack[stack_length].begin(), stack[stack_length].end());
      xml.push_back('>');
    }
    if (!compacting) xml.push_back('\n');
  }
  return *this;
}

xml_builder& xml_builder::compact(bool compact) {
  compacting = compact;
  if (!compacting && !xml.empty() && xml.back() != '\n') xml.push_back('\n');
  return *this;
}

xml_builder& xml_builder::append_raw(string_piece data) {
  xml.insert(xml.end(), data.str, data.str + data.len);
  return *this;
}

xml_builder& xml_builder::close_all() {
  while (stack_length) close();
  return *this;
}

string_piece xml_builder::current() const {
  return string_piece(xml.data(), xml.size());
}

} // namespace microrestd
} // namespace ufal

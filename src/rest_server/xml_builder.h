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
  inline xml_builder& indent();

  // Close all open objects and arrays
  inline xml_builder& close_all(bool indent_before_close = false);

  // Return current xml
  inline string_piece current() const;

  // Remove current xml prefix; for response_generator
  void discard_current_prefix(size_t length);

  // XML mime
  static const char* mime;

 private:
  enum mode_t { NORMAL, IN_ELEMENT, NEED_INDENT };

  inline void normalize_mode();
  void encode(string_piece str);

  std::vector<char> xml;
  std::vector<std::string> stack;
  size_t stack_length = 0;
  mode_t mode;
};

// Definitions
xml_builder& xml_builder::clear() {
  xml.clear();
  stack.clear();
  stack_length = 0;
  mode = NORMAL;
  return *this;
}

xml_builder& xml_builder::element(string_piece name) {
  if (mode == NEED_INDENT) {
    if (stack_length) {
      xml.push_back('\n');
      xml.insert(xml.end(), stack_length, ' ');
    }
    mode = NORMAL;
  }
  if (mode == IN_ELEMENT) {
    xml.push_back('>');
    mode = NORMAL;
  }

  xml.push_back('<');
  xml.insert(xml.end(), name.str, name.str + name.len);
  mode = IN_ELEMENT;

  if (stack_length < stack.size())
    stack[stack_length].assign(name.str, name.len);
  else
    stack.emplace_back(name.str, name.len);
  stack_length++;

  return *this;
}

xml_builder& xml_builder::attribute(string_piece name, string_piece value) {
  if (mode == IN_ELEMENT) {
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
  if (mode == IN_ELEMENT) {
    xml.push_back('>');
    mode = NORMAL;
  }
  if (mode == NEED_INDENT) {
    if (stack_length) {
      xml.push_back('\n');
      xml.insert(xml.end(), stack_length, ' ');
    }
    mode = NORMAL;
  }
  encode(str);
  return *this;
}

xml_builder& xml_builder::close() {
  if (stack_length) {
    stack_length--;
    if (mode == NEED_INDENT) {
    }
    if (mode == IN_ELEMENT)
    if (in_element) {
      xml.push_back('/');
      xml.push_back('>');
      in_element = false;
    } else {
      xml.push_back('<');
      xml.push_back('/');
      xml.insert(xml.end(), stack[stack_length].begin(), stack[stack_length].end());
      xml.push_back('>');
    }
  }
  return *this;
}

xml_builder& xml_builder::indent() {
  if (in_element) {
    xml.push_back('>');
    in_element = false;
  }
  need_indent = true;
  return *this;
}

xml_builder& xml_builder::close_all(bool indent_before_close) {
  while (stack_length) {
    if (indent_before_close) indent();
    close();
  }
  return *this;
}

string_piece xml_builder::current() const {
  return string_piece(xml.data(), xml.size());
}

void normalize_mode();

} // namespace microrestd
} // namespace ufal

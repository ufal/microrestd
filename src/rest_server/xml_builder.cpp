// This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
//
// Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstring>

#include "xml_builder.h"

namespace ufal {
namespace microrestd {

const char* xml_builder::mime = "application/xml";

void xml_builder::discard_current_prefix(size_t length) {
  if (!length) return;

  size_t xml_size = xml.size();
  if (length < xml_size) {
    memmove(xml.data(), xml.data() + length, xml_size - length);
    xml.resize(xml_size - length);
  } else {
    xml.clear();
  }
}

void xml_builder::encode(string_piece str) {
  for (; str.len; str.str++, str.len--)
    switch (*str.str) {
      case '&': xml.push_back('&'); xml.push_back('a'); xml.push_back('p'); xml.push_back('o'); xml.push_back('s'); xml.push_back(';'); break;
      case '<': xml.push_back('&'); xml.push_back('l'); xml.push_back('t'); xml.push_back(';'); break;
      case '>': xml.push_back('&'); xml.push_back('g'); xml.push_back('t'); xml.push_back(';'); break;
      case '"': xml.push_back('&'); xml.push_back('q'); xml.push_back('u'); xml.push_back('o'); xml.push_back('t'); xml.push_back(';'); break;
      default: xml.push_back(*str.str);
    }
}


} // namespace microrestd
} // namespace ufal

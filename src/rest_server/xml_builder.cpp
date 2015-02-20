// This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
//
// Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "xml_builder.h"

namespace ufal {
namespace microrestd {

const char* xml_builder::mime = "application/xml";

xml_builder& xml_builder::declaration(const char* encoding, const char* standalone) {
  static const string_piece declaration = "<?xml version=\"1.0\"";
  static const string_piece declaration_encoding = " encoding=\"";
  static const string_piece declaration_standalone = " standalone=\"";

  xml.insert(xml.end(), declaration.str, declaration.str + declaration.len);
  if (encoding) {
    xml.insert(xml.end(), declaration_encoding.str, declaration_encoding.str + declaration_encoding.len);
    xml.insert(xml.end(), encoding, encoding + strlen(encoding));
    xml.push_back('"');
  }
  if (standalone) {
    xml.insert(xml.end(), declaration_standalone.str, declaration_standalone.str + declaration_standalone.len);
    xml.insert(xml.end(), standalone, standalone + strlen(standalone));
    xml.push_back('"');
  }
  xml.push_back('?');
  xml.push_back('>');
  return *this;
}

void xml_builder::discard_current_prefix(size_t length) {
  if (!length) return;

  if (length < xml.size())
    xml.erase(xml.begin(), xml.begin() + length);
  else
    xml.clear();
  if (!length) return;
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

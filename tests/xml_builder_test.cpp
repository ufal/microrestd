// This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
//
// Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <iostream>

#include "microrestd.h"

using namespace std;
using namespace ufal::microrestd;

int main(void) {
  xml_builder xml;

  xml.declaration();
  xml.indent().element("root");
  xml.indent().text("less-than:<");
  xml.indent().element("element").attribute("attr", "value").attribute("quot", "\"");
  xml.indent().text("greater-than:>");
  xml.indent().close();
  xml.indent().text("amp:&");
  xml.indent().element("element").close();
  xml.finish(true);

  cout << xml.current();

  return 0;
}

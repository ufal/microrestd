// This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
//
// Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cstdio>

#include "microrestd.h"

using namespace ufal::microrestd;

int main(void) {
  xml_builder xml;

  xml.element("nazdar");
  xml.text("text-nazdar");
  xml.element("test");
  xml.text("text-test");
  xml.close();
  xml.text("text-nazdar");
  xml.close_all();

  auto data = xml.current();
  printf("%*s", int(data.len), data.str);

  return 0;
}

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
  json_builder json;

  json.object();
  json.key("ahoj").value("nazdar").value(" appended", true);
  json.key("array").array().value("h1").value("h2").value("\"quot\"").close();
  json.key("object").object().key("a").value("A").key("b").value("B").close();
  json.close();

  auto data = json.current();
  printf("%*s\n", int(data.len), data.str);

  json.clear();
  json.object();
  json.indent().key("ahoj").indent().value("nazdar").value(" appended", true);
  json.indent().key("array").indent().array().indent().value("h1").indent().value("h2").indent().value("\"quot\"").indent().close();
  json.indent().key("object").indent().object().indent().key("a").value("A").indent().key("b").value("B").indent().close();
  json.indent().close();

  data = json.current();
  printf("%*s\n", int(data.len), data.str);

  return 0;
}

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
  json_builder json;

  json.object();
  json.key("ahoj").value("nazdar").value(" appended", true);
  json.key("array").array().value("h1").value("h2").value("\"quot\"").value(0).value(-42).value_bool(true).value_bool(false).value(nullptr).close();
  json.key("object").object().key("a").value("A").key("b").value("B").close();
  json.finish();

  cout << json.current();

  json.clear();
  json.object();
  json.indent().key("ahoj").indent().value("nazdar").value(" appended", true);
  json.indent().key("array").indent().array().indent().value("h1").indent().value("h2").indent().value("\"quot\"").indent().close();
  json.indent().key("object").indent().object().indent().key("a").value("A").indent().key("b").value("B").indent().close();
  json.finish(true);

  cout << json.current();

  return 0;
}

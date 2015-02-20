// This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
//
// Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <chrono>
#include <cstdlib>
#include <string>
#include <thread>

#include "microrestd.h"

using namespace std;
using namespace ufal::microrestd;

class file_service : public rest_service {
  class file_generator : public response_generator {
   public:
    file_generator(FILE* f) : f(f) {}
    ~file_generator() { if (f) fclose(f); }

    virtual bool generate() override {
      size_t data_size = data.size();
      data.resize(data_size + 1024);
      size_t read = fread(data.data() + data_size, 1, 1024, f);
      data.resize(data_size + read);

      // Now sleep for 2 seconds to simulate hard work :-)
      this_thread::sleep_for(chrono::seconds(2));

      return read;
    }
    virtual string_piece current() const override {
      return string_piece(data.data(), data.size());
    }
    virtual void consume(size_t length) override {
      if (length >= data.size()) data.clear();
      else if (length) data.erase(data.begin(), data.begin() + length);
    }

   private:
    FILE* f;
    vector<char> data;
  };

 public:
  virtual bool handle(rest_request& req) override {
    fprintf(stderr, "Serving url %s via method %s\n", req.url.c_str(), req.method.c_str());

    if (req.method != "HEAD" && req.method != "GET") return req.respond_method_not_allowed("HEAD, GET");

    if (!req.url.empty()) {
      FILE* f = fopen(req.url.c_str() + 1, "rb");
      if (f) return req.respond("application/octet-stream", new file_generator(f));
    }

    return req.respond_not_found();
  }
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s port [threads] [connection_limit]\n", argv[0]);
    return 1;
  }
  int port = stoi(argv[1]);
  int threads = argc >= 3 ? stoi(argv[2]) : 0;
  int connection_limit = argc >= 4 ? stoi(argv[3]) : 2;

  rest_server server;
  server.set_max_connections(connection_limit);
  server.set_threads(threads);
  if (!server.start(new file_service(), port)) {
    fprintf(stderr, "Cannot start REST server!\n");
    return 1;
  }
  server.wait_until_closed();

  return 0;
}

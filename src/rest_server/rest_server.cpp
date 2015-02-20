// This file is part of MicroRestD <http://github.com/ufal/microrestd/>.
//
// Copyright 2015 Institute of Formal and Applied Linguistics, Faculty of
// Mathematics and Physics, Charles University in Prague, Czech Republic.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>

#if defined(_WIN32) && !defined(__CYGWIN__)
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#endif

#include "response_generator.h"
#include "rest_server.h"
#include "../libmicrohttpd/microhttpd.h"

namespace ufal {
namespace microrestd {

using namespace libmicrohttpd;
using namespace std;

class MHD_ResponseDeleter {
 public:
  void operator()(MHD_Response* response) {
    MHD_destroy_response(response);
  }
};

class MHD_PostProcessorDeleter {
 public:
  void operator()(MHD_PostProcessor* post_processor) {
    MHD_destroy_post_processor(post_processor);
  }
};

// Class rest_server::microhttpd_request
class rest_server::microhttpd_request : public rest_request {
 public:
  static bool initialize();

  microhttpd_request(const rest_server& server, MHD_Connection* connection, const char* url, const char* content_type, const char* method);

  int handle(rest_service* service);
  bool process_post_data(const char* post_data, size_t post_data_len);

  const sockaddr* address() const;
  const char* forwarded_for() const;

  virtual bool respond(const char* content_type, string_piece body, bool make_copy = true) override;
  virtual bool respond(const char* content_type, response_generator* generator) override;
  virtual bool respond_not_found() override;
  virtual bool respond_method_not_allowed(const char* comma_separated_allowed_methods) override;
  virtual bool respond_error(string_piece error, int code = 400) override;

 private:
  const rest_server& server;
  MHD_Connection* connection;

  unique_ptr<MHD_PostProcessor, MHD_PostProcessorDeleter> post_processor;
  bool need_post_processor;
  bool unsupported_post_data;
  unsigned remaining_post_limit;

  unique_ptr<response_generator> generator;
  bool generator_end;
  unsigned generator_offset;

  static MHD_Response* create_response(string_piece data, const char* content_type, bool make_copy);
  static MHD_Response* create_generator_response(microhttpd_request* request, const char* content_type);
  static MHD_Response* create_plain_permanent_response(const string& data);
  static void response_common_headers(unique_ptr<MHD_Response, MHD_ResponseDeleter>& response, const char* content_type);

  static int get_iterator(void* cls, MHD_ValueKind kind, const char* key, const char* value);
  static int post_iterator(void* cls, MHD_ValueKind kind, const char* key, const char* filename, const char* content_type, const char* transfer_encoding, const char* data, uint64_t off, size_t size);
  static ssize_t generator_callback(void* cls, uint64_t pos, char* buf, size_t max);

  static bool valid_utf8(const string& text);

  static bool benevolent_compare(const char* string, const char* pattern);
  static bool supported_content_type(const char* content_type);
  static bool supported_transfer_encoding(const char* transfer_encoding);

  static unique_ptr<MHD_Response, MHD_ResponseDeleter> response_not_allowed, response_not_found, response_too_large, response_unsupported_post_data, response_invalid_utf8;
};
unique_ptr<MHD_Response, MHD_ResponseDeleter> rest_server::microhttpd_request::response_not_allowed,
                                              rest_server::microhttpd_request::response_not_found,
                                              rest_server::microhttpd_request::response_too_large,
                                              rest_server::microhttpd_request::response_unsupported_post_data,
                                              rest_server::microhttpd_request::response_invalid_utf8;

rest_server::microhttpd_request::microhttpd_request(const rest_server& server, MHD_Connection* connection, const char* url, const char* content_type, const char* method)
  : server(server), connection(connection), unsupported_post_data(false), remaining_post_limit(server.max_post_size + 1) {
  // Initialize rest_request fields
  this->url = url;
  this->method = method;
  this->content_type = content_type;

  // Create post processor if needed
  need_post_processor = this->method == MHD_HTTP_METHOD_POST &&
      (benevolent_compare(content_type, MHD_HTTP_POST_ENCODING_FORM_URLENCODED) ||
       benevolent_compare(content_type, MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA));
  if (need_post_processor) {
    post_processor.reset(MHD_create_post_processor(connection, 32 << 10, &post_iterator, this));
    if (!post_processor) fprintf(stderr, "Cannot allocate new post processor!\n");
  }

  // Collect GET arguments
  MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, get_iterator, this);
}

bool rest_server::microhttpd_request::initialize() {
  static string not_allowed = "Requested method is not allowed.\n";
  static string not_found = "Requested URL was not found.\n";
  static string too_large = "Request was too large.\n";
  static string unsupported_post_data = "Unsupported format of the multipart/form-data POST request. Currently only the following is supported:\n"
      " - Content-type: application/octet-stream or text/plain or text/plain; charset=utf-8\n"
      " - Content-transfer-encoding: 7bit or 8bit or binary\n";
  static string invalid_utf8 = "The request arguments are not valid UTF-8.\n";

  response_not_allowed.reset(create_plain_permanent_response(not_allowed));
  if (!response_not_allowed) return false;
  if (MHD_add_response_header(response_not_allowed.get(), MHD_HTTP_HEADER_ALLOW, "HEAD, GET, POST, PUT, DELETE") != MHD_YES) return false;

  response_not_found.reset(create_plain_permanent_response(not_found));
  if (!response_not_found) return false;

  response_too_large.reset(create_plain_permanent_response(too_large));
  if (!response_too_large) return false;

  response_unsupported_post_data.reset(create_plain_permanent_response(unsupported_post_data));
  if (!response_unsupported_post_data) return false;

  response_invalid_utf8.reset(create_plain_permanent_response(invalid_utf8));
  if (!response_invalid_utf8) return false;

  return true;
}

int rest_server::microhttpd_request::handle(rest_service* service) {
  // Check that method is supported
  if (method != MHD_HTTP_METHOD_HEAD && method != MHD_HTTP_METHOD_GET && method != MHD_HTTP_METHOD_POST && method != MHD_HTTP_METHOD_PUT && method != MHD_HTTP_METHOD_DELETE)
    return MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response_not_allowed.get());

  // Close post_processor if exists
  if (post_processor) post_processor.reset();

  // Was the POST format supported
  if (unsupported_post_data)
    return MHD_queue_response(connection, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE, response_unsupported_post_data.get());

  // Was the request too large?
  if (!remaining_post_limit)
    return MHD_queue_response(connection, MHD_HTTP_REQUEST_ENTITY_TOO_LARGE, response_too_large.get());

  // Are all arguments legal utf-8?
  for (auto&& param : params)
    if (!valid_utf8(param.first) || !valid_utf8(param.second))
      return MHD_queue_response(connection, MHD_HTTP_UNSUPPORTED_MEDIA_TYPE, response_invalid_utf8.get());

  // Let the service handle the request and respond with one of the respond_* methods.
  return service->handle(*this) ? MHD_YES : MHD_NO;
}

bool rest_server::microhttpd_request::process_post_data(const char* post_data, size_t post_data_len) {
  if (need_post_processor)
    return post_processor && MHD_post_process(post_processor.get(), post_data, post_data_len) == MHD_YES;
  else
    return body.append(post_data, post_data_len), true;
}

const sockaddr* rest_server::microhttpd_request::address() const {
  auto info = MHD_get_connection_info(connection, MHD_CONNECTION_INFO_CLIENT_ADDRESS);
  return info ? info->client_addr : nullptr;
}

const char* rest_server::microhttpd_request::forwarded_for() const {
  return MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "X-Forwarded-For");
}

bool rest_server::microhttpd_request::respond(const char* content_type, string_piece body, bool make_copy) {
  unique_ptr<MHD_Response, MHD_ResponseDeleter> response(create_response(body, content_type, make_copy));
  if (!response) return false;
  return MHD_queue_response(connection, MHD_HTTP_OK, response.get()) == MHD_YES;
}

bool rest_server::microhttpd_request::respond(const char* content_type, response_generator* generator) {
  this->generator.reset(generator);
  this->generator_end = false;
  this->generator_offset = 0;
  unique_ptr<MHD_Response, MHD_ResponseDeleter> response(create_generator_response(this, content_type));
  if (!response) return false;
  return MHD_queue_response(connection, MHD_HTTP_OK, response.get()) == MHD_YES;
}

bool rest_server::microhttpd_request::respond_not_found() {
  return MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response_not_found.get()) == MHD_YES;
}

bool rest_server::microhttpd_request::respond_method_not_allowed(const char* comma_separated_allowed_methods) {
  unique_ptr<MHD_Response, MHD_ResponseDeleter> response(create_response("Requested method is not allowed.\n", "text/plain", false));
  if (!response) return false;
  if (MHD_add_response_header(response.get(), MHD_HTTP_HEADER_ALLOW, comma_separated_allowed_methods) != MHD_YES) return response.reset(), false;
  return MHD_queue_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED, response.get()) == MHD_YES;
}

bool rest_server::microhttpd_request::respond_error(string_piece error, int code) {
  unique_ptr<MHD_Response, MHD_ResponseDeleter> response(create_response(error, "text/plain", true));
  if (!response) return false;
  return MHD_queue_response(connection, code, response.get()) == MHD_YES;
}

MHD_Response* rest_server::microhttpd_request::create_plain_permanent_response(const string& data) {
  unique_ptr<MHD_Response, MHD_ResponseDeleter> response(MHD_create_response_from_buffer(data.size(), (void*) data.c_str(), MHD_RESPMEM_PERSISTENT));
  response_common_headers(response, "text/plain");
  return response.release();
}

MHD_Response* rest_server::microhttpd_request::create_response(string_piece data, const char* content_type, bool make_copy) {
  unique_ptr<MHD_Response, MHD_ResponseDeleter> response(MHD_create_response_from_buffer(data.len, (void*) data.str, make_copy ? MHD_RESPMEM_MUST_COPY : MHD_RESPMEM_PERSISTENT));
  response_common_headers(response, content_type);
  return response.release();
}

MHD_Response* rest_server::microhttpd_request::create_generator_response(microhttpd_request* request, const char* content_type) {
  unique_ptr<MHD_Response, MHD_ResponseDeleter> response(MHD_create_response_from_callback(-1, 32 << 10, generator_callback, request, nullptr));
  response_common_headers(response, content_type);
  return response.release();
}

void rest_server::microhttpd_request::response_common_headers(unique_ptr<MHD_Response, MHD_ResponseDeleter>& response, const char* content_type) {
  if (!response) return;
  if (MHD_add_response_header(response.get(), MHD_HTTP_HEADER_CONTENT_TYPE, content_type) != MHD_YES ||
      MHD_add_response_header(response.get(), MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, "*") != MHD_YES ||
      MHD_add_response_header(response.get(), MHD_HTTP_HEADER_CONNECTION, "close") != MHD_YES)
    response.reset();
}


int rest_server::microhttpd_request::get_iterator(void* cls, MHD_ValueKind kind, const char* key, const char* value) {
  auto self = (microhttpd_request*) cls;
  if (kind == MHD_GET_ARGUMENT_KIND && self->remaining_post_limit) {
    auto value_len = value ? strlen(value) : 0;
    if (self->remaining_post_limit > value_len) {
      if (self->params.emplace(key, value ? value : string()).second)
        self->remaining_post_limit -= value_len;
    } else {
      self->remaining_post_limit = 0;
    }
  }
  return MHD_YES;
}

int rest_server::microhttpd_request::post_iterator(void* cls, MHD_ValueKind kind, const char* key, const char* /*filename*/, const char* content_type, const char* transfer_encoding, const char* data, uint64_t off, size_t size) {
  auto self = (microhttpd_request*) cls;
  if (kind == MHD_POSTDATA_KIND && self->remaining_post_limit) {
    // Check that content_type and transfer_encoding are supported
    if ((content_type && !supported_content_type(content_type)) ||
        (transfer_encoding && !supported_transfer_encoding(transfer_encoding))) {
      self->unsupported_post_data = true;
      self->remaining_post_limit = 0;
    }

    if (self->remaining_post_limit > size) {
      string& value = self->params[key];
      if (!off) value.clear();
      if (value.size() == off) {
        if (size) value.append(data, size);
        self->remaining_post_limit -= size;
      } else {
        fprintf(stderr, "Cannot append to key %s at offset %u, have only %u\n", key, unsigned(off), unsigned(value.size()));
      }
    } else {
      self->remaining_post_limit = 0;
    }
  }

  return MHD_YES;
}

ssize_t rest_server::microhttpd_request::generator_callback(void* cls, uint64_t /*pos*/, char* buf, size_t max) {
  auto request = (microhttpd_request*) cls;
  string_piece data = request->generator->current();
  while (data.len - request->generator_offset < request->server.min_generated && !request->generator_end) {
    request->generator_end = !request->generator->generate();
    data = request->generator->current();
  }

  // End of data?
  if (data.len <= request->generator_offset) return MHD_CONTENT_READER_END_OF_STREAM;

  // Copy generated data and remove them from the generator
  size_t data_len = min(data.len - request->generator_offset, max);
  memcpy(buf, data.str + request->generator_offset, data_len);
  request->generator_offset += data_len;
  if (data.len - request->generator_offset < request->server.min_generated) {
    request->generator->consume(request->generator_offset);
    request->generator_offset = 0;
  }
  return data_len;
}

bool rest_server::microhttpd_request::valid_utf8(const string& text) {
  for (auto str = (const unsigned char*) text.c_str(); *str; str++)
    if (*str >= 0x80) {
      if (*str < 0xC0) return false;
      else if (*str < 0xE0) {
        str++; if (*str < 0x80 || *str >= 0xC0) return false;
      } else if (*str < 0xF0) {
        str++; if (*str < 0x80 || *str >= 0xC0) return false;
        str++; if (*str < 0x80 || *str >= 0xC0) return false;
      } else if (*str < 0xF8) {
        str++; if (*str < 0x80 || *str >= 0xC0) return false;
        str++; if (*str < 0x80 || *str >= 0xC0) return false;
        str++; if (*str < 0x80 || *str >= 0xC0) return false;
      } else return false;
    }

  return true;
}

bool rest_server::microhttpd_request::benevolent_compare(const char* string, const char* pattern) {
  // While there are pattern characters.
  while (*pattern) {
    // Skip spaces.
    while (*string && isspace(*string)) string++;
    if (!*string) return false;

    // Match the next character ignoring case.
    if (tolower(*string++) != tolower(*pattern++)) return false;
  }
  // Skip final spaces.
  while (*string && isspace(*string)) string++;

  // Succeed only if there are no characters in string left.
  return !*string;
}

bool rest_server::microhttpd_request::supported_content_type(const char* content_type) {
  return benevolent_compare(content_type, "application/octet-stream") ||
         benevolent_compare(content_type, "text/plain") ||
         benevolent_compare(content_type, "text/plain;charset=utf-8");
}

bool rest_server::microhttpd_request::supported_transfer_encoding(const char* transfer_encoding) {
  return benevolent_compare(transfer_encoding, "binary") ||
         benevolent_compare(transfer_encoding, "7bit") ||
         benevolent_compare(transfer_encoding, "8bit");
  return true;
}


// Class rest_server
bool rest_server::set_log_file(const std::string& file_name, unsigned max_log_size) {
  lock_guard<decltype(log_file_mutex)> log_file_lock(log_file_mutex);

  if (log_file) fclose(log_file);
  log_file = fopen(file_name.c_str(), "a");
  if (log_file) {
    setvbuf(log_file, NULL, _IOLBF, 0);
    this->max_log_size = max_log_size;
  }

  return log_file;
}

void rest_server::set_min_generated(unsigned min_generated) { this->min_generated = min_generated; }
void rest_server::set_max_connections(unsigned max_connections) { this->max_connections = max_connections; }
void rest_server::set_max_post_size(unsigned max_post_size) { this->max_post_size = max_post_size; }
void rest_server::set_threads(unsigned threads) { this->threads = threads; }
void rest_server::set_timeout(unsigned timeout) { this->timeout = timeout; }

bool rest_server::start(rest_service* service, unsigned port) {
  if (!service) return false;
  this->service = service;

  if (!microhttpd_request::initialize()) return false;

#if !defined(_WIN32) || defined(__CYGWIN__)
  sigset_t set;
  if (sigemptyset(&set) != 0) return false;
  if (sigaddset(&set, SIGUSR1) != 0) return false;
  if (sigprocmask(SIG_BLOCK, &set, nullptr) != 0) return false;
#endif

  for (int use_poll = 1; use_poll >= 0; use_poll--) {
    MHD_OptionItem threadpool_size[] = {
      { threads ? MHD_OPTION_THREAD_POOL_SIZE : MHD_OPTION_END, int(threads), nullptr },
      { MHD_OPTION_END, 0, nullptr }
    };
    MHD_OptionItem connection_limit[] = {
      { max_connections ? MHD_OPTION_CONNECTION_LIMIT : MHD_OPTION_END, int(max_connections), nullptr },
      { MHD_OPTION_END, 0, nullptr }
    };

    daemon = MHD_start_daemon((threads ? MHD_USE_SELECT_INTERNALLY : MHD_USE_THREAD_PER_CONNECTION) | (use_poll ? MHD_USE_POLL : 0) | MHD_USE_PIPE_FOR_SHUTDOWN,
                              port, nullptr, nullptr, &handle_request, this,
                              MHD_OPTION_LISTENING_ADDRESS_REUSE, 1,
                              MHD_OPTION_ARRAY, threadpool_size,
                              MHD_OPTION_ARRAY, connection_limit,
                              MHD_OPTION_CONNECTION_MEMORY_LIMIT, size_t(64 << 10),
                              MHD_OPTION_CONNECTION_TIMEOUT, timeout,
                              MHD_OPTION_NOTIFY_COMPLETED, &request_completed, this,
                              MHD_OPTION_END);

    if (daemon) {
      logf("Starting service, port %u, max connections %u, timeout %u, max post size %u, min generated %u.", port, max_connections, timeout, max_post_size, min_generated);
      return true;
    }
  }

  return false;
}

void rest_server::wait_until_closed() {
#if defined(_WIN32) && !defined(__CYGWIN__)
  logf("Server started.");
  wait_indefinitely();
#else
  logf("Server started, serving until USR1 is received.");

  sigset_t set;
  if (sigemptyset(&set) != 0) wait_indefinitely();
  if (sigaddset(&set, SIGUSR1) != 0) wait_indefinitely();

  // Wait for SIGUSR1
  int signal;
  do {
    if (sigwait(&set, &signal) != 0) wait_indefinitely();
  } while (signal != SIGUSR1);

  // Should stop now. Quiesce the daemon and wait for current requests to be
  // handled.
  logf("USR1 received, closing listening port and waiting for current requests to finish.");
  MHD_socket socket = *(MHD_socket*)MHD_get_daemon_info(daemon, MHD_DAEMON_INFO_LISTEN_FD);
  if (socket != MHD_INVALID_SOCKET) shutdown(socket, SHUT_RDWR);
  MHD_quiesce_daemon(daemon);
  while (true) {
    unsigned connections = *(unsigned*)MHD_get_daemon_info(daemon, MHD_DAEMON_INFO_CURRENT_CONNECTIONS);
    logf("There are %u current connections.", connections);
    if (!connections) break;
    sleep(1);
  }
  logf("Stopping now.");

  if (log_file) fclose(log_file);
  log_file = nullptr;

  MHD_stop_daemon(daemon);
  daemon = nullptr;
  service = nullptr;

  if (socket != MHD_INVALID_SOCKET) close(socket);
#endif
}

void rest_server::wait_indefinitely() {
#if defined(_WIN32) && !defined(__CYGWIN__)
  logf("Waiting indefinitely.");
  Sleep(INFINITE);
#else
  logf("An error occurred in waiting for USR1, waiting indefinitely.");
  while(true)
    pause();
#endif
}

int rest_server::handle_request(void* cls, struct MHD_Connection* connection, const char* url, const char* method, const char* /*version*/, const char* upload_data, size_t* upload_data_size, void** con_cls) {
  auto self = (rest_server*) cls;
  auto request = (microhttpd_request*) *con_cls;

  // Do we have a new request?
  if (!request) {
    const char* content_type = MHD_lookup_connection_value (connection, MHD_HEADER_KIND, MHD_HTTP_HEADER_CONTENT_TYPE);
    if (!content_type) content_type = "";

    if (!(request = new microhttpd_request(*self, connection, url, content_type, method)))
      return fprintf(stderr, "Cannot allocate new request!\n"), MHD_NO;

    *con_cls = request;
    return MHD_YES;
  }

  // Are we processing POST data?
  if (*upload_data_size) {
    if (!request->process_post_data(upload_data, *upload_data_size))
      return MHD_NO;

    *upload_data_size = 0;
    return MHD_YES;
  }

  // Log complete request
  self->log_request(request);

  // Handle complete request
  return request->handle(self->service) ? MHD_YES : MHD_NO;
}

void rest_server::request_completed(void* /*cls*/, struct MHD_Connection* /*connection*/, void** con_cls, int /*toe*/) {
  auto request = (const microhttpd_request*) *con_cls;
  if (request) delete request;
}

void rest_server::logf(const char* message, ...) {
  if (!log_file) return;

  // Prepare timestamp
  char timestamp[32];
  time_t time_now;
  tm tm_now;
  time_now = time(nullptr);
#ifdef _MSC_VER
  localtime_s(&tm_now, &time_now);
#else
  localtime_r(&time_now, &tm_now);
#endif
  size_t len = strftime(timestamp, sizeof(timestamp), "%a %d. %b %Y %H:%M:%S\t", &tm_now);

  // Locked using the log_file_mutex
  {
    lock_guard<decltype(log_file_mutex)> log_file_lock(log_file_mutex);

    if (len) fwrite(timestamp, 1, len, log_file);

    va_list ap;
    va_start(ap, message);
    vfprintf(log_file, message, ap);
    va_end(ap);

    fputc('\n', log_file);
  }
}

void rest_server::log_append_pair(string& message, const char* key, const string& value) {
  size_t to_clean = message.size();

  message.append(key);
  message.append(to_string(value.size()));
  message.push_back(':');

  if (!max_log_size || value.size() < max_log_size) {
    message.append(value);
  } else {
    struct utf8_helper {
      static bool valid_start(char chr) { return uint8_t(chr) < uint8_t(0x80) || uint8_t(chr) >= uint8_t(0xE0); }
    };

    size_t utf8_border = max_log_size >> 1;
    while (utf8_border && !utf8_helper::valid_start(value[utf8_border])) utf8_border--;
    message.append(value, 0, utf8_border);

    message.append(" ... ");

    utf8_border = value.size() - (max_log_size >> 1);
    while (utf8_border < value.size() && !utf8_helper::valid_start(value[utf8_border])) utf8_border++;
    message.append(value, utf8_border, value.size() - utf8_border);
  }

  // Map the \t, \r, \n in the appended message.
  for (; to_clean < message.size(); to_clean++)
    switch (message[to_clean]) {
      case '\t':
      case '\r': message[to_clean] = ' '; break;
      case '\n': message[to_clean] = '\r'; break;
    }
}

void rest_server::log_request(const microhttpd_request* request) {
  if (!log_file) return;

  auto sock_addr = request->address();
  char address[64] = "";
  if (sock_addr && sock_addr->sa_family == AF_INET) getnameinfo(sock_addr, sizeof(sockaddr_in), address, sizeof(address), nullptr, 0, NI_NUMERICHOST);
  if (sock_addr && sock_addr->sa_family == AF_INET6) getnameinfo(sock_addr, sizeof(sockaddr_in6), address, sizeof(address), nullptr, 0, NI_NUMERICHOST);

  auto forwarded_for = request->forwarded_for();

  string data;
  log_append_pair(data, "body", request->body);
  for (auto&& param : request->params) {
    data.push_back('\t');
    log_append_pair(data, param.first.c_str(), param.second);
  }

  logf("Request\t%s\t%s\t%s\t%s", address, forwarded_for, request->url.c_str(), data.c_str());
}

} // namespace microrestd
} // namespace ufal

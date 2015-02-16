// Modified by Milan Straka <straka@ufal.mff.cuni.cz> for microrestd.

/*
     This file is part of libmicrohttpd
     (C) 2007 Christian Grothoff (and other contributing authors)

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @file fileserver_example.c
 * @brief minimal example for how to use libmicrohttpd to serve files
 * @author Christian Grothoff
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "libmicrohttpd/microhttpd.h"

namespace ufal {
namespace microrestd {
namespace libmicrohttpd {

#define PAGE "<html><head><title>File not found</title></head><body>File not found</body></html>"

static ssize_t
file_reader (void *cls, uint64_t pos, char *buf, size_t max)
{
  FILE* file = (FILE*) cls;

  (void)  fseek (file, pos, SEEK_SET);
  return fread (buf, 1, max, file);
}

static void
free_callback (void *cls)
{
  FILE* file = (FILE*) cls;
  fclose (file);
}

static int
ahc_echo (void * /*cls*/,
          struct MHD_Connection *connection,
          const char *url,
          const char *method,
          const char * /*version*/,
          const char * /*upload_data*/,
	  size_t * /*upload_data_size*/, void **ptr)
{
  static int aptr;
  struct MHD_Response *response;
  int ret;
  FILE *file;

  if (0 != strcmp (method, MHD_HTTP_METHOD_GET))
    return MHD_NO;              /* unexpected method */
  if (&aptr != *ptr)
    {
      /* do never respond on first call */
      *ptr = &aptr;
      return MHD_YES;
    }
  *ptr = NULL;                  /* reset when done */
  file = fopen (&url[1], "rb");
  if (file == NULL)
    {
      response = MHD_create_response_from_buffer (strlen (PAGE),
						  (void *) PAGE,
						  MHD_RESPMEM_PERSISTENT);
      ret = MHD_queue_response (connection, MHD_HTTP_NOT_FOUND, response);
      MHD_destroy_response (response);
    }
  else
    {
      fseek(file, 0, SEEK_END);
      long size = ftell(file);
      fseek(file, 0, SEEK_SET);

      response = MHD_create_response_from_callback (size, 32 * 1024,     /* 32k page size */
                                                    &file_reader,
                                                    file,
                                                    &free_callback);
      if (response == NULL)
	{
	  fclose (file);
	  return MHD_NO;
	}
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
      MHD_destroy_response (response);
    }
  return ret;
}

} // namespace libmicrohttpd
} // namespace microrestd
} // namespace ufal

using namespace ufal::microrestd::libmicrohttpd;

int
main (int argc, char *const *argv)
{
  struct MHD_Daemon *d;

  if (argc != 2)
    {
      printf ("%s PORT\n", argv[0]);
      return 1;
    }
  // Try using POLL first, then SELECT.
  for (int poll = 1; poll >= 0; poll--) {
    d = MHD_start_daemon (MHD_USE_THREAD_PER_CONNECTION | (poll ? MHD_USE_POLL : 0) | MHD_USE_PIPE_FOR_SHUTDOWN,
                          atoi (argv[1]), nullptr, nullptr, &ahc_echo, (void*) PAGE,
                          MHD_OPTION_LISTENING_ADDRESS_REUSE, 1,
                          MHD_OPTION_CONNECTION_LIMIT, 5,
                          MHD_OPTION_CONNECTION_MEMORY_LIMIT, size_t(64 * 1024),
                          MHD_OPTION_CONNECTION_TIMEOUT, 60,
                          MHD_OPTION_END);
    if (d) break;
  }
  if (!d) return 1;

  fprintf(stderr, "Running...\n");
  (void) getc (stdin);
  MHD_stop_daemon (d);
  return 0;
}

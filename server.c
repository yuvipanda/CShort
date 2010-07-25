#include <unistd.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <microhttpd.h>
#include <tdb.h>

#define PORT 8888


int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url,
			  const char *method, const char *version, const char *upload_data,
			  size_t *upload_data_size, void **con_cls)
{
  TDB_CONTEXT *db = (TDB_CONTEXT*) cls;
  const char *page;
  struct MHD_Response *response;
  int ret;

  // We assume that nginx won't pass us / itself.
  if (strcmp(url, "/")==0)
    {
      printf("\nSetup nginx properly!");
      return MHD_NO;
    }

  TDB_DATA key, data;
  key.dptr = strdup(url);
  key.dsize = strlen(url);

  response = MHD_create_response_from_data (strlen(page),
					    (void*) page, MHD_YES, MHD_NO);

  if (strcmp(method, "GET")==0)
    {
      TDB_DATA data;
      data = tdb_fetch (db, key);
      if (data.dptr)
	{
	  MHD_add_response_header (response, "Location", data.dptr);
	  ret = MHD_queue_response (connection, MHD_HTTP_MOVED_PERMANENTLY, response);
	}
      else
	{
	  ret = MHD_queue_response (connection, 404, response);
	}
    }
  else if (strcmp(method, "POST")==0)
    {
      printf ("%s -> %s", url, upload_data);

      data.dptr = strdup(upload_data);
      data.dsize = strlen(upload_data);
      tdb_store(db, key, data, TDB_REPLACE);
      ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
    }

  free(data.dptr);
  free(key.dptr);
    
  MHD_destroy_response(response);

  return ret;
}

int main (int argc, char** argv)
{
  struct MHD_Daemon *daemon;

  TDB_CONTEXT *db;
  db = tdb_open ("linksdb", 0, NULL, O_CREAT | O_RDWR, 0666);
  daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL,
			     &answer_to_connection, db, MHD_OPTION_END);
  if (daemon == NULL)
    return 1;

  getchar();

  MHD_stop_daemon (daemon);
  return 0;
}


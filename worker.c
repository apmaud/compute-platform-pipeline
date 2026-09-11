#include "pipeline.h"
#include <stdatomic.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

int main()
{
  const char *listen_path = WORKER_SOCK_PATH;
  const char *sink_path = "sink path";

  // receive a frame
  // how can we receive a frame? unix socket to receive connection 
 
  int listen_fd = unix_listen(listen_path); // listening for a connection from sensor
  if (listen_fd < 0)
  {
    fprintf(stderr, "[worker %d] could not listen on %s\n", getpid(), listen_path);
    return 1;
  }
  printf("[worker %d] is listening on %s\n", getpid(), listen_path);

  int client_fd = accept(listen_fd, NULL, NULL);// connection once sensor sim appears
  if (client_fd < 0)
  {
    perror("[worker] listener accept");
    return 1;
  }
  printf("[worker %d] sensor-sim connected, processing frames\n", getpid());

  uint64_t processed = 0;
  frame_t frame;

  for(;;)
  {
    ssize_t r = read_full(client_fd, const void *buf, sizeof(frame));

  }


  // process it 
  // what does it mean to process? should just wait, act lik eim processing or actually perform something
  


  // forward it 
  // connect to sink socket
  int sink_fd = unix_connect_retry(sink_path, 20, 100000);






  
  return 0;
}

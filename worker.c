#include <stdatomic.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>

#include "pipeline.h"

int main()
{
  const char *listen_path = WORKER_SOCK_PATH;
  const char *sink_path = SINK_SOCK_PATH;
  useconds_t processing_delay_us = 5000; // for fake processing 5ms


  // Connecting to sink
  printf("[worker %d] connecting to sink at %s\n", getpid(), sink_path);
  int sink_fd = unix_connect_retry(sink_path, 20, 100000);
  if (sink_fd < 0)
  {
    fprintf(stderr, "[worker %d] could not connect to sink\n", getpid());
    return 1;
  }
 
  // listening for a connection from sensor
  int listen_fd = unix_listen(listen_path);
  if (listen_fd < 0)
  {
    fprintf(stderr, "[worker %d] could not listen on %s\n", getpid(), listen_path);
    return 1;
  }
  printf("[worker %d] is listening on %s\n", getpid(), listen_path);


  // connection accepting once sensor-sim is queued for connection on listen_fd, this will produce another fd
  int client_fd = accept(listen_fd, NULL, NULL);
  if (client_fd < 0)
  {
    perror("[worker] listener accept");
    return 1;
  }
  printf("[worker %d] sensor-sim connected, processing frames\n", getpid());

  // Processing of frames,
  uint64_t processed = 0;
  frame_t frame; // reusable slot

  for(;;)
  {
    ssize_t r = read_full(client_fd, &frame, sizeof(frame)); // total bytes read
    if (r == 0)
    {
      printf("[worker %d] sensor-sim disconnected (EOF) \n", getpid());
      break;
    }
    if (r != (ssize_t)sizeof(frame))
    {
      fprintf(stderr, "[worker %d] short read/error from sensor-sim\n", getpid());
      break;
    }

    usleep(processing_delay_us); // fake processing

    // making the result to give to the sink, normally this would include the result of processing
    // no processing result is added, because this is a simulation
    result_t result;
    result.seq = frame.seq;
    result.gen_time_ns = frame.gen_time_ns;
    result.worker_pid = getpid();
    

    // write to sink
    ssize_t w = write_full(sink_fd, &result, sizeof(result));
    if (w != (ssize_t)sizeof(result))
    {
      fprintf(stderr, "[worker %d] short write to sink\n", getpid());
      break;
    }
    processed++; // processed frame counter
  }
  
  printf("[worker %d] processed %llu frames, shutting down\n", getpid(), (unsigned long long)processed);
  close(client_fd);
  close(sink_fd);
  close(listen_fd);
  return 0;
}

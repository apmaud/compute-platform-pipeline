#include <stdio.h>
#include <sys/socket.h>
#include "pipeline.h"

int main(int argc, char *argv[])
{
  const char *listen_path = SINK_SOCK_PATH;

  int opt;
  while ((opt = getopt(argc, argv, "l:")) != -1)
  {
    switch (opt)
    {
      case 'l': listen_path = optarg; break;
      default:
        fprintf(stderr, "usage: %s [-l listen_path]\n, argv[0]", argv[0]);
        return 1;
    }
  }

  int listen_fd = unix_listen(listen_path);
  if (listen_fd < 0 )
  {
    fprintf(stderr, "[sink] could not listen on %s\n", listen_path);
    return 1;
  }
  printf("[sink] listening on %s\n", listen_path);
  
  int client_fd = accept(listen_fd, NULL, NULL);
  if (client_fd < 0)
  {
    perror("accept");
    return 1;
  }
  printf("[sink] worker connected, receiving results\n");

  uint64_t count = 0;
  result_t result;

  for (;;)
  {
    ssize_t r = read_full(client_fd, &result, sizeof(result));
    if (r == 0)
    {
      printf("[sink] worker disconnected (EOF)\n");
      break;
    }

    if (r !=  (ssize_t)sizeof(result))
    {
      fprintf(stderr, "[sink] short read/error from worker\n");
      break;
    }

    uint64_t now_ns = now_monotonic_ns();
    uint64_t latency_ns = now_ns - result.gen_time_ns;
    
    printf("[sink] seq=%-4llu worker_pid=%-6d latency=%.3f ms\n",
        (unsigned long long)result.seq,
        (int)result.worker_pid,
        latency_ns / 1e6);
    count++;
  }

  printf("[sink] total results received: %llu\n", (unsigned long long)count);
  close(client_fd);
  close(listen_fd);
  return 0;
  // receive a result

  // figure out the latency
  // print or record the latency


}

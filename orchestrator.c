#include <stdlib.h>
#include <stdio.h>


#include "pipeline.h"

#define MAX_WORKERS 32

int main (int argc, char *argv[])
{
  // spawn N workers: how many, socket path for each, PID and connection fd
  int num_workers = 3;
  const char *listen_path = WORKER_SOCK_PATH;
  const char *sink_path = SINK_SOCK_PATH;

  int opt;
  while ((opt = getopt(argc, argv, "w:l:s")) != -1)
  {
    switch (opt)
    {
      case 'w': num_workers = atoi(optarg); break;
      case 'l': listen_path = optarg; break;
      case 's': sink_path = optarg; break;
      default:
        fprintf(stderr, "usage: %s [-w num_workers] [-l listen_path] [-s sink_path]\n", argv[0]);
        return 1;
    }
  }

  if (num_workers < 1 || num_workers > MAX_WORKERS)
  {
    fprintf(stderr, "[orchestrator] num_workers must be between 1 and %d\n", MAX_WORKERS);
  }
  
  // arrays for indexing for tracking
  char worker_paths[MAX_WORKERS][64];
  pid_t worker_pids[MAX_WORKERS];
  int worker_fds[MAX_WORKERS];





  // accept frames from sensor-sim
  // round-robin dispatch each frame to one of the N workers
  
}

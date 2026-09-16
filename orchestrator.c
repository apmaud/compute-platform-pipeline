#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>

#include "pipeline.h"

#define MAX_WORKERS 32


static pid_t spawn_worker(const char *worker_path, const char *sink_path)
{
  pid_t pid = fork();
  if (pid < 0)
  {
    perror("fork");
    return -1;
  }
  if (pid == 0) // only true in the new child
  {
    // replaces current process image with a new process image
    execl("./worker", "worker", "-l", worker_path, "-c", sink_path, (char *)NULL);
    perror("execl");
    _exit(1);
  }
  return pid;
}

int main (int argc, char *argv[])
{
  //  SIGPIPE = broken pipe, signal kernal sends to a process when calling write() on a socket whose reading end no longer exists (worker or sink)
  // ignores the kill action to the orchestrator if writing to a dead worker and returns -1 ordinary path failure
  signal(SIGPIPE, SIG_IGN); 

  // spawn N workers: how many, socket path for each, PID and connection fd
  int num_workers = 3;
  const char *listen_path = WORKER_SOCK_PATH;
  const char *sink_path = SINK_SOCK_PATH;

  int opt;
  while ((opt = getopt(argc, argv, "w:l:s:")) != -1)
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
    return 1;
  }
  
  // arrays for indexing for tracking
  char worker_paths[MAX_WORKERS][64];
  pid_t worker_pids[MAX_WORKERS];
  int worker_fds[MAX_WORKERS];

  // spawning the workers, need unique socket path, child process running the actual worker binary and reuslting PID saved

  for (int i = 0; i < num_workers; i++)
  {
    snprintf(worker_paths[i], sizeof(worker_paths[i]), "/tmp/pipeline_worker_%d.sock", i);

    worker_pids[i] = spawn_worker(worker_paths[i], sink_path);
    if (worker_pids[i] < 0 ) return 1;
    printf("[orchestrator] spawned worker %d (pid=%d) listening at %s\n", i, worker_pids[i], worker_paths[i]);
  }

  // connect to each spawned worker
  for (int i = 0; i < num_workers; i++)
  {
    worker_fds[i] = unix_connect_retry(worker_paths[i], 20, 100000);
    if (worker_fds[i] < 0)
    {
      fprintf(stderr, "[orchestrator] could not connect to worker %d\n", i);
      return 1;
    }
    printf("[orchestrator] connected to worker %d\n", i);

  }

  // accept frames from sensor-sim
  int listen_fd = unix_listen(listen_path);
  if (listen_fd < 0)
  {
    fprintf(stderr, "[orchestrator] could not listen on %s\n", listen_path);
    return 1;
  }
  printf("[orchestrator] listening for sensor-sim on %s\n", listen_path);
  
  int client_fd = accept(listen_fd, NULL, NULL);
  if (client_fd < 0)
  {
    perror("accept");
    return 1;
  }
  printf("[orchestrator] sensor-sim connected, dispatching frames\n");


  // round-robin dispatch each frame to one of the N workers
  uint64_t dispatched = 0;
  int next_worker = 0;
  frame_t frame;
  for (;;)
  {
    // reading from sensor
    ssize_t r = read_full(client_fd, &frame, sizeof(frame));
    if (r == 0)
    {
      printf("[orchestrator] sensor-sim disconnected (EOF)\n");
      break;
    }
    if (r != (ssize_t)sizeof(frame))
    {
      fprintf(stderr, "[orchestrator] short read/error from sensor-sim\n");
      break;
    }

    // writing to worker
    ssize_t w = write_full(worker_fds[next_worker], &frame, sizeof(frame));
    if (w != (ssize_t)sizeof(frame))
    {
      fprintf(stderr, "[orchestrator] worker %d appears dead, respawning...\n", next_worker);
      close(worker_fds[next_worker]);
      worker_pids[next_worker] = spawn_worker(worker_paths[next_worker], sink_path);
      worker_fds[next_worker] = unix_connect_retry(worker_paths[next_worker], 20, 100000);
      if (worker_pids[next_worker] < 0 || worker_fds[next_worker] < 0)
      {
        fprintf(stderr, "[orchestrator] failed to respawn worker %d, giving up\n", next_worker);
        break;
      }
      printf("[orchestrator] worker %d respawned (pid=%d)\n", next_worker, worker_pids[next_worker]);
      write_full(worker_fds[next_worker], &frame, sizeof(frame));
    }

    dispatched++;
    next_worker = (next_worker + 1) % num_workers;
  }

  printf("[orchestrator] dispatched %llu frames total \n", (unsigned long long) dispatched);

  close(client_fd);
  close(listen_fd);
  for (int i = 0; i < num_workers; i++)
  {
    close(worker_fds[i]);
  }
  // process has finished running, collecting entry sitting in kernel's proces table
  for (int i = 0; i < num_workers; i++)
  {
    waitpid(worker_pids[i], NULL, 0);
  }

}

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <stdlib.h>

#include "pipeline.h"

// per-connection bookkeeping struct
typedef struct {
  int fd;
  uint8_t buf[sizeof(result_t)];
  size_t buf_used; // bytes of current result_t collected so far
} sink_conn_t;

typedef struct
{
  uint64_t count;
  uint64_t sum_latency_ns;
  uint64_t min_latency_ns;
  uint64_t max_latency_ns;
  uint64_t first_result_ns;
  uint64_t last_result_ns;
} stats_t;

// called once total of 24 bytes actually arrived for result_t, then we can process it!
static void handle_result(sink_conn_t *conn, stats_t *stats, int quiet)
{
  // copies buffer in conn to the actual properly typed result_t
  result_t result;
  memcpy(&result, conn->buf, sizeof(result));

  // latency calculated and printed
  uint64_t now_ns = now_monotonic_ns();
  uint64_t latency_ns = now_ns - result.gen_time_ns;


  if (!quiet)
  {
    printf("[sink] seq=%-4llu worker_pid=%-6d latency=%.3f ms\n",
      (unsigned long long)result.seq,
      (int)result.worker_pid,
      latency_ns / 1e6);
  }

  if (stats->count == 0)
  {
    stats->first_result_ns = now_ns;
  }
  stats->last_result_ns = now_ns;
  stats->count++;
  stats->sum_latency_ns += latency_ns;
  if (latency_ns < stats->min_latency_ns) stats->min_latency_ns = latency_ns;
  if (latency_ns > stats->max_latency_ns) stats->max_latency_ns = latency_ns;

  conn->buf_used = 0;
}

static void print_summary(const stats_t *stats)
{
  printf("\n[sink] ===== summary =====\n");
  printf("[sink] total results:   %llu\n", (unsigned long long)stats->count);
  
  if (stats->count == 0) return;

  double avg_ms = (double)stats->sum_latency_ns / (double)stats->count / 1e6;
  double min_ms = (double)stats->min_latency_ns / 1e6;
  double max_ms = (double)stats->max_latency_ns / 1e6;
  printf("[sink] avg latency:     %.3f ms\n", avg_ms);
  printf("[sink] min latency:     %.3f ms\n", min_ms);
  printf("[sink] max latency:     %.3f ms\n", max_ms);

  // guard block, if only one result, first and last are the same and elapsed is 0
  if (stats->count > 1) {
      double elapsed_s = (double)(stats->last_result_ns - stats->first_result_ns) / 1e9;
      if (elapsed_s > 0.0) {
          printf("[sink] throughput:      %.1f frames/sec\n", (double)stats->count / elapsed_s);
      }
  }
}

int main(int argc, char *argv[])
{
  const char *listen_path = SINK_SOCK_PATH;
  int quiet = 0;

  int opt;
  while ((opt = getopt(argc, argv, "l:q")) != -1)
  {
    switch (opt)
    {
      case 'l': listen_path = optarg; break;
      case 'q': quiet = 1; break;
      default:
        fprintf(stderr, "usage: %s [-l listen_path]\n, argv[0]", argv[0]);
        return 1;
    }
  }

  // Begin non-blocking listen
  int listen_fd = unix_listen(listen_path);
  if (listen_fd < 0 )
  {
    fprintf(stderr, "[sink] could not listen on %s\n", listen_path);
    return 1;
  }
  set_nonblocking(listen_fd);
  printf("[sink] listening on %s\n", listen_path);

  // create empty watch list for kernel to watch: will be watching new for new connections (ptr = NULL) and already connected (ptr = conn)
  int epfd = epoll_create1(0);
  if (epfd < 0)
  {
    perror("epoll_create1");
    return 1;
  }
  // Create event for epoll to watch for, EPOLLIN means when the fd has data ready to read, (ready to get accepted for listening sockets)
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.ptr = NULL; // marker for saying this event came from the listening socket, not a worker connection
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0)
  {
    perror("epoll_ctl (listen_fd)");
    return 1;
  }



  stats_t stats;
  memset(&stats, 0, sizeof(stats));
  stats.min_latency_ns = UINT64_MAX; // to the max, so first real latency is guaranteed to be smaller

  int active_workers = 0; // how many workers are currently connected right now
  int ever_connected = 0; // flag that flips to 1 the first time any worker connects to the sink, never flips back
  
  struct epoll_event events[16];
  // blocks on watching for any events in the watch list
  for(;;)
  {
    int n = epoll_wait(epfd, events, 16, -1); // count of how many events are ready
    if (n < 0)
    {
      if (errno == EINTR) continue;
      perror("epoll_wait");
      break;
    }

    for (int i = 0; i < n; i++) // looping over n fds are that havve something ready
    {
      // NEW CONNECTIONS
      if (events[i].data.ptr == NULL) // check data.ptr value that fd was registered with
      {
        for (;;)
        {
          int client_fd = accept(listen_fd, NULL, NULL);
          if (client_fd < 0) break;

          set_nonblocking(client_fd);
          
          // heap allocated variable, needs to exist after this code block
          sink_conn_t *conn = calloc(1, sizeof(sink_conn_t));
          conn->fd = client_fd;
          
          // hand epoll a pointer to this specific worker's own state struct, tied to this specific fd
          struct epoll_event cev;
          cev.events = EPOLLIN;
          cev.data.ptr = conn;
          epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev);
          
          active_workers++;
          ever_connected = 1;
          printf("[sink] worker connected (fd=%d), %d active\n", client_fd, active_workers);
        }
        continue;
      }
      
      // ALREADY ACCEPTED WORKER CONNECTION THAT IS READABLE
      sink_conn_t *conn = events[i].data.ptr;
      size_t remaining = sizeof(result_t) - conn->buf_used; // how many more bytes this connection still needs before it has a complete result_t (24 bytes total)
      ssize_t r = read(conn->fd, conn->buf + conn->buf_used, remaining); // reads to next unfilled byte in this connection's buffer

      if (r < 0)
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK) continue; // normal for a non-blocking fd, continue skips to to top of for loop to the next ready event, will get called again for this fd once there is something ro read
        r = 0;
      }
      if (r == 0) // worker closed the connection
      {
        printf("[sink] worker disconnected (fd=%d)\n", conn->fd);
        epoll_ctl(epfd, EPOLL_CTL_DEL, conn->fd, NULL); //stop epoll from watching the fd
        close(conn->fd);
        free(conn);
        active_workers--;
        if(ever_connected && active_workers == 0) goto done; // exit if every worker that ever showed up has left
        continue;
      }

      // however many bytes read managed to deliver, add to the connectiosn running total
      conn->buf_used += (size_t)r;
      if(conn->buf_used == sizeof(result_t))
      {
        handle_result(conn, &stats, quiet); // if 24 bytes came through, actualy process it
      }
    }
  }

done:
  print_summary(&stats);
  close(listen_fd);
  close(epfd);
  return 0;

}

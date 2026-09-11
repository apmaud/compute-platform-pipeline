#ifndef PIPELINE_H
#define PIPELINE_H 

#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#define PAYLOAD_SIZE 64
#define WORKER_SOCK_PATH "/tmp/pipeline_worker.sock"
#define SINK_SOCK_PATH "/tmp/pipeline_sink.sock"

typedef struct
{
  uint64_t seq;
  uint64_t gen_time_ns;
  uint8_t payload[PAYLOAD_SIZE];
} frame_t;

typedef struct
{
  uint64_t seq;
  uint64_t gen_time_ns;
  pid_t worker_pid;
} result_t;


int unix_connect(const char *path);
int unix_connect_retry(const char *path, int max_attempts, useconds_t delay_us);
uint64_t now_monotonic_ns(void);
ssize_t write_full(int fd, const void *buf, size_t n);
ssize_t read_full(int fd, const void *buf, size_t n);
int unix_listen(const char *path);

#endif

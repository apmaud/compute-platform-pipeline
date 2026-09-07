#include <bits/time.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>

#include "pipeline.h"


uint64_t now_monotonic_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts); // almost like a stopwatch start
  return (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec; // conversion to nanoseconds for simplicity later
}

ssize_t write_full(int fd, const void *buf, size_t n)
{
  

}

int unix_connect(const char *path)
{
  return -1;
}

int unix_connect_retry(const char *path, int max_attempts, useconds_t delay_us)
{
  return -1;
}

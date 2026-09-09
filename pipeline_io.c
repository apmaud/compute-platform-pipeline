#include <bits/time.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>

#include "pipeline.h"


uint64_t now_monotonic_ns(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts); // almost like a stopwatch start
  return (uint64_t)ts.tv_sec * 1000000000 + (uint64_t)ts.tv_nsec; // conversion to nanoseconds for simplicity later
}

ssize_t write_full(int fd, const void *buf, size_t n)
{
  const uint8_t *p = buf; // 1 byte exactly because its the memory address start, this will move! It points to memory address 0 of the frame
  size_t left = n;
  while (left > 0)
  {
    ssize_t w = write(fd, p, left);
    if (w < 0)
    {
      if (errno == EINTR) continue;
      return -1;
    }
    if (w == 0) break;
    p += w; // move forward this pointer however many bytes were successfully written
    left -= (size_t)w; // get new total number of bytes to write
  }

  return (ssize_t)(n-left); // returns the total bytes written (total bytes of frame - new total number of bytes to write)

}

int unix_connect(const char *path)
{
  int fd = socket(AF_UNIX, SOCK_STREAM, 0); // woohoo unix sockets
  if (fd < 0) return -1;

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  if (strlen(path) >= sizeof(addr.sun_path))
  {
    fprintf(stderr, "socket path too long: %s\n", path);
    close(fd);
    return -1;
  }
  strncpy(addr.sun_path, path, sizeof(addr.sun_path));

  if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    close(fd);
    return -1;
  }
  
  return fd;
}

int unix_connect_retry(const char *path, int max_attempts, useconds_t delay_us)
{
  for (int i = 0; i < max_attempts; i++)
  {
    int fd = unix_connect(path);
    if (fd >= 0) return fd;
    usleep(delay_us);
  }
  return -1;
}

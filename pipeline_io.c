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

ssize_t read_full(int fd, const void *buf, size_t n)
{
  uint8_t *p = buf;
  size_t left = n;
  while (left > 0)
  {
    ssize_t r = read(fd, p, left);
    if (r < 0)
    {
      if (errno == EINTR) continue;
      return -1;
    }
    if (r == 0) break;
    p += r;
    left -= (size_t)r;
  }
  return (ssize_t)(n-left); // returns total bytes read
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
  strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1); // added path - 1 later on, looking at man pages, need space for null terminating byte?

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

int unix_listen(const char *path)
{
  // make socket for bind and listen
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) return -1;

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  if (strlen(path) >= sizeof(addr.sun_path))
  {
    fprintf(stderr, "[worker] listen: socket path too long %s\n", path);
    close(fd);
    return -1;
  }
  strncpy(addr.sun_path, path, sizeof(path)-1);  // -1 for null temrinating byte, man page
  
  unlink(path); // removes stale socket file from a previous run
                
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
  {
    perror("[worker] bind");
    close(fd);
    return -1;
  }

  if (listen(fd, 1) < 0)
  {
    perror("[worker] listen");
    close(fd);
    return -1;
  }

  return fd;
}


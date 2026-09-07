#include <time.h>
#include <sys/types.h>

#include "pipeline.h"

int unix_connect(const char *path)
{
  return -1;
}

int unix_connect_retry(const char *path, int max_attempts, useconds_t delay_us)
{
  return -1;
}

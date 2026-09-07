#ifndef PIPELINE_H
#define PIPELINE_H 

#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>

#define PAYLOAD_SIZE 64

typedef struct
{
  uint64_t seq;
  uint64_t gen_time_ns;
  uint8_t payload[PAYLOAD_SIZE];
} frame_t;


int unix_connect(const char *path);
int unix_connect_retry(const char *path, int max_attempts, useconds_t delay_us);

#endif

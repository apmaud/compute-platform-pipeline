#ifndef PIPELINE_H
#define PIPELINE_H 

#include <cstdint>

#define PAYLOAD_SIZE 16384



typedef struct
{
  uint64_t seq;
  uint64_t gen_time_ns;
  uint8_t payload[PAYLOAD_SIZE];
} frame_t;

#endif

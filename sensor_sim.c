#include <linux/limits.h>
#include <string.h>
#include <sys/types.h> 
#include <stdio.h>
#include <error.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>

#include "pipeline.h"

int main(int argc, char *argv[])
{
  // simulated freq for a sensor
  double rate_hz = 10.0;
  int num_frames = 50;
  char *connect_path = WORKER_SOCK_PATH;

  // option flags to put on the call, so we can edit the simulated sensor options, loop will pull out all flags in argv
  int opt;
  while ((opt = getopt(argc, argv, "r:n:s:")) != -1)
  {
    switch (opt) // opt returns the arg it scanned for and the value it scanned for : after it, stored in optarg
    {
      case 'r': rate_hz = atof(optarg); break;
      case 'n': num_frames = atoi(optarg); break;
      case 's': connect_path = optarg; break;
      default:
        fprintf(stderr, "Usage: %s [-r rate_hz] [-n num_frames] [-s socket_path] \n", argv[0]);
        return 1;
    }
  }
  useconds_t period_us = (useconds_t)(1000000.0 / rate_hz); // conversion


  // must connect to worker somehow, fd for that
  printf("[sensor-sim] connection to worker at %s\n", connect_path);
  int fd = unix_connect_retry(connect_path, 20, 100000);
  if (fd < 0)
  {
    fprintf(stderr, "[sensor-sim] could not connect to worker\n");
    return 1;
  }
  printf("[sensor-sim] connected, sending %d frames at %f Hz \n", num_frames, rate_hz);
 
  
  for (int i = 0; i < num_frames; i++)
  {
    // zero out frame
    frame_t frame;
    memset(&frame, 0, sizeof(frame));
    // fill out frame with data
    frame.seq = (uint64_t)i;
    frame.gen_time_ns = now_monotonic_ns();
    memset(frame.payload, (int)(i & 0xFF), sizeof(frame.payload)); // note memset only overwrites with first 8 bits of int, we also bitwise & the current loop int with the 0xFF mask to focus on only those first 8 bits again 

    // write in full the frame
    ssize_t w = write_full(fd, &frame, sizeof(frame));
    if (w != (ssize_t)sizeof(frame))
    {
      fprintf(stderr, "[sensor-sim] short write on frame %d (wrote %zd of %zu bytes)\n", i, w, sizeof(frame));
      break;
    }
    // sleep to simulate frequency again of a sensor
    usleep(period_us); 
  }

  printf("[sensor-sim] done sending, closing connection\n");
  close(fd);
  return 0;
}

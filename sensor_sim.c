#include <linux/limits.h>
#include <sys/types.h> 
#include <stdio.h>
#include <error.h>
#include <unistd.h>
#include <stdlib.h>

#include "pipeline.h"

int main(int argc, char *argv[])
{
  // simulated freq for a sensor
  double rate_hz = 10.0;
  int num_frames = 50;
  char *connect_path = "connect path";

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



  // must connect to worker somehow, fd for that
  int fd = unix_connect_retry(connect_path, 20, 100000);
  if (fd < 0)
  {
    perror("Fail unix connect");
  }
  
  
  for (;;)
    // make a frame
    // send it to a worker
    // cd
    //
    //
  return 0;
}

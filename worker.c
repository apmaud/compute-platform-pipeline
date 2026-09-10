


#include "pipeline.h"
int main()
{
  char *sink_path = "sink path";

  // receive a frame
  // how can we receive a frame? unix socket to receive connection 
  int listen_fd; // listening for a connection from sensor
  int client_fd; // connection once sensor sim appears

  int 



  // process it 
  // what does it mean to process? should just wait, act lik eim processing or actually perform something
  


  // forward it 
  // connect to sink socket
  int sink_fd = unix_connect_retry(sink_path, 20, 100000);






  
  return 0;
}

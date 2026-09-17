This is a simulated distributed sensor-processing pipeline written in C, communicating entirely over Unix domain sockets.
A producer generates synthetic sensor frames, an orchestrator round-robin dispatches the frames across a pool of worker processes it spawns and supervises, and a sink collects the processed results and reports real throughput and latency statistics.
It models the same shape as a real compute-platform pipeline: fan-out processing across a fixed worker pool, detection and recovery for downed parts, epoll-based multiplexing on the collection side to handle multiple simultaneous connections in a single thread.

<h2>Conceptually the program operates in ~four layers tied together by shared Unix-socket transport:</h2>

<h3>1: Frame generation (sensor-sim)</h3>
- Generates timestamped example frames at a configurable rate, acts how a real sensor would
- Each frame carries a seq number, a timestamp, and a fixed-size payload
- Connects out over a Unix domain socket to whatever is listening downstream

<h3>2: Dispatch and failure-recovery layer (orchestrator)</h3>
- Spawns a fixed pool of N worker processes via fork() + exec(), each with their own listen path
- Connects out to workers and round-robins each incoming frame across them in strict rotation
- Detects dead workers reactively when a write fails, and respawns a replacement at the same socket path
- Reaps zombie children processes on shutdown

<h3>3: Processing layer (worker)</h3>
- Accepts a single upstream connection and connects downstream to sink
- Reads each incoming frame, performs "processing" and forwards a processed result

<h3>4: Collection and Aggregation Layer (sink)</h3>
- Accepts connections from a number of workers at once, through a single-threaded and non-blocking epoll event loop
- Computes per-frame latency
- Tracks stats and throughput, prints the summary once connections are shut down

<h3> Transport</h3>
- Every inter-process link in the pipeline is a Unix domain socket
- A small shared library provides reused read/write function helpers that guarantee a full sized struct is transferred even if multiple read/writes are needed

<h2>Setup</h2>
Compile with CMake:
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

A Dockerfile and docker-compose.yml are included, which will containerize sensor-sim, the orchestrator (and spawned workers) and the sink as three services sharing a Docker volume for the shared Unix socket files.
```bash
docker compose up --build
```

To run locally, start these in order in three terminals:
```bash
./sink -q

./orchestrator -w 3

./sensor_sim -r 20 -n 50
```

<h2> Things to Improve </h2>
Keep in mind this was made for my own education and practice, to work through making a distributed data pipeline in C. There is plenty to improve here:
- Ideally I should have made each worker its own container
- TCP/IP sockets should have been used for communication between separate containers. A shared volume can be iffy
- Actually generated some realistic data, actually processed it, and forwarded everything to the sink
- Also a big one: Since I'm round-robining the frames to a worker, I currently do not have a way to re-order the frames in the original way they were sent from the sensor, once they arrive at the sink. If this was actual data, then it would be jumbled.

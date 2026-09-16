FROM debian:bookworm-slim AS builder
# installs GCC, CMAKE, MAKE
RUN apt-get update && apt-get install -y --no-install-recommends gcc cmake make && \ rm -rf /var/lib/apt/lists/* 
# sets working directory
WORKDIR /src
# copy files from local to image's directory
COPY pipeline.h pipeline_io.c sensor_sim.c worker.c sink.c orchestrator.c CMakeLists.txt ./
# makes a build directory and cmake the build files into the directory -> invoke what was generated (make) to compile
RUN mkdir build && cd build && cmake .. && cmake --build .

# a new stage, copy the built files into this image
FROM debian:bookworm-slim
WORKDIR /app
COPY --from=builder /src/build/sensor_sim /src/build/worker /src/build/sink /src/build/orchestrator ./

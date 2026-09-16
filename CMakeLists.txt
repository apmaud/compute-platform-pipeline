cmake_minimum_required(VERSION 3.10)
project(sensor_pipeline C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# build from .c file to pipeline_io as a shared STATIC library 
# compiles into an archive file that gets linked into whatever executable uses it
# pipeline_io -> libpipeline_io.a (container holding pipeline_io.o) -> linker copies object's actual code directly into sensor_sim, worker, sink and orchestrator
add_library(pipeline_io STATIC pipeline_io.c)
target_compile_options(pipeline_io PRIVATE -Wall -Wextra -g)

# Sensor executable
add_executable(sensor_sim sensor_sim.c)
target_link_libraries(sensor_sim PRIVATE pipeline_io) # tells CMake linker to pull in libpipeline_io when copiling sensor_sim.c
target_compile_options(sensor_sim PRIVATE -Wall -Wextra -g)

# Worker executable
add_executable(worker worker.c)
target_link_libraries(worker PRIVATE pipeline_io)
target_compile_options(worker PRIVATE -Wall -Wextra -g)

# Sink executable
add_executable(sink sink.c)
target_link_libraries(sink PRIVATE pipeline_io)
target_compile_options(sink PRIVATE -Wall -Wextra -g)

# Orchestrator executable
add_executable(orchestrator orchestrator.c)
target_link_libraries(orchestrator PRIVATE pipeline_io)
target_compile_options(orchestrator PRIVATE -Wall -Wextra -g)

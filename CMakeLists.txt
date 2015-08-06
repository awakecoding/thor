cmake_minimum_required(VERSION 2.8)

project(Thor C)

set(CMAKE_C_FLAGS "-std=c99 -g -O3 -pedantic -Wall -Wno-unused-result")

set(THOR_SOURCES
	libthor/block.c
	libthor/frame.c
	libthor/transform.c
	libthor/intra_prediction.c
	libthor/inter_prediction.c
	libthor/common_kernels.c
	libthor/snr.c
	libthor/simd.c
	libthor/bits.c
	libthor/vlc.c
	libthor/strings.c
	libthor/enc_kernels.c)

include_directories(libthor)

add_library(thor STATIC ${THOR_SOURCES})

if(UNIX)
	target_link_libraries(thor m)
endif()

add_executable(thor-tool tool/thor.c)
target_link_libraries(thor-tool thor)

set_target_properties(thor-tool PROPERTIES OUTPUT_NAME "thor")


cmake_minimum_required(VERSION 2.8)

project(Thor C)

set(CMAKE_C_FLAGS "-std=c99 -g -O3 -pedantic -Wall -Wno-unused-result")

set(THOR_COMMON_SOURCES
	common/common_block.c
	common/common_frame.c
	common/transform.c
	common/intra_prediction.c
	common/inter_prediction.c
	common/common_kernels.c
	common/snr.c
	common/simd.c)

set(THOR_ENCODER_SOURCES
	enc/encode_block.c
	enc/encode_frame.c
	enc/mainenc.c
	enc/putbits.c
	enc/putvlc.c
	enc/strings.c
	enc/write_bits.c
	enc/enc_kernels.c)

set(THOR_DECODER_SOURCES
	dec/decode_block.c
	dec/getbits.c
	dec/getvlc.c
	dec/maindec.c
	dec/read_bits.c
	dec/decode_frame.c)

include_directories(common)

add_executable(thorenc ${THOR_COMMON_SOURCES} ${THOR_ENCODER_SOURCES})
add_executable(thordec ${THOR_COMMON_SOURCES} ${THOR_DECODER_SOURCES})

if(UNIX)
	target_link_libraries(thorenc m)
	target_link_libraries(thordec m)
endif()




set(THOR_HEADERS
	block.h
	frame.h
	intra.h
	inter.h
	snr.h
	simd.h
	bits.h
	vlc.h
	image.h
	params.h
	kernels.h
	transform.h
	thor.h)

set(THOR_SOURCES
	block.c
	frame.c
	intra.c
	inter.c
	snr.c
	simd.c
	bits.c
	vlc.c
	color.c
	image.c
	params.c
	kernels.c
	transform.c
	thor.c)

add_library(thor STATIC ${THOR_SOURCES} ${THOR_HEADERS})

if(UNIX)
	target_link_libraries(thor m)
endif()

install(TARGETS thor DESTINATION ${CMAKE_INSTALL_LIBDIR} EXPORT ThorTargets)


cmake_minimum_required(VERSION 2.8.10)

project(Thor C)

include(GNUInstallDirs)
include(CheckCCompilerFlag)
include(CMakePackageConfigHelpers)

set(THOR_VERSION_MAJOR "0")
set(THOR_VERSION_MINOR "1")
set(THOR_VERSION_REVISION "0")
set(THOR_API_VERSION "${FREERDP_VERSION_MAJOR}.${FREERDP_VERSION_MINOR}")
set(THOR_VERSION "${FREERDP_API_VERSION}.${FREERDP_VERSION_REVISION}")

if(UNIX)
	check_c_compiler_flag(-Wno-unused-result Wno-unused-result)
	if(Wno-unused-result)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
	endif()

	check_c_compiler_flag(-Wno-unused-but-set-variable Wno-unused-but-set-variable)
	if(Wno-unused-but-set-variable)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
	endif()
endif()

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32_LEAN_AND_MEAN")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4244")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHsc")

	if(CMAKE_VS_PLATFORM_TOOLSET MATCHES "^Intel.*")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Qip /O2 /QxSSE3 /Qrestrict /Qansi_alias /Qalias_args- /fp:fast=2")
	endif()
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3 -Wall")
endif()

include_directories(include)

add_subdirectory(include)
add_subdirectory(libthor)
add_subdirectory(tool)

include(CTest)
enable_testing()
add_subdirectory(test)

export(PACKAGE thor)

set(THOR_CMAKE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/thor")
set(THOR_INCLUDE_DIR "include")

configure_package_config_file(ThorConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/ThorConfig.cmake
	INSTALL_DESTINATION ${THOR_CMAKE_INSTALL_DIR} PATH_VARS THOR_INCLUDE_DIR)

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/ThorConfigVersion.cmake
	VERSION ${THOR_VERSION} COMPATIBILITY SameMajorVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ThorConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/ThorConfigVersion.cmake
	DESTINATION ${THOR_CMAKE_INSTALL_DIR})

install(EXPORT ThorTargets DESTINATION ${THOR_CMAKE_INSTALL_DIR})


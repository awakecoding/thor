cmake_minimum_required(VERSION 2.8.10)

project(Thor C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(THOR_VERSION_MAJOR "0")
set(THOR_VERSION_MINOR "1")
set(THOR_VERSION_REVISION "0")
set(THOR_API_VERSION "${FREERDP_VERSION_MAJOR}.${FREERDP_VERSION_MINOR}")
set(THOR_VERSION "${FREERDP_API_VERSION}.${FREERDP_VERSION_REVISION}")

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_CRT_SECURE_NO_WARNINGS")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWIN32_LEAN_AND_MEAN")
else()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3 -Wall -Wno-unused-result")
endif()

include_directories(include)
include_directories(libthor)

add_subdirectory(include)
add_subdirectory(libthor)
add_subdirectory(tool)

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


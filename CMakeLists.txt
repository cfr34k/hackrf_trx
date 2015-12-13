cmake_minimum_required (VERSION 2.6)
project (hackrf_trx)

# The current project version
set(VERSION_NUMBER "0.0.0")

# CFLAGS configuration
set(CMAKE_C_FLAGS "-std=c99 -Wall -pedantic -D_POSIX_C_SOURCE=199309L")
set(CMAKE_C_FLAGS_DEBUG "-ggdb -O0")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Find Sources
file(GLOB_RECURSE C_SRCS src/*.c)
file(GLOB_RECURSE C_HDRS src/*.h)

# pthreads
find_package(Threads)

# Version header generation
find_package(Git)

set(HACKRF_LIBRARIES hackrf)

execute_process(COMMAND
  "${GIT_EXECUTABLE}" rev-parse --short HEAD
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	OUTPUT_VARIABLE REVISION
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in" "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

# Linking
add_executable(${CMAKE_PROJECT_NAME} ${C_SRCS})

target_link_libraries(${CMAKE_PROJECT_NAME} ${HACKRF_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT})

######################################################################################################################
# Setup
######################################################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.20)    # define the minimum required version of CMake to be used
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # For clang-tidy.
SET(BUILD_SHARED_LIBS OFF)              # We expect external libraries to be linked statically.
SET(CMAKE_CXX_STANDARD 20)              # Compile as C++20. GoogleTest requires at least C++14
SET(CMAKE_CXX_STANDARD_REQUIRED ON)     # Require C++20 support.

# Define the project meta information
PROJECT(Cupid
        VERSION 2026.01
        DESCRIPTION "A matching engine in modern C++ for performance and elegance"
        LANGUAGES C CXX
        )

# Detect the operating system
MESSAGE("Compiling on the operating system of ${CMAKE_SYSTEM_NAME}")
IF (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    ADD_DEFINITIONS(-DOS_LINUX)
ELSE()
    MESSAGE(FATAL_ERROR "Only Linux is supported. Your operating system ${CMAKE_SYSTEM_NAME} is not supported.")
ENDIF()

######################################################################################################################
# Search Path Specification
######################################################################################################################

# Find the required thread package on the platform
SET(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)

######################################################################################################################
# Include
######################################################################################################################

# Includes header file path
SET(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
SET(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

######################################################################################################################
# Build
######################################################################################################################
ADD_LIBRARY(adder ${SRC_DIR}/adder.cpp)
TARGET_INCLUDE_DIRECTORIES(adder PUBLIC ${INCLUDE_DIR})

ADD_EXECUTABLE(main ${SRC_DIR}/main.cpp)
TARGET_LINK_LIBRARIES(main adder)
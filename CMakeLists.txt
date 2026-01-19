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

INCLUDE(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest
        GIT_TAG        v1.17.0 # or a later release
)
SET(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

######################################################################################################################
# Search Path Specification
######################################################################################################################

# Find the required thread package on the platform
SET(THREADS_PREFER_PTHREAD_FLAG ON)
FIND_PACKAGE(Threads REQUIRED)

# Find the Google benchmark package
FIND_PACKAGE(benchmark REQUIRED)

# Find clang-format
IF (NOT DEFINED CLANG_FORMAT_BIN)
  FIND_PROGRAM(CLANG_FORMAT_BIN NAMES clang-format clang-format-14)
ENDIF ()
IF ("${CLANG_FORMAT_BIN}" STREQUAL "CLANG_FORMAT_BIN-NOTFOUND")
    MESSAGE(WARNING "Couldn't find clang-format.")
ELSE ()
    MESSAGE(STATUS "Found clang-format at ${CLANG_FORMAT_BIN}")
ENDIF ()

# Find cpplint
IF (NOT DEFINED CLANG_FORMAT_BIN)
  FIND_PROGRAM(CPPLINT_BIN NAMES cpplint cpplint.py)
ENDIF ()
IF ("${CPPLINT_BIN}" STREQUAL "CPPLINT_BIN-NOTFOUND")
    MESSAGE(WARNING "Couldn't find cpplint.")
ELSE ()
    MESSAGE(STATUS "Found cpplint at ${CPPLINT_BIN}")
ENDIF ()

######################################################################################################################
# Include
######################################################################################################################

# Includes file path
SET(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
SET(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
SET(TEST_DIR ${PROJECT_SOURCE_DIR}/test)

######################################################################################################################
# Build
######################################################################################################################
ADD_LIBRARY(default_engine ${SRC_DIR}/default_engine.cpp)
TARGET_INCLUDE_DIRECTORIES(default_engine PUBLIC ${INCLUDE_DIR})

######################################################################################################################
# Test & Benchmark
######################################################################################################################
ENABLE_TESTING()

GET_FILENAME_COMPONENT(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
ADD_EXECUTABLE(default_engine_test ${TEST_DIR}/default_engine_test.cpp)
TARGET_LINK_LIBRARIES(
  default_engine_test
  default_engine
  GTest::gtest_main
)

INCLUDE(GoogleTest)
GTEST_DISCOVER_TESTS(default_engine_test)

ADD_EXECUTABLE(engine_benchmark ${TEST_DIR}/engine_benchmark.cpp)
TARGET_INCLUDE_DIRECTORIES(engine_benchmark PRIVATE ${INCLUDE_DIR})
TARGET_COMPILE_DEFINITIONS(engine_benchmark PRIVATE PROJECT_ROOT_PATH="${PROJECT_ROOT}")
TARGET_LINK_LIBRARIES(engine_benchmark default_engine benchmark::benchmark)
######################################################################################################################
# Formater + Linter
######################################################################################################################

SET(SUPPORT_DIR ${PROJECT_SOURCE_DIR}/support)
SET(FORMAT_STYLE google)

STRING(CONCAT FORMAT_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/src,"
        "${CMAKE_CURRENT_SOURCE_DIR}/include,"
        "${CMAKE_CURRENT_SOURCE_DIR}/test,"
        )

# "make format"
ADD_CUSTOM_TARGET(format ${SUPPORT_DIR}/run_clang_format.py
        ${CLANG_FORMAT_BIN}
        ${SUPPORT_DIR}/clang_format_exclusions.txt
        --source_dirs
        ${FORMAT_DIRS}
        --fix
        --quiet
)

# "make cpplint"
FILE(GLOB_RECURSE LINT_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
        )

ADD_CUSTOM_TARGET(cpplint
        echo '${LINT_FILES}' | xargs -n12 -P8
        ${CPPLINT_BIN}
        --verbose=2 --quiet
        --linelength=120
        --filter=-legal/copyright,-build/include_subdir
        )
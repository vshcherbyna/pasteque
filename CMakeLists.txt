cmake_minimum_required (VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project (pasteque)

include_directories(${PROJECT_SOURCE_DIR})

FILE(GLOB SRCFILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
FILE(GLOB HDRFILES ${PROJECT_SOURCE_DIR}/src/*.h)

if (MSVC)
    SET(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Ob0 /Od /RTC1")
    SET(CMAKE_CXX_FLAGS_RELEASE "/MT /O2 /Ob2 /DNDEBUG")
endif (MSVC)

if (NOT MSVC)
    add_definitions(-DNDEBUG)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -pthread")

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=barcelona")
    endif ()
endif ()

if (DEFINED _BTYPE)
    add_definitions(-D_BTYPE=${_BTYPE})

    if (NOT MSVC AND _BTYPE STREQUAL "1" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi2")
    endif ()
endif ()

add_executable(pasteque ${SRCFILES} ${HDRFILES})

if (NOT MSVC)
    target_compile_options(pasteque PRIVATE -Wall -Werror)
endif ()

option(PASTEQUE_UNIT_TESTS "Build the unit tests" ON)

if (PASTEQUE_UNIT_TESTS)
    include(FetchContent)

    FetchContent_Declare(googletest
        GIT_REPOSITORY  https://github.com/google/googletest.git
        GIT_TAG         v1.18.0
        GIT_SHALLOW     TRUE)

    FetchContent_MakeAvailable(googletest)

    enable_testing()

    FILE(GLOB UTSRCFILES ${PROJECT_SOURCE_DIR}/src/unit/*.cpp)

    add_executable(unit ${UTSRCFILES} ${SRCFILES} ${HDRFILES})
    target_link_libraries(unit gtest gtest_main)
    target_compile_definitions(unit PRIVATE UNIT_TEST)

    if (NOT MSVC)
        target_compile_options(unit PRIVATE -Wall -Werror)
    endif ()
endif ()

# 3.16 is what googletest itself asks for, and anything below 3.10 is already
# deprecated by cmake 4, which is what the ci runners now ship.
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

    # -march names an x86 model, so it is only offered to an x86 target. Apple silicon and
    # any other arm host take the same build without it. This engine is 64 bit only: the
    # bitscan and popcount it leans on are 64 bit intrinsics.
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=barcelona")
    endif ()
endif ()

# _BTYPE=1 takes the sliders through pext instead of the magic multiply. It is asked for
# rather than detected: pext is fast on intel from haswell and on amd from zen 3, and slow
# enough on the zen 1 and zen 2 parts that guessing wrong costs more than it wins.
#
# pext is an x86 instruction. On any other target the define is still accepted and the
# sliders stay on the magic multiply, so -mbmi2 is only ever offered to an x86 compiler.
if (DEFINED _BTYPE)
    add_definitions(-D_BTYPE=${_BTYPE})

    if (NOT MSVC AND _BTYPE STREQUAL "1" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mbmi2")
    endif ()
endif ()

add_executable(pasteque ${SRCFILES} ${HDRFILES})

# -Werror belongs to this engine's own sources. Setting it globally would hand it to
# googletest as well, where a newer compiler warning is not ours to fix.
if (NOT MSVC)
    target_compile_options(pasteque PRIVATE -Wall -Werror)
endif ()

# fetched by tag rather than carried as a submodule: the gitlink pinned a 2019 commit
# that cmake 4 refuses to configure, and a gitlink is easy to lose when a change moves
# between clones.

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

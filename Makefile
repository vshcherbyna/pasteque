#
# Builds pastèque for OpenBench, which runs a bare 'make' with EXE overridden and then
# starts the binary as '<exe> bench'. The first target is therefore the one that has to
# produce a working engine with no arguments and no environment.
#
#     make                      # the basic target, what OpenBench builds
#     make EXE=pasteque-dev     # ... under another name
#     make bench                # build, then run the bench the same way OpenBench will
#     make CC=clang++           # any compiler that speaks c++17
#
# CMakeLists.txt remains the build the unit tests and the IDEs use. This file exists so
# that a machine with nothing but a compiler and make can produce the same engine.
#

CC   = g++
EXE  = pasteque

# src/unit is the test suite and has a main() of its own, so the wildcard stays shallow.

SRC  = src/*.cpp

STD   = -std=c++17
WARN  = -Wall
OPTIM = -O3 -funroll-loops
DEFS  = -DNDEBUG
LIBS  = -pthread

# What the compiler says about itself, asked once. The two questions worth asking are
# whether it is clang, which spells link time optimisation differently from gcc, and
# whether it is mingw, which needs its runtime linked in statically to travel.

DEFINES := $(shell echo | $(CC) -E -dM - 2>/dev/null)
MACHINE := $(shell uname -m 2>/dev/null)

ifneq ($(findstring __clang__,$(DEFINES)),)
    OPTIM += -flto
else
    OPTIM += -flto=auto
endif

ifneq ($(findstring __MINGW32__,$(DEFINES)),)
    LIBS += -static
endif

# -march names an x86 model, so it is only offered to an x86 host. Apple silicon and
# every other arm target take the same build without it: the 64 bit bitscan and popcount
# this engine leans on are already there and need no asking for.

ifneq ($(filter x86_64 amd64 AMD64,$(MACHINE)),)
    OPTIM += -march=native -mpopcnt
endif

CFLAGS = $(STD) $(WARN) $(OPTIM) $(DEFS)

.PHONY: basic bench clean

basic:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(EXE)

bench: basic
	./$(EXE) bench

clean:
	@rm -f $(EXE) $(EXE).exe

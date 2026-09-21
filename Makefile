CC   = g++
EXE  = pasteque

SRC  = src/*.cpp

STD   = -std=c++17
WARN  = -Wall
OPTIM = -O3 -funroll-loops
BTYPE = 0
DEFS  = -DNDEBUG -D_BTYPE=$(BTYPE)
LIBS  = -pthread

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

ifneq ($(filter x86_64 amd64 AMD64,$(MACHINE)),)
    OPTIM += -march=native -mpopcnt
endif

ifeq ($(BTYPE),1)
ifneq ($(filter x86_64 amd64 AMD64,$(MACHINE)),)
    OPTIM += -mbmi2
endif
endif

CFLAGS = $(STD) $(WARN) $(OPTIM) $(DEFS)

.PHONY: basic bench clean

basic:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(EXE)

bench: basic
	./$(EXE) bench

clean:
	@rm -f $(EXE) $(EXE).exe

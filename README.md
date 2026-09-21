# Overview

<img src="pasteque.png" alt="Logo" width="200">

Pastèque is a free UCI chess engine from Ukraine. It is not a complete chess program: it needs a UCI-compatible GUI to be used.

# History

Work on Pastèque started in 2018, and it never amounted to much — mainly because I moved on to Igel soon afterwards. I recently found the old sources on my hard drive and decided to resurrect the project, so that it would at least see a release.

# Evaluation is stochastic on purpose

All evaluation parameters — piece values, twelve piece-square tables, phase weights, the tempo bonus and the move-ordering values — are random on purpose.

It is a fair question why an engine would judge a position with random numbers. The reason is to start from an engine that knows nothing about chess beyond the legal moves.

# Building

A C++17 compiler and a 64-bit target.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Builds `pasteque` and the `unit` test binary; googletest is fetched by tag at configure time. `-DPASTEQUE_UNIT_TESTS=OFF` builds the engine alone. cmake 3.16 or newer.

```
make                     # the engine, which is what OpenBench builds
make EXE=pasteque-dev    # ... under another name
make bench               # build, then run the bench
make CC=clang++          # any compiler that speaks c++17
make BTYPE=1             # sliders by pext rather than the magic multiply
```

The makefile asks the compiler about itself once and adapts: `-flto` against `-flto=auto`, `-static` for mingw, `-march=native` only on x86. It builds the engine alone — the unit tests are CMake's job.

# The search

- iterative deepening, fail-soft alpha-beta, quiescence at the horizon
- legal move generation at every node; magic bitboards for the sliders, or `pext`
- captures ordered MVV/LVA-style then promotions, insertion sorted, on the random piece values
- evaluation tapered between an opening and a closing set of tables
- the fifty-move rule, and a lone-minor endgame scored as a draw
- soft and hard time limits, the clock polled every 2048 nodes

There is no transposition table and no repetition detection yet, so the engine cannot see a threefold coming.

# Checking your build

Any build you make yourself should be checked against the reference node count:

```
pasteque bench
```

on Windows, or:

```
./pasteque bench
```

on Linux. The node total it prints must match the value in `bench.nodes`. If the two differ, your binary is not searching the same tree as the reference build, and any result it produces cannot be compared with published ones.

# Overview

<img src="pasteque.png" alt="Logo" width="200">

Pastèque is a free UCI chess engine from Ukraine. It is not a complete chess program: it needs a UCI-compatible GUI to be used.

# History

Work on Pastèque started in 2018, and it never amounted to much — mainly because I moved on to Igel soon afterwards. I recently found the old sources on my hard drive and decided to resurrect the project, so that it would at least see a release.

# Evaluation starts random on purpose

Every evaluation parameter was drawn at random, so the engine begins knowing nothing about chess beyond the legal moves. Nothing may come from another engine, from a stronger program's labelled positions, or from published values — only the rules and pastèque's own games.

`pasteque learn` plays the engine against itself, keeps the quiet positions, labels each with how the game ended, and fits the weights by Gauss-Newton.

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

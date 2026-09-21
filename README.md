# Overview

![Logo]https://raw.githubusercontent.com/vshcherbyna/pasteque/refs/heads/foundations/pasteque.png?token=GHSAT0AAAAAAD7FJG6OKRL4CIPPBHT746NS2VQQ2KA)

Pastèque is a free UCI chess engine from Ukraine. It is not a complete chess program: it needs a UCI-compatible GUI to be used.

# History

The work on Pastèque was started in 2018, and it was never got to anything meaniningful mostly because I then switched to working on Igel. I recently found some sources on my hard drive and decided to resurrenct the project to let it least be released.

## Evaluation is stochastic on purpose

All evaluation parameters — piece values, twelve piece-square tables, phase weights, the tempo bonus and the move-ordering values — are random on purpose. One can wonder - why use random numbers as a chess evaluation - the rational behind is to build an engine that is truly having zero chess knowledge.

## Building

A C++17 compiler and a 64-bit target.

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Builds `pasteque` and the `unit` test binary; googletest is fetched by tag at configure time.
`-DPASTEQUE_UNIT_TESTS=OFF` builds the engine alone. cmake 3.16 or newer.

```
make                     # the engine, which is what OpenBench builds
make EXE=pasteque-dev    # ... under another name
make bench               # build, then run the bench
make CC=clang++          # any compiler that speaks c++17
make BTYPE=1             # sliders by pext rather than the magic multiply
```

The makefile asks the compiler about itself once and adapts: `-flto` against `-flto=auto`,
`-static` for mingw, `-march=native` only on x86. It builds the engine alone — the unit tests are
CMake's job.

## The search

- iterative deepening, fail-soft alpha-beta, quiescence at the horizon
- legal move generation at every node; magic bitboards for the sliders, or `pext`
- captures ordered MVV/LVA-style then promotions, insertion sorted, on the random piece values
- evaluation tapered between an opening and a closing set of tables
- the fifty-move rule, and a lone-minor endgame scored as a draw
- soft and hard time limits, the clock polled every 2048 nodes
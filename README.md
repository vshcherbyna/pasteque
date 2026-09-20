# pastèque

A UCI chess engine in C++17, with no dependency outside the standard library. It generates legal
moves, searches them with alpha-beta and quiescence, keeps its own clock and speaks enough UCI to
play a complete game.

Started in 2018, picked up again in 2026. The engine answers to `pasteque 0.0.0`.

## The evaluation is random on purpose

All 788 evaluation parameters — piece values, twelve piece-square tables, phase weights, the tempo
bonus and the move-ordering values — are uniform draws from the operating system's cryptographic
random source, baked into `src/judge.cpp` and `src/search.cpp`. A knight is currently worth -93
centipawns in the opening. That is not a bug.

An engine that starts from hand-set values inherits somebody else's chess, and every later
measurement is taken relative to that inheritance. This one is meant to arrive at an evaluation by
tuning into it.

- **Baked into the source, not seeded at startup**, so a tuner can move one parameter and replay
  the same game.
- **Everything structural was left alone**: mate and window sentinels, the buckets that sort
  captures against promotions, the rule that a lone minor cannot mate, and the time management.
- **One range for every term, ±256**, so no hierarchy is smuggled in through the bounds. The
  largest reachable static score is 16,384, clear of the mate window at 31,872.

It plays accordingly: 0 out of 20 against the same engine with hand-set values, every game lost by
mate, and not one illegal move in the twenty.

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

## Running

```
$ ./pasteque
pasteque 0.0.0 by Volodymyr Shcherbyna
uci
id name pasteque 0.0.0
id author Volodymyr Shcherbyna
uciok
```

| command | |
|---|---|
| `uci`, `isready`, `ucinewgame`, `quit` | |
| `position startpos \| fen <fen> [moves ...]` | every move is checked for legality |
| `go` | `wtime`/`btime`, `winc`/`binc`, `movestogo`, `movetime`, `depth`; a bare `go` searches to depth 6 |
| `bench [depth]` | |

No `setoption` options, no `stop`, no pondering.

`bench`, as a UCI command or as `pasteque bench`, searches sixteen fixed positions to depth 5:

```
11115778 nodes 5393390 nps
```

The node count is a watermark — identical on every compiler and platform, kept in `bench.nodes`,
asserted by every CI job. Anything that moves it changed the search.

## The search

- iterative deepening, fail-soft alpha-beta, quiescence at the horizon
- legal move generation at every node; magic bitboards for the sliders, or `pext`
- captures ordered MVV/LVA-style then promotions, insertion sorted, on the random piece values
- evaluation tapered between an opening and a closing set of tables
- the fifty-move rule, and a lone-minor endgame scored as a draw
- soft and hard time limits, the clock polled every 2048 nodes

Not there yet: transposition table, Zobrist hashing, repetition detection, null move, late move
reductions, aspiration windows, threads. Repetition is the one with a visible cost — the engine
cannot see a threefold coming.

### pext

Built with `_BTYPE=1`, rook and bishop lookups index their tables with a `pext` extract rather
than the magic multiply, and the magics are neither searched for at startup nor stored. Both
builds return the same moves and the same bench; only the speed differs.

It is a switch rather than a detection from `__BMI2__`: `pext` is one fast operation on Intel from
Haswell and AMD from Zen 3, and microcoded on Zen 1 and Zen 2, which report `__BMI2__` all the
same. It is x86 only — on Apple silicon the define is accepted, the sliders stay on the magic
multiply, and `-mbmi2` is never passed.

## Tests

68 googletest cases in `src/unit`, covering board state and FEN, move generation, evaluation
symmetry, search invariants, the clock, the UCI handshake and the bench. Perft runs the six
standard positions and asserts that make and unmake return the exact FEN they started from.

```
cmake --build build --config Release --target unit
./build/Release/unit          # or ./build/unit on a single-config generator
```

## Continuous integration

Eight workflows: seven build the default engine with CMake and the makefile across Linux, Windows
MSVC, Windows MinGW and macOS arm64, and the eighth builds `_BTYPE=1`. All assert the bench
watermark. Because that watermark is the same either way, the `_BTYPE=1` jobs also disassemble the
binary and count `pext` instructions — some on x86, none on arm64.

## Layout

| | |
|---|---|
| `src/bitboard.*` | bitboard primitives, LERF with A1 at 0 |
| `src/attacks.*` | attack tables, magic bitboards |
| `src/board.*` | position, FEN, make and unmake, attack and pin queries |
| `src/move.*`, `src/moves.*` | move encoding, legal move generation |
| `src/judge.*` | evaluation |
| `src/search.*` | alpha-beta, quiescence, move ordering |
| `src/clock.*` | time management |
| `src/perft.*`, `src/bench.*` | perft, the bench positions and driver |
| `src/uci.*`, `src/pasteque.cpp` | the UCI loop, `main` |
| `src/unit/` | the test suite |

## Licence

GNU General Public License v3 or later. See `LICENSE`.

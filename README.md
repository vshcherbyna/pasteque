# pastèque

A UCI chess engine in C++17, with no dependency outside the standard library.

It generates legal moves, searches them with alpha-beta and quiescence, keeps its own clock, and
speaks enough UCI to play a complete game in a GUI or against another engine. What it does not
have is an opinion about chess: every number in its evaluation was drawn at random, deliberately,
and is waiting for a tuner.

Started in 2018, picked up again in 2026. The engine answers to `pasteque 0.0.0`.

## The evaluation knows nothing about chess

All 788 evaluation parameters — the piece values, twelve piece-square tables, the phase weights,
the tempo bonus and the move-ordering values — are uniform draws from the operating system's
cryptographic random source, baked into `src/judge.cpp` and `src/search.cpp`.

A knight is currently worth -93 centipawns in the opening, a pawn 28. That is not a bug.

An engine that starts from hand-set values inherits somebody else's chess, and every later
measurement is taken relative to that inheritance. The intent here is to arrive at an evaluation
by tuning one, from a starting point that cannot be accused of having been taught anything.

Three things about the draw are worth knowing:

- **The values are baked into the source, not seeded at startup.** A tuner has to be able to move
  one parameter and replay the same game, and an engine that reseeds on every launch can be
  neither tuned nor debugged.
- **Everything structural was left alone.** Mate and window sentinels, the buckets that sort
  captures against promotions, the rule that a lone minor cannot mate, and the whole of the time
  management. Randomising those breaks the engine rather than un-teaching it.
- **The range is bounded at ±256,** one range for every term so no hierarchy between material and
  placement is smuggled in through the bounds. The largest reachable static score is 16,384,
  which keeps it clear of the mate window starting at 31,872.

It plays accordingly. Measured against the same engine carrying hand-set values, it scored 0 out
of 20 at fixed depth, every game lost by mate — and finished all twenty without an illegal move,
which was the part being tested.

## Building

Requirements: a C++17 compiler and a 64-bit target. The engine leans on 64-bit bitscan and
popcount, and there is no 32-bit fallback.

### CMake

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Builds `pasteque` and the `unit` test binary. googletest is fetched at configure time by tag, so
the first configure needs network access; `-DPASTEQUE_UNIT_TESTS=OFF` builds the engine alone.
cmake 3.16 or newer.

`-D_BTYPE=1` takes the sliders through `pext` rather than the magic multiply. See below.

### Make

```
make                     # the engine, which is what OpenBench builds
make EXE=pasteque-dev    # ... under another name
make bench               # build, then run the bench
make CC=clang++          # any compiler that speaks c++17
make BTYPE=1             # sliders by pext rather than the magic multiply
```

The makefile asks the compiler about itself once and adapts: `-flto` for clang against
`-flto=auto` for gcc, `-static` for mingw so the binary travels, `-march=native -mpopcnt` only on
x86. It builds the engine and nothing else — the unit tests are CMake's job.

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
| `go` | understands `wtime`/`btime`, `winc`/`binc`, `movestogo`, `movetime` and `depth`; a bare `go` searches to depth 6 |
| `bench [depth]` | |

There are no `setoption` options, no `stop`, and no pondering.

### bench

`bench`, as a UCI command or as `pasteque bench`, searches sixteen fixed positions to depth 5 and
prints a node total:

```
11115778 nodes 5393390 nps
```

That total is a watermark. It is identical on every compiler and platform, it is kept in
`bench.nodes`, and every CI job asserts the two agree. Anything that moves it changed the search,
and `bench.nodes` is expected to move in the same commit.

## pext

Built with `_BTYPE=1`, rook and bishop lookups index their tables with a `pext` extract instead
of the magic multiply and shift, and the magics are neither searched for at startup nor stored.
The attack tables are the same either way, so the two builds return the same moves and the same
bench; only the speed differs.

It is a switch rather than something detected from `__BMI2__`, because owning the instruction and
wanting it are different questions. `pext` is a single fast operation on Intel from Haswell and on
AMD from Zen 3, and microcoded — far slower than the multiply it replaces — on Zen 1 and Zen 2,
which report `__BMI2__` all the same.

`pext` is x86 only, and Apple silicon has no equivalent instruction. Asking for `_BTYPE=1` on a
non-x86 target is accepted rather than refused: the sliders stay on the magic multiply, and the
build never sees `-mbmi2`. The same command therefore works on every host the engine builds on.

## What the search does

- iterative deepening, fail-soft alpha-beta, quiescence at the horizon
- legal move generation at every node; magic bitboards for the sliders, or `pext`
- captures ordered MVV/LVA-style, then promotions, insertion sorted — on the random piece values
- evaluation tapered between an opening and a closing set of tables
- the fifty-move rule, and a lone-minor endgame scored as a draw
- soft and hard time limits, with the clock polled every 2048 nodes

Not there yet: transposition table, Zobrist hashing, repetition detection, null move, late move
reductions, aspiration windows, threads. Repetition is the one with a visible cost — the engine
cannot see a threefold coming and will walk into one.

## Tests

68 googletest cases in `src/unit`, built as the `unit` target:

```
cmake --build build --config Release --target unit
./build/Release/unit          # or ./build/unit on a single-config generator
```

They cover board state and FEN round trips, move encoding, move generation including castling,
en passant and promotions, evaluation symmetry and phase, search invariants that hold whatever
the tables say, the clock, the UCI handshake and the bench. Perft runs the six standard positions
at shallow depth and asserts that make and unmake return the position to the exact FEN it started
from; the deep runs live outside the suite so it stays quick.

## Continuous integration

Eight workflows. Seven build the default engine — CMake on Linux, Windows MSVC, Windows MinGW and
macOS arm64, and the makefile on Linux, Windows MinGW and macOS arm64 — and the eighth builds
`_BTYPE=1`. Every one asserts the bench watermark. The macOS jobs also assert the binary is arm64
and nothing else; the MinGW job checks what it links against, which is what `-static` is there to
guarantee.

The `_BTYPE=1` workflow asserts more than that the build succeeds, because the bench watermark is
the same either way and so cannot tell the two apart: it disassembles the binary and counts `pext`
instructions. On x86 there must be some, and on macOS arm64 there must be none, which is what
proves the fallback rather than assuming it.

## Layout

| | |
|---|---|
| `src/bitboard.*` | bitboard primitives, squares, LERF layout with A1 at 0 |
| `src/attacks.*` | attack tables and the magic bitboards for sliders |
| `src/board.*` | position, FEN, make and unmake, attack and pin queries |
| `src/move.*` | move encoding |
| `src/moves.*` | legal move generation |
| `src/judge.*` | evaluation |
| `src/search.*` | alpha-beta, quiescence, move ordering |
| `src/clock.*` | time management |
| `src/perft.*` | perft |
| `src/bench.*` | the bench positions and driver |
| `src/uci.*` | the UCI loop |
| `src/pasteque.cpp` | `main` |
| `src/unit/` | the test suite |

## Licence

GNU General Public License v3 or later. See `LICENSE`.

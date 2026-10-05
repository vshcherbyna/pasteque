# Overview

<img src="pasteque.png" alt="Logo" width="200">

Pastèque is a free UCI chess engine from Ukraine. It is not a complete chess program: it needs a UCI-compatible GUI to be used.

# History

Work on Pastèque started in 2018, and it never amounted to much — mainly because I moved on to Igel soon afterwards. I recently found the old sources on my hard drive and decided to resurrect the project, so that it would at least see a release.

# Evaluation starts stochastic on purpose

All evaluation parameters — piece values, twelve piece-square tables, phase weights, the tempo bonus and the move-ordering values — were drawn at random. The intent is to begin from an engine that knows nothing of chess beyond the legal moves.

The piece values and the tempo are no longer random; they were estimated by the procedure below. The piece-square tables are flat, and the phase and move-ordering values are still as drawn.

# Learning the evaluation

Constraint: no parameter may derive from another engine, from labelled positions produced by a stronger program, or from published values. The admissible sources are the rules of chess and the engine's own play.

## Model

`Judge::evaluate` is a sum of per-piece, per-square weights, scaled by a phase that those weights do not influence. The evaluation is therefore **linear in its own parameters**. With the square tables flat it reduces to

```
eval(p) = Σ_t  w_t · ( n_t(mover) − n_t(opponent) )  +  w_0
```

over the five non-king piece types. Two parameters are unidentifiable: the king cancels between the sides, and the overall scale cannot affect a move choice, so it is fixed by convention at `w_pawn = 100`.

## Estimation

Each retained position carries the game's outcome `r ∈ {0, ½, 1}` from the mover's view. Identical feature vectors are aggregated, which is exact for both gradient and curvature, and the weights minimise

```
Σ_i  n_i · ( r̄_i − σ(eval_i) )²
```

by Gauss-Newton over a 6×6 normal-equation system, forty rounds from a zero start. Squared error under a logistic link is not globally convex, but at six parameters the iteration is stable and reproducible; there is no learning rate.

## Data

- Openings: uniform random legal playout of 8 plies from the initial position, deduplicated by position key. The search is deterministic, so the opening set is the only source of variety.
- Games: engine against itself under a fixed node budget, making results independent of machine load and the data reproducible.
- Termination: strictly by rule — mate, stalemate, fifty moves, threefold repetition, insufficient material. Games exceeding a ply limit are discarded rather than labelled.
- Retained positions: not in check, played move neither capture nor promotion, material sufficient. Capped per game, sampled evenly, so game length does not weight the sample.
- Labels within a game are perfectly correlated, so the effective sample size is the number of games, not positions.

## Results

Nested subsets of 10,000 games played under random evaluation:

| games | knight | bishop | rook | queen |
|---|---|---|---|---|
| 1,250 | 145 | 173 | 275 | 317 |
| 2,500 | 153 | 179 | 294 | 320 |
| 5,000 | 150 | 176 | 296 | 326 |
| 10,000 | 141 | 178 | 303 | 337 |

Bootstrap standard errors over games (12 resamples, full set): knight ±6.5, bishop ±7.7, rook ±9.0, queen ±10.1. Against commonly published values (305 / 325 / 500 / 925) the deviations are 15 to 57 standard errors. The estimates are precise and biased; enlarging the sample reduces the error bars and not the deviation.

Refitting from 300 games played by the engine carrying the first estimates:

| | knight | bishop | rook | queen |
|---|---|---|---|---|
| 10,000 games, random evaluation | 141 | 178 | 303 | 337 |
| 300 games, one round later | 194 | 232 | 364 | 853 |

Material predicts the outcome only as well as the player converts it. Under random evaluation a rook advantage and a queen advantage both map to near-certain wins, the logistic link saturates, and the large weights are not separately identified. The bias is reduced by strengthening the generator, not by enlarging the sample.

## Limitation

Outcome labels are bounded, so advantages beyond the saturation point are indistinguishable and the largest weights remain compressed. Search scores are unbounded and remove the ceiling; fitting against them is the next step.

## `learn`

```
pasteque learn [games] [nodes] [seed]
```

Generates openings, plays them out, retains and aggregates positions, fits, and prints the result for `judge.cpp`. Single-threaded; games are independent and aggregation is a sum, so parallelism would be exact.

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
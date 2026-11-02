# StackRisk

StackRisk is an in-progress C++20 Texas Hold'em equity and risk simulator.
This repository is being built in tested milestones; it is not a finished
poker or bankroll-analysis product.

## Current milestone

**Implemented:** card parsing and deck generation; five- and seven-card hand
evaluation for all standard poker categories, including ace-low straights;
seeded Monte Carlo deal simulation; pot-share equity and a 95% normal-
approximation confidence interval; a command-line interface; deterministic
unit tests for categories, tie-breakers, split pots, and invalid input; and
parallel execution that produces the same results as a single worker for a
given seed.

**Not implemented yet:** bankroll-risk analysis. The current runtime
measurement is specific to the machine and scenario recorded below. The
confidence interval is a sampling estimate, not a guarantee about real-world
outcomes.

## Build and test

Requires CMake 3.20+ and a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The tests can also be compiled directly with Clang:

```sh
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -I include \
  src/card.cpp src/evaluator.cpp src/simulation.cpp tests/test_main.cpp \
  -o /tmp/stackrisk-tests
/tmp/stackrisk-tests
```

Cards use two-character notation: ranks `2`-`9`, `T`, `J`, `Q`, `K`, `A` and
suits `c`, `d`, `h`, `s` (for example, `As` is the ace of spades).
Run `./build/stackrisk-cli --help` for a compact example and option overview.

## CLI example

```sh
./build/stackrisk-cli \
  --hero As Kd \
  --board 2c,3s,4h \
  --opponents 2 \
  --trials 100000 \
  --seed 42 \
  --workers 4
```

With the same inputs and seed, the simulation is repeatable. Equity counts a
win as 1, a loss as 0, and a tied pot as an equal share. The reported win/tie/
loss counts categorize outcomes; the equity calculation uses each outcome's
share of the pot. The CLI reports win/tie/loss counts and their percentages,
and echoes the hole cards, known board, opponent count, seed, and worker count
so each result is tied to the setup that produced it.

Across three local runs, 100,000 trials with two opponents and a three-card
board took about 0.36 seconds with one worker and 0.10-0.14 seconds with four
workers. This is a local smoke benchmark, not a controlled performance claim;
results vary by machine, compiler, and hand configuration.

# StackRisk

StackRisk is an in-progress C++20 Texas Hold'em equity and risk simulator.
This repository is being built in tested milestones; it is not a finished
poker or bankroll-analysis product.

## Current milestone

**Implemented:** card parsing and deck generation; five- and seven-card hand
evaluation for all standard poker categories, including ace-low straights;
deterministic unit tests for categories, tie-breakers, and invalid hands.

**Not implemented yet:** Monte Carlo deal generation, equity estimates,
confidence intervals, bankroll-risk analysis, and parallel simulation.

## Build and test

Requires CMake 3.20+ and a C++20 compiler.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The evaluator tests can also be compiled directly with Clang:

```sh
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -I include \
  src/card.cpp src/evaluator.cpp tests/test_main.cpp \
  -o /tmp/stackrisk-tests
/tmp/stackrisk-tests
```

Cards use two-character notation: ranks `2`-`9`, `T`, `J`, `Q`, `K`, `A` and
suits `c`, `d`, `h`, `s` (for example, `As` is the ace of spades).

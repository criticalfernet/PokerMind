# Poker Bot

A command-line poker game in C++23 with a built-in advisor. It tells you your chance of winning and whether calling or raising is worth it.

## What it does

The game plays Texas Hold'em (Omaha is supported in the evaluator) with any number of players, a small blind, betting rounds, side pots and showdowns.

Before each move, the advisor prints your win probability, the expected return of calling, and a suggested raise (or "don't raise").

## How it works

Win probability comes from a Monte Carlo simulation. It deals out the rest of the board and the opponents' hands many times, counts how often you win, and runs across multiple threads.

Opponents don't have to be random. Each one can be given a weight between 0 and 1. A higher weight makes the simulation give them stronger hands. All possible hands are sorted from best to worst on the current board, and opponents are drawn from that list with a bias set by their weight.

Expected return uses the win probability with the pot and the amount to call. Calling is worth `p * pot - (1 - p) * toCall`. The net value subtracts the chips you already put in this hand. The break-even figure is the pot odds, which is the win probability you need for a call to be profitable.

The raise suggestion tries a range of raise sizes and picks the best one. It assumes bigger raises make opponents fold more and leave them with stronger hands when they call. It only suggests a raise if it beats calling.

## Files

`card.hpp`, `deck.hpp` and `hand.hpp` hold the basic types.
`evaluator.hpp` ranks hands for Texas Hold'em and Omaha.
`bestHands.hpp` lists and sorts every possible opponent hand.
`probability.hpp` has the Monte Carlo simulation and the weights.
`ev.hpp` has expected return and the raise advice.
`simulator.hpp` runs the game.
`check.hpp` has test runs for the weights and the hand sorting.
`game.cpp` is the entry point.

## Build and run

```
make run
```

In `game.cpp`, `main` can call `playGame()` for the full game, `checkForWeightsMonteCarlo()` to compare weight setups, or `checkForSortedHands()` to print the best hands on a board.

## Known limits

The raise advice is a simple model, not a solved strategy. Fold chances are guessed, not measured. Opponent weights are set by hand and are not learned from their betting.
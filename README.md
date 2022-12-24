## About

This fork of [Connect 4 Game Solver](https://github.com/PascalPons/connect4) is used to create a JSON dataset
with perfect move scores and position evaluations.

Besides adding rapidjson library, all my changes are in main.cpp file. If you want to modify this
program, that's probably what you should be looking at.

## How to run

```
make
./c4solver
```

With numEpisodes = 100 000, it generates ~2.3 milion samples and takes ~400 MB of memory.

## Dataset

There are 3 data fields.

**position** - array of size 42. On a 6x7 board, index 0 would be mapped to top left corner and index 41
to bottom right. Values of the array are:
- 1 for PLAYER_1
- 2 for PLAYER_2
- 0 if the square is empty.

**winner** - with perfect play from both sides, who will win the game:
- 1 if PLAYER_1
- 2 if PLAYER_2
- 0 if draw

**action_probabilities** - how good each action is, normalized such that values add up to 1.

For example, if actions 1, 3 and 4 all lead to victory in the fewest number of moves, action probabilities
would be:
```
0.33, 0, 0.33, 0.33, 0, 0, 0
```
For losing/drawing positions, actions are choosen such that loss/draw will happen in the greatest number of moves.

This might not be the best way to create action probabilities data. If one action leads to a victory in 10 moves 
and another in 11 moves, that's not such a big difference. It seems extreme to give a score of 1 to the best action and 0 to
the slightly worse action.

## Self play

Data is generated through self play. 30% (epsilon) of the actions are selected randomly, other
actions are selected greedily. This needs to be improved.
 
---

# Connect 4 Game Solver

This C++ source code is published under AGPL v3 license.

Read the associated [step by step tutorial to build a perfect Connect 4 AI](http://blog.gamesolver.org) for explanations.

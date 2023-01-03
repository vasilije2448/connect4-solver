## About

This fork of [Connect 4 Game Solver](https://github.com/PascalPons/connect4) is used to create a JSON dataset
with perfect move scores and position evaluations.

Besides adding rapidjson library, almost all my changes are in main.cpp file. If you want to modify this
program, that's probably what you should be looking at.

## How to run

```
make
./c4solver
```

With NUM_EPISODES = 100 000, it generates ~2.3 million samples and takes ~400 MB of memory.

Inside main.cpp, change NUM_CPU if you want to run in parallel.

## Dataset

There are 3 data fields.

**position** - Array of size 42. On a 6x7 board, index 0 would be mapped to top left corner and index 41
to bottom right. Values of the array are:
- 1 for PLAYER_1
- 2 for PLAYER_2
- 0 if the square is empty.

**winner** - With perfect play from both sides, who will win the game:
- 1 if PLAYER_1
- 2 if PLAYER_2
- 0 if draw

**scores** - Winning actions have a positive score and losing actions have a negative score. 0 is used for drawing actions. Absolute value of the score tells you how quickly the result will happen, assuming perfect play from both sides. Score of 1 means that the current player can end the game with his last action, 2 means with second last etc. -1000 is used for illegal actions.

## Self play

Data is generated through self play. Actions are selected based on their scores. If scores are:

```
-1000, -3, 2, 0, 0, 5, 5
```

Probability of selecting each action will be:

```
0.00, 0.03, 0.18, 0.12, 0.12, 0.27, 0.27
```

---

# Connect 4 Game Solver

This C++ source code is published under AGPL v3 license.

Read the associated [step by step tutorial to build a perfect Connect 4 AI](http://blog.gamesolver.org) for explanations.

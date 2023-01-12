## About

This fork of [Connect 4 Game Solver](https://github.com/PascalPons/connect4) is used to:

1) Create a JSON dataset with perfect move scores and position evaluations.

2) Provide a python wrapper for the C++ Solver with pybind11.

While the original code was made to work with different board sizes, some stuff is now hard coded
to work only with the most common 6 rows x 7 columns board size.

## 1. Dataset generation

### How to run

```
make
./c4solver
```

With NUM_EPISODES = 100 000, it generates ~2.3 million samples and takes ~400 MB of memory.

Inside main.cpp, change NUM_CPU if you want to run in parallel.

### Dataset

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

### Self play

Data is generated through self play. Actions are selected based on their scores. If scores are:

```
-1000, -3, 2, 0, 0, 5, 5
```

Probability of selecting each action will be:

```
0.00, 0.03, 0.18, 0.12, 0.12, 0.27, 0.27
```

## 2. Python wrapper for the Solver

### How to run

After cloning the repository, inside pybind11 run:

```
git submodule update --init
```

Then, from connect4-solver (project root):

```
cmake -S . -B python_build
cmake --build python_build
```

This will generate a shared library (shared object) .so file inside python_build directory. It
will be named "connect4_solver.cpython-37m-x86_64-linux-gnu.so" or similar, depending on your
python version and OS. This file and an opening book (7x6.book) is all you need to use the
PerfectSolver inside your python programs.

Example code:

```
import imp
connect4_solver = imp.load_dynamic('connect4_solver', 'python_build/connect4_solver.cpython-37m-x86_64-linux-gnu.so')
from connect4_solver import PerfectSolver

ps = PerfectSolver('7x6.book')
kaggle_state = [0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 2, 0,
                0, 0, 0, 0, 0, 1, 0,
                0, 0, 0, 0, 2, 1, 0]
print(ps.computeScores(kaggle_state))
```

### Possible source of errors

If your shared library is compiled for a different python version than the one you're using, your code might
not work. Inside CMakeLists.txt, you can change the lines:

```
set (PYTHONVERSION "3.7.9")
set (PYBIND11_PYTHON_VERSION "3.7.9")
```

but that might not be enough. For example, if you don't have python 3.7 installed on your OS, but
have 3.8, cmake will ignore your config and use that instead. You could install the specified version on your OS, or you could run cmake commands from within the conda environment. Just make sure to remove the previously generated python_build directory first.

---

# Connect 4 Game Solver

This C++ source code is published under AGPL v3 license.

Read the associated [step by step tutorial to build a perfect Connect 4 AI](http://blog.gamesolver.org) for explanations.

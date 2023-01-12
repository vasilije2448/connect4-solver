#include <iostream>
#include <cmath>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "Solver.hpp"

using namespace GameSolver::Connect4;

int getCurrentPlayer(std::vector<int> kaggleState) {
	int player1Stones = std::count(kaggleState.begin(), kaggleState.end(), 1);
	int player2Stones = std::count(kaggleState.begin(), kaggleState.end(), 2);
	int currentPlayer = 1;
	if(player2Stones < player1Stones) {
		currentPlayer = 2;
	}
	return currentPlayer;
}

Position createPositionFromKaggleState(std::vector<int> kaggleState) {
	Position P;
	Position::position_t currentPosition = 0;
	Position::position_t mask = 0;
	int moves = 0;
	int currentPlayer = getCurrentPlayer(kaggleState);
	int currentStone;
	Position::position_t currentBitMask;
	Position::position_t numberOne = 1; // int is 32-bit, which isn't enough for mask
	for(int i = 0; i < kaggleState.size(); i++) {
		currentStone = kaggleState[i];
		if(currentStone == 0) continue;
		currentBitMask = numberOne << (int)((i % 7) * 7 + (5 - std::floor(i / 7)));
		mask |= currentBitMask;
		if(currentStone == currentPlayer) {
			currentPosition |= currentBitMask;
		}
		moves++;
	}
	P.setCurrentPosition(currentPosition);
	P.setMask(mask);
	P.setMoves(moves);
	return P;
}

class PerfectSolver {
  private:
    Solver solver;
    bool weak = false;

  public:
  PerfectSolver(std::string pathToBook) {
    solver.loadBook(pathToBook);
  }

  std::vector<int> computeScores(std::vector<int> kaggleState) {
    Position P = createPositionFromKaggleState(kaggleState);
    return solver.analyze(P, weak);
  }
};

namespace py = pybind11;

PYBIND11_MODULE(connect4_solver, module_handle) {
  py::class_<PerfectSolver>(
			module_handle, "PerfectSolver"
			).def(py::init<std::string>())
      .def("computeScores", &PerfectSolver::computeScores)
  ;
}


/*
 * This file is part of Connect4 Game Solver <http://connect4.gamesolver.org>
 * Copyright (C) 2017-2019 Pascal Pons <contact@gamesolver.org>
 *
 * Connect4 Game Solver is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Connect4 Game Solver is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Connect4 Game Solver. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Solver.hpp"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include <iostream>
#include <algorithm>
#include <valarray>
#include <string>
#include <random>
#include <cmath>
#include <climits>
#include <omp.h>
#include <chrono>


using namespace GameSolver::Connect4;

const int NUM_ROWS = 6;
const int NUM_COLUMNS = 7;

const int NUM_CPU = 1;
const std::string DATASET_NAME = "dataset.json";
const int NUM_EPISODES = 100000;

std::valarray<float> computeActionProbabilities(std::vector<int> scores) {
  std::valarray<float> actionProbabilities(NUM_COLUMNS);
  int maxScore = *max_element(scores.begin(), scores.end());
  for(int i = 0; i < (int)scores.size(); i++) {
    if(scores[i] == maxScore) {
      actionProbabilities[i] = 1;
    } else {
      actionProbabilities[i] = 0;
    }
  }
  actionProbabilities /= actionProbabilities.sum();

 return actionProbabilities;
}

int computeAction(std::vector<int> scores, std::mt19937& generator) {
  int action;
  std::vector<int> actions{1, 2, 3, 4, 5, 6, 7};
  std::valarray<float> actionProbabilities(NUM_COLUMNS);
  int minLegalScore = INT_MAX;
  for(int i = 0; i < NUM_COLUMNS; i++) {
    if(scores[i] == -1000) { // illegal action
      continue;
    } else if(scores[i] < minLegalScore) {
      minLegalScore = scores[i];
    }
  }

  for(int i = 0; i < NUM_COLUMNS; i++) {
    if(scores[i] == -1000) { // illegal action
      actionProbabilities[i] = 0;
    } else {
      actionProbabilities[i] = std::abs(minLegalScore) + scores[i] + 1; 
    }
  }
  std::discrete_distribution<std::size_t> d{std::begin(actionProbabilities), std::end(actionProbabilities)};
  action = actions[d(generator)];
  return action;
}


/*
 * Finds the first empty square for the given column and plays the move.
 * Assumes that the move is legal.
 *
 * @param currentPosition vector of size 42. On a 6x7 board, top left maps to index 0, bottom right to 41.
 * @param action Value of the column. Not 0 indexed, minimum value is 1.
 * @param currentPlayerId 1 for player 1, 2 for player 2.
 * @return board with the action played.
 */
std::vector<int> modifyCurrentPosition(std::vector<int>& currentPosition, int action, int currentPlayerID) {
  int h = NUM_ROWS * NUM_COLUMNS - 1; // helper variable for indexing
  for(int i = 0; i < NUM_COLUMNS; i++) {
    int idx = h - (NUM_COLUMNS - action + i * NUM_COLUMNS);
    if(currentPosition[idx] == 0) {
      currentPosition[idx] = currentPlayerID;
      break;
    }
  }
  return currentPosition;
}

void saveDataset(std::vector<std::vector<int>>& positionsVector,
    std::vector<std::valarray<float>>& actionProbabilitiesVector,
    std::vector<int>& winnersVector,
    std::string datasetName) {
  rapidjson::StringBuffer stringBuffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);

  for(int i = 0; i < (int)positionsVector.size(); i++) {
    writer.StartObject();
    writer.Key("position");
    writer.StartArray();
    for(int j = 0; j < NUM_ROWS*NUM_COLUMNS; j++) {
      writer.Uint(positionsVector[i][j]);
    }
    writer.EndArray();
    writer.Key("winner");
    writer.Uint(winnersVector[i]);
    writer.Key("action_probabilities");
    writer.StartArray();
    for(int j = 0; j < NUM_COLUMNS; j++) {
      writer.Double(actionProbabilitiesVector[i][j]);
    }
    writer.EndArray();
    writer.EndObject();
    stringBuffer.Put('\n');
  }
  std::ofstream file(datasetName);
  file << stringBuffer.GetString();
}

int main() {

  std::vector<std::vector<std::vector<int>>> threadToPositionsVector(NUM_CPU);
  std::vector<std::vector<std::valarray<float>>> threadToActionProbabilitiesVector(NUM_CPU);
  std::vector<std::vector<int>> threadToWinnersVector(NUM_CPU);
  omp_set_num_threads(NUM_CPU);
  omp_set_dynamic(0);
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
  #pragma omp parallel
  {
    Solver solver;
    bool weak = false;
    int numEpisodesPerCPU = std::floor(NUM_EPISODES / NUM_CPU);
    std::string opening_book = "7x6.book";
    solver.loadBook(opening_book);
    int action;

    std::vector<std::vector<int>> positionsVector;
    std::vector<std::valarray<float>> actionProbabilitiesVector;
    std::vector<int> winnersVector; // 0 for draw, 1 for p1, 2 for p2
    for(int episodeNum = 0; episodeNum < numEpisodesPerCPU; episodeNum++) {
      Position P;
      int currentPlayerID = 1; // used only for dataset. Domain = {1, 2}
      std::vector<int> currentPosition(6*7); // used only for dataset. Solver uses Position class.
      bool gameOver = false;
      while(!gameOver) {
        std::vector<int> scores = solver.analyze(P, weak);
        std::valarray<float> actionProbabilities = computeActionProbabilities(scores);
        positionsVector.push_back(std::vector<int>(currentPosition)); // store _copy_ of the current position
        actionProbabilitiesVector.push_back(actionProbabilities);
        int maxScore = *max_element(scores.begin(), scores.end());
        int winner;
        if(maxScore == 0) { // draw
          winner = 0;
        } else if(maxScore > 0) {
          winner = currentPlayerID;
        } else {
          winner = 3 - currentPlayerID;
        }
        winnersVector.push_back(winner);

        std::mt19937 gen(std::random_device{}());
        action = computeAction(scores, gen);
        currentPosition = modifyCurrentPosition(currentPosition, action, currentPlayerID);
        if(P.isWinningMove(action - 1) || P.nbMoves() == NUM_ROWS*NUM_COLUMNS-1) {
          gameOver = true;
        }

        P.play(std::to_string(action));
        currentPlayerID = 3 - currentPlayerID;
      }
    }
    threadToPositionsVector[omp_get_thread_num()] = positionsVector;
    threadToActionProbabilitiesVector[omp_get_thread_num()] = actionProbabilitiesVector;
    threadToWinnersVector[omp_get_thread_num()] = winnersVector;
  }

  // Concat vectors from all threads
  std::vector<std::vector<int>> positionsVector;
  std::vector<std::valarray<float>> actionProbabilitiesVector;
  std::vector<int> winnersVector;

  for(int i = 0; i < NUM_CPU; i++) {
    std::vector<std::vector<int>> pv = threadToPositionsVector[i];
    positionsVector.insert(
        positionsVector.end(),
        std::make_move_iterator(pv.begin()),
        std::make_move_iterator(pv.end())
      );
    std::vector<std::valarray<float>> apv = threadToActionProbabilitiesVector[i];
    actionProbabilitiesVector.insert(
        actionProbabilitiesVector.end(),
        std::make_move_iterator(apv.begin()),
        std::make_move_iterator(apv.end())
      );
    std::vector<int> wv = threadToWinnersVector[i];
    winnersVector.insert(
        winnersVector.end(),
        std::make_move_iterator(wv.begin()),
        std::make_move_iterator(wv.end())
      );
  }


  std::cout << "Positions vector size: " << positionsVector.size() << "\n";
  std::cout << "Action probabilities vector size: " << actionProbabilitiesVector.size() << "\n";
  std::cout << "Winners vector size: " << winnersVector.size() << "\n";
  std::cout << "Saving dataset\n";
  saveDataset(positionsVector, actionProbabilitiesVector, winnersVector, DATASET_NAME);

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::cout << "Execution time: " << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() << " [s]" << std::endl;
}

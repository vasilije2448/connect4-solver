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


using namespace GameSolver::Connect4;

int NUM_ROWS = 6;
int NUM_COLUMNS = 7;

std::valarray<float> computeActionProbabilities(std::vector<int> scores) {
  std::valarray<float> actionProbabilities(7);
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

int computeAction(std::vector<int> scores, std::valarray<float> actionProbabilities, float epsilon, std::mt19937& generator) {
  int action;
  std::vector<int> actions{1, 2, 3, 4, 5, 6, 7};
  std::uniform_real_distribution<> d1(0, 1);
  float randomVal = d1(generator);
  if(randomVal < epsilon) { // select random legal action
    std::vector<float> randomActionProbs(NUM_COLUMNS);
    for(int i = 0; i < (int)randomActionProbs.size(); i++) {
      if(scores[i] == -1000) { // illegal action
        randomActionProbs[i] = 0;
      } else {
        randomActionProbs[i] = 1;
      }
    }
    // no need to normalize randomActionProbs, discrete_distribution does it
    std::discrete_distribution<std::size_t> d2{std::begin(randomActionProbs), std::end(randomActionProbs)};
    action = actions[d2(generator)];
  } else { // select one of the best actions
    std::discrete_distribution<std::size_t> d2{std::begin(actionProbabilities), std::end(actionProbabilities)};
    action = actions[d2(generator)];
  }

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
  Solver solver;
  bool weak = false;
  int numEpisodes = 100000;
  std::string datasetName = "dataset.json";
  std::string opening_book = "7x6.book";

  solver.loadBook(opening_book);

  int action;
  float epsilon = 0.3; // % of random moves. This should be improved by better ways of exploration.

  std::vector<std::vector<int>> positionsVector;
  // Action probabilities:
  // For a given position, how good is each action (normalized such that values add up to 1).
  // For example, if actions 1, 3 and 4 all lead to victory in the fewest number of moves, action probabilities
  // would be: 0.33, 0, 0.33, 0.33, 0, 0, 0.
  // For losing/drawing positions, actions are choosen such that loss/draw will happen in the greatest
  // number of moves.
  // 
  // This might not be the best way to create action probabilities data. 
  // If one action will lead to a victory in 10 moves and another in 11 moves, that's not such a big
  // difference. It seems extreme to give a score of 1 to the best action and 0 to the slightly worse
  // action.
  std::vector<std::valarray<float>> actionProbabilitiesVector;
  std::vector<int> winnersVector; // 0 for draw, 1 for p1, 2 for p2
  for(int episodeNum = 0; episodeNum < numEpisodes; episodeNum++) {
    std::cout << "Episode number: " << episodeNum << " / " << numEpisodes << "\n";
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
      action = computeAction(scores, actionProbabilities, epsilon, gen);
      currentPosition = modifyCurrentPosition(currentPosition, action, currentPlayerID);
      if(P.isWinningMove(action - 1) || P.nbMoves() == NUM_ROWS*NUM_COLUMNS-1) {
        gameOver = true;
      }

      P.play(std::to_string(action));
      currentPlayerID = 3 - currentPlayerID;
    }
  }

  std::cout << "Positions vector size: " << positionsVector.size() << "\n";
  std::cout << "Action probabilities vector size: " << actionProbabilitiesVector.size() << "\n";
  std::cout << "Winners vector size: " << winnersVector.size() << "\n";
  std::cout << "Saving dataset\n";
  saveDataset(positionsVector, actionProbabilitiesVector, winnersVector, datasetName);
}



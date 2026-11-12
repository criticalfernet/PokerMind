#include <iostream>
#include <string>
#include <limits>
#include <memory>

#include "deck.hpp"
#include "hand.hpp"
#include "evaluator.hpp"
#include "probability.hpp"
#include "simulator.hpp"
#include "bestHands.hpp"
#include "check.hpp"
#include "humanPolicy.hpp"

using namespace std;

void clearScreen() {
  cout << "\033[2J\033[H";
  cout.flush();
}

void waitForEnter(const string& message) {
    cout << "\n" << message << " (press Enter)";
    cin.get();
}

int playGame() {
  int numPlayers;
  cout << "Enter No. of players: ";
  cin >> numPlayers;

  int var = 0;
  // cout << "Enter Variation (0 for texas and 1 for Omaha): ";
  // cin >> var;

  Variant variant = (var == 0) ? Variant::TexasHoldem : Variant::Omaha;
  Simulator sim(variant);
  for(int i = 0; i < numPlayers; i++) {
    sim.addPlayer(500, std::make_unique<HumanPolicy>());
  }

  cout << "\nGame: " << (sim.getVariant() == Variant::TexasHoldem ? "Texas Hold'em" : "Omaha") << '\n';

  for(int i = 0; i <5; i++){
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    waitForEnter("Start Hand. ");
    clearScreen();
    sim.playHand();
  }

  return 0;
}

int main() {
  clearScreen();

  // return checkForSortedHands();
  // return checkForWeightsMonteCarlo();
  return playGame();
}
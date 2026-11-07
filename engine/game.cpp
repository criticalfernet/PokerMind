#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <format>
#include <limits>

#include "deck.hpp"
#include "hand.hpp"
#include "evaluator.hpp"
#include "probability.hpp"
#include "simulator.hpp"

using namespace std;

void clearScreen() {
  cout << "\033[2J\033[H";
  cout.flush();
}


int main() {
  clearScreen();
  int numPlayers;
  cout << "Enter No. of players: ";
  cin >> numPlayers;

  int var;
  cout << "Enter Variation (0 for texas and 1 for Omaha): ";
  cin >> var;

  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  Variant variant = (var == 0) ? Variant::TexasHoldem : Variant::Omaha;

  Simulator sim(variant);
  for(int i = 0; i < numPlayers; i++) {
    sim.addPlayer(500);
  }

  cout << "\nGame: " << (sim.getVariant() == Variant::TexasHoldem ? "Texas Hold'em" : "Omaha") << '\n';

  for(int i = 0; i <5; i++){
    sim.playHand();
  }

  return 0;
}
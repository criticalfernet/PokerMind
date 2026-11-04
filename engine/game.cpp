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

using namespace std;

void clearScreen() {
  cout << "\033[2J\033[H";
  cout.flush();
}

void waitForEnter(const string& message) {
    cout << "\n" << message << " (press Enter)";
    cin.get();
}

void revealCards(Deck& deck, vector<Hand>& hands, int count) {
  for (int i = 0; i < count; i++) {
    Card comm = deck.draw();

    for (auto& hand : hands) {
      hand.addCommCard(comm);
    }
  }
}

const string getCategoryName(HandCategory category) {
  switch (category) {
    case HandCategory::None:           return "None";
    case HandCategory::HighCard:       return "High Card";
    case HandCategory::OnePair:        return "One Pair";
    case HandCategory::TwoPair:        return "Two Pair";
    case HandCategory::ThreeOfAKind:   return "Three of a Kind";
    case HandCategory::Straight:       return "Straight";
    case HandCategory::Flush:          return "Flush";
    case HandCategory::FullHouse:      return "Full House";
    case HandCategory::FourOfAKind:    return "Four of a Kind";
    case HandCategory::StraightFlush:  return "Straight Flush";
    case HandCategory::RoyalFlush:     return "Royal Flush";
  }
  return "Unknown";
}

pair<HandRank,vector<int>> getWinners(vector<HandRank>& ranks, const vector<Hand>& hands) {
  HandRank best;
  bool found = false;
  for (size_t i = 0; i < hands.size(); i++) {
    if (hands[i].getFolded()) continue;

    if (!found || best < ranks[i]) {
      best = ranks[i];
      found = true;
    }
  }

  if (!found) {
    throw runtime_error("No eligible players remaining");
  }

  vector<int> winners;
  for (size_t i = 0; i < hands.size(); i++) {
    if (!hands[i].getFolded() && ranks[i] == best) {
      winners.push_back(static_cast<int>(i + 1));
    }
  }

  return {best, winners};
}

void printCards(string info, vector<Card> cards, bool folded, double probability) {
  if (folded) cout << "\033[2m";
  cout << info;
  for(Card c : cards){
    cout << " " << c;
  }
  cout << "  ";
  cout << format("({:.2f}%)", probability*100);
  cout << '\n';
  if (folded) cout << "\033[0m";
}

void printTable(const string& stage, vector<Hand>& hands){
  clearScreen();
  auto probabilities = monteCarlo(hands, 10000);
  cout << "\n========== " << stage << " ==========\n";

  auto community = hands[0].getCommCards();

  cout << "Community Cards:";
  for (const auto& card : community) {
    cout << " " << card;
  }
  cout << "\n\n";

  for (size_t i = 0; i < hands.size(); i++) {
      string info = format("Player {}:", i + 1);
      printCards(info, hands[i].getHoleCards(), hands[i].getFolded(), probabilities[i]);
  }

  cout << "=============================\n";
}

void printStats(vector<Hand>& hands, vector<int>& winners, HandRank best){
  clearScreen();
  printTable("RESULTS", hands);

  cout << "Winning hand: " << getCategoryName(best.category) << "\n\n";

  cout << "Winner(s): ";
  for (int player : winners) {
    cout << "Player " << player << " ";
  }

  cout << '\n';
  cout << "=============================\n";
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

  int cardsNeeded = 5 + numPlayers*(2 + 2*var);
  if(cardsNeeded > 52) throw runtime_error("Can't play with this many players");

  Deck deck;
  deck.shuffle();

  Variant variant = (var == 0) ? Variant::TexasHoldem : Variant::Omaha;

  vector<Hand> hands;
  for(int i = 0; i < numPlayers; i++) {
    hands.push_back(Hand(variant));

    hands[i].addHoleCard(deck.draw());
    hands[i].addHoleCard(deck.draw());
    if (variant == Variant::Omaha) {
      hands[i].addHoleCard(deck.draw());
      hands[i].addHoleCard(deck.draw());
    }
  }

  hands[1].fold();
  hands[3].fold();

  cout << "\nGame: " << (variant == Variant::TexasHoldem ? "Texas Hold'em" : "Omaha") << '\n';
  printTable("PRE-FLOP", hands);

  waitForEnter("Reveal FLOP");
  revealCards(deck, hands, 3);
  printTable("FLOP", hands);

  waitForEnter("Reveal TURN");
  revealCards(deck, hands, 1);
  printTable("TURN", hands);

  waitForEnter("Reveal RIVER");
  revealCards(deck, hands, 1);
  printTable("RIVER", hands);

  waitForEnter("Reveal Results");

  vector<HandRank> ranks;
  for(int i = 0; i < numPlayers; i++) {
    auto [rank, cards] = evaluateHand(hands[i]);
    ranks.push_back(rank);
  }

  auto [best, winners] = getWinners(ranks, hands);

  printStats(hands, winners, best);

  return 0;
}
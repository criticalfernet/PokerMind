#pragma once

#include <vector>
#include "hand.hpp"
#include "evaluator.hpp"

std::vector<Card> getKnownCards(const std::vector<Hand>& hands) {
  std::vector<Card> revealed;

  if(!hands.empty()) {
    for (const Card& c : hands[0].getCommCards()){
      revealed.push_back(c);
    }
  }

  for (const Hand& hand : hands) {
    for (const Card& c : hand.getHoleCards()) {
      revealed.push_back(c);
    }
  }

  return revealed;
}


std::vector<int> monteCarloStep(Deck& deck, const std::vector<Hand>& hands){
  if (hands.empty()) return {};
  std::vector<Hand> _hands = hands; //local copy
  
  const size_t remaining = 5 - _hands[0].getCommCardsCount();
  deck.shuffleK(remaining);

  for(size_t i = 0; i < remaining; i++) {
    Card card = deck.draw();
    for(Hand& h : _hands) {
      if(!h.getFolded())h.addCommCard(card);
    }
  }

  HandRank best;
  bool found = false;
  std::vector<int> winners;
  winners.reserve(_hands.size());

  for (size_t i = 0; i < _hands.size(); i++) {
    if (_hands[i].getFolded()) continue;

    auto [rank, _] = evaluateHand(_hands[i]);

    if (!found || best < rank) {
      best = rank;
      found = true;
      winners.clear();
      winners.push_back(static_cast<int>(i));
    }
    else if (rank == best) {
      winners.push_back(static_cast<int>(i));
    }
  }

  return winners;
}


std::vector<double> monteCarlo(const std::vector<Hand>& hands, int simulations) {
  if(hands.empty() || simulations <= 0) return {};

  std::vector<Card> known = getKnownCards(hands);
  Deck deck(known);

  std::vector<int> wins(hands.size(), 0);

  for(int i = 0; i < simulations; i++){
    std::vector<int> winners = monteCarloStep(deck, hands);
    for (int player : winners) {
      wins[player]++;
    }
  }

  std::vector<double> probabilities(hands.size(), 0.0);
  for (size_t i = 0; i < hands.size(); i++) {
    probabilities[i] = static_cast<double>(wins[i]) / simulations;
  }

  return probabilities;
}

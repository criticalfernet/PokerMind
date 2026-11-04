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


std::vector<int> monteCarloStep(Deck& deck, std::vector<Card>& communityCards, const std::vector<std::vector<Card>>& holeCards, Variant var){
  if (holeCards.empty()) return {};
  
  const size_t remaining = 5 - communityCards.size();

  for(size_t i = 0; i < remaining; i++) {
    communityCards.push_back(deck.draw());
  }

  HandRank best;
  bool found = false;
  std::vector<int> winners;
  winners.reserve(holeCards.size());

  for (size_t i = 0; i < holeCards.size(); i++) {
    auto [rank, _] = evaluateHand(communityCards, holeCards[i], var);

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

  communityCards.resize(communityCards.size() - remaining);

  return winners;
}


std::vector<int> monteCarloStep(Deck& deck,const std::vector<Hand>& hands){
  std::vector<Card> communityCards = hands[0].getCommCards();
  std::vector<std::vector<Card>> holeCards;
  std::vector<int> playerIndices;

  for(size_t i = 0; i < hands.size(); i++) {
    if(hands[i].getFolded()) continue;
    holeCards.push_back(hands[i].getHoleCards());
    playerIndices.push_back(static_cast<int>(i));
  }

  std::vector<int> winners = monteCarloStep(deck, communityCards, holeCards, hands[0].getVariant());

  for (int& winner : winners) {
    winner = playerIndices[winner];
  }

  return winners;
}


std::vector<double> monteCarlo(const std::vector<Hand>& hands, int simulations) {
  if(hands.empty() || simulations <= 0) return {};

  std::vector<Card> known = getKnownCards(hands);
  Deck deck(known);

  std::vector<int> wins(hands.size(), 0);

  for(int i = 0; i < simulations; i++){
    deck.shuffle();
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


std::vector<int> monteCarloPartialStep(Deck& deck, int remainingPlayers, Hand& hand) {
  std::vector<Card> communityCards = hand.getCommCards();
  std::vector<std::vector<Card>> holeCards;
  holeCards.push_back(hand.getHoleCards());

  for(int i = 0; i < remainingPlayers; i++) {
    if(hand.getVariant() == Variant::TexasHoldem) holeCards.push_back({deck.draw(), deck.draw()});
    if(hand.getVariant() == Variant::Omaha) holeCards.push_back({deck.draw(), deck.draw(),deck.draw(), deck.draw()});
  }

  return monteCarloStep(deck, communityCards, holeCards, hand.getVariant());
}


double monteCarloPartial(int totalPlayers, Hand& hand, int simulations) {
  if(simulations <= 0) return {};

  std::vector<Card> known = getKnownCards({hand});
  Deck deck(known);

  int wins = 0;

  for(int i = 0; i < simulations; i++){
    deck.shuffle();
    std::vector<int> winners = monteCarloPartialStep(deck,totalPlayers-1, hand);
    for (int player : winners) {
      if(player == 0) wins++;
    }
  }

  return static_cast<double>(wins) / simulations;
}
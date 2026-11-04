#pragma once

#include <vector>
#include <future>
#include <thread>
#include <algorithm>

#include "hand.hpp"
#include "evaluator.hpp"

inline std::vector<Card> getKnownCards(const std::vector<Hand>& hands) {
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


inline std::vector<int> monteCarloStep(Deck& deck, std::vector<Card>& communityCards, const std::vector<std::vector<Card>>& holeCards, Variant var){
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
    const HandRank& rank = evaluateHand(communityCards, holeCards[i], var);

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


inline std::vector<int> monteCarloStep(Deck& deck,const std::vector<Hand>& hands){
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


inline std::vector<double> monteCarlo(const std::vector<Hand>& hands, int simulations) {
  if(hands.empty() || simulations <= 0) return {};
  if (hands[0].getCommCards().size() == 5) simulations = 1;

  std::vector<Card> known = getKnownCards(hands);
  Deck baseDeck(known);

  unsigned hw = std::thread::hardware_concurrency();
  const int nThreads = std::max(1u, std::min<unsigned>(hw, simulations));
  const int chunk = simulations / nThreads;
  const int extra = simulations % nThreads;


  auto worker = [&](int count) -> std::vector<int> {
    Deck deck = baseDeck;
    std::vector<int> wins(hands.size(), 0);

    for (int s = 0; s < count; s++) {
      deck.shuffle();
      std::vector<int> winners = monteCarloStep(deck, hands);
      for (int player : winners) {
        wins[player]++;
      }
    }
    return wins;
  };

  std::vector<std::future<std::vector<int>>> futures;
  futures.reserve(nThreads);

  for(int t = 0; t < nThreads; t++){
    const int count = chunk + (t < extra ? 1 : 0);
    futures.push_back(std::async(std::launch::async, worker, count));
  }

  std::vector<int> totals(hands.size(), 0);
  for (auto& f : futures) {
    const std::vector<int> w = f.get();
    for (size_t i = 0; i < totals.size(); ++i) totals[i] += w[i];
  }

  std::vector<double> probabilities(hands.size(), 0.0);

  for (size_t i = 0; i < totals.size(); ++i){
    probabilities[i] = static_cast<double>(totals[i]) / simulations;
  }

  return probabilities;
}


inline std::vector<int> monteCarloPartialStep(Deck& deck, int remainingPlayers, const Hand& hand) {
  std::vector<Card> communityCards = hand.getCommCards();
  std::vector<std::vector<Card>> holeCards;
  holeCards.push_back(hand.getHoleCards());

  const int cardsPerPlayer = hand.getVariant() == Variant::TexasHoldem ? 2 : 4;
  for (int i = 0; i < remainingPlayers; i++) {
    std::vector<Card> holes;
    holes.reserve(cardsPerPlayer);
    for (int j = 0; j < cardsPerPlayer; j++) holes.push_back(deck.draw());
    holeCards.push_back(std::move(holes));
  }

  return monteCarloStep(deck, communityCards, holeCards, hand.getVariant());
}


inline double monteCarloPartial(int totalPlayers, const Hand& hand, int simulations) {
  if(simulations <= 0) return 0.0;

  std::vector<Card> known = hand.getCommCards();
  known.insert(known.end(), hand.getHoleCards().begin(), hand.getHoleCards().end());
  const Deck baseDeck(known);

  unsigned hw = std::thread::hardware_concurrency();
  const int nThreads = std::max(1u, std::min<unsigned>(hw, simulations));
  const int chunk = simulations / nThreads;
  const int extra = simulations % nThreads;

  auto worker = [&](int count) -> int {
    Deck deck = baseDeck;
    int wins = 0;
    for (int i = 0; i < count; i++) {
      deck.shuffle();
      for (int p : monteCarloPartialStep(deck, totalPlayers - 1, hand))
        if (p == 0) wins++;
    }
    return wins;
  };

  std::vector<std::future<int>> futures;
  futures.reserve(nThreads);
  for (int t = 0; t < nThreads; t++)
    futures.push_back(std::async(std::launch::async, worker, chunk + (t < extra ? 1 : 0)));

  int wins = 0;
  for (auto& f : futures) wins += f.get();

  return static_cast<double>(wins) / simulations;
}
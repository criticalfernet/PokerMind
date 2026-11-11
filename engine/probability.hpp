#pragma once

#include <vector>
#include <future>
#include <thread>
#include <algorithm>
#include <random>

#include "hand.hpp"
#include "evaluator.hpp"
#include "bestHands.hpp"

inline std::vector<double> buildHandCdf(size_t n, double weight) {
  constexpr double BIAS_STRENGTH = 4.0;
  const double k = (weight - 0.5) * 2.0 * BIAS_STRENGTH;

  std::vector<double> cdf(n);
  double running = 0.0;
  for (size_t i = 0; i < n; ++i) {
    const double q = n > 1 ? 1.0 - static_cast<double>(i) / (n - 1) : 1.0;
    running += std::exp(k * q);
    cdf[i] = running;
  }
  return cdf;
}

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


inline std::vector<int> monteCarloPartialStep(Deck& deck,const Hand& hand,const std::vector<std::array<Card, 2>>& sortedHands,
  const std::vector<std::vector<double>>& opponentCdfs,std::mt19937& rng) {

  std::vector<Card> communityCards = hand.getCommCards();
  std::vector<std::vector<Card>> holeCards;
  holeCards.push_back(hand.getHoleCards());

  bool used[4][15] = {};
  std::uniform_real_distribution<double> uni(0.0, 1.0);

  for (const auto& cdf : opponentCdfs) {
    bool assigned = false;

    for (int attempt = 0; attempt < 100 && !assigned; ++attempt) {
      const double u = uni(rng) * cdf.back();
      size_t idx = std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin();
      if (idx >= sortedHands.size()) idx = sortedHands.size() - 1;

      const auto& h = sortedHands[idx];
      const int s0 = static_cast<int>(h[0].suit), s1 = static_cast<int>(h[1].suit);
      if (used[s0][h[0].rank] || used[s1][h[1].rank]) continue;

      used[s0][h[0].rank] = used[s1][h[1].rank] = true;
      deck.remove(h[0]);
      deck.remove(h[1]);
      holeCards.push_back({h[0], h[1]});
      assigned = true;
    }

    if (!assigned) {
      const int cardsPerPlayer = hand.getVariant() == Variant::TexasHoldem ? 2 : 4;
      std::vector<Card> holes;
      for (int j = 0; j < cardsPerPlayer; ++j) holes.push_back(deck.draw());
      holeCards.push_back(std::move(holes));
    }
  }

  return monteCarloStep(deck, communityCards, holeCards, hand.getVariant());
}


inline double monteCarloPartial(int totalPlayers, const Hand& hand, int simulations, const std::vector<double>& weights = {}) {
  if(simulations <= 0) return 0.0;

  std::vector<Card> known = hand.getCommCards();
  known.insert(known.end(), hand.getHoleCards().begin(), hand.getHoleCards().end());
  const Deck baseDeck(known);

  const auto sortedHands = sortPossibleHands(getPossibleHands(hand), hand.getCommCards());

  const int opponents = totalPlayers - 1;
  std::vector<std::vector<double>> opponentCdfs;
  for (int i = 0; i < opponents; ++i) {
    const double w = i < static_cast<int>(weights.size()) ? weights[i] : 0.5;
    opponentCdfs.push_back(buildHandCdf(sortedHands.size(), w));
  }

  unsigned hw = std::thread::hardware_concurrency();
  const int nThreads = std::max(1u, std::min<unsigned>(hw, simulations));
  const int chunk = simulations / nThreads;
  const int extra = simulations % nThreads;

  auto worker = [&](int count) -> double {
    std::mt19937 rng(std::random_device{}());
    double wins = 0;
    for (int i = 0; i < count; i++) {
      Deck deck = baseDeck;
      deck.shuffle();
      auto winners = monteCarloPartialStep(deck, hand, sortedHands, opponentCdfs, rng);
      if(std::find(winners.begin(), winners.end(), 0) != winners.end())
        wins += 1.0/ winners.size();
    }
    return wins;
  };

  std::vector<std::future<double>> futures;
  futures.reserve(nThreads);
  for (int t = 0; t < nThreads; t++)
    futures.push_back(std::async(std::launch::async, worker, chunk + (t < extra ? 1 : 0)));

  double wins = 0;
  for (auto& f : futures) wins += f.get();

  return static_cast<double>(wins) / simulations;
}
#pragma once

#include <iostream>
#include <vector>
#include <format>
#include <chrono>

#include "deck.hpp"
#include "hand.hpp"
#include "evaluator.hpp"
#include "probability.hpp"

int checkForWeightsMonteCarlo() {
  constexpr int PLAYERS = 5;
  constexpr int SIMS = 1'000'000;

  // My hand: Q♠ 9♦
  Hand hand(Variant::TexasHoldem);
  hand.addHoleCard({12, Suit::Diamonds});
  hand.addHoleCard({9,  Suit::Diamonds});

  // Flop: 9♥ 5♣ 2♦
  hand.addCommCard({9, Suit::Hearts});
  hand.addCommCard({5, Suit::Diamonds});
  hand.addCommCard({2, Suit::Diamonds});

  std::cout << "Hole:";
  for (const Card& c : hand.getHoleCards()) std::cout << " " << c;
  std::cout << "\nBoard:";
  for (const Card& c : hand.getCommCards()) std::cout << " " << c;
  std::cout << "\nPlayers: " << PLAYERS << ", sims: " << SIMS << "\n\n";

  auto run = [&](const std::string& label, const std::vector<double>& weights) {
    const auto t0 = std::chrono::steady_clock::now();
    const double p = monteCarloPartial(PLAYERS, hand, SIMS, weights);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>
      (std::chrono::steady_clock::now() - t0).count();
    std::cout << std::format("{:<28} win: {:6.2f}%   ({} ms)\n", label, p * 100, ms);
  };

  const int opp = PLAYERS - 1;

  run("1. No weights",{});
  run("2. All strong (0.99)", std::vector<double>(opp, 0.99));
  run("3. All weak (0.01)", std::vector<double>(opp, 0.01));
  run("4. Neutral (0.5)", std::vector<double>(opp, 0.50));
  run("5. Mixed (0.01, 0.1, 0.2, 0.3)", {0.01, 0.1, 0.2, 0.3});
  run("6. Mixed (0.4, 0.5, 0.6, 0.7)", {0.4, 0.5, 0.6, 0.7});
  run("7. Mixed (0.7, 0.8, 0.9, 0.99)", {0.7, 0.8, 0.9, 0.99});

  return 0;
}


int checkForSortedHands(int cap = 25) {
  Simulator sim(Variant::TexasHoldem);
  for(int i = 0; i < 2; i++) {
    sim.addPlayer(500);
  }

  sim.startHand();
  sim.dealNextStreet();
  sim.dealNextStreet();

  Hand hand = sim.getHands()[0];

  std::cout << "Community Cards: ";
  for(Card c : hand.getCommCards()) {
    std::cout << c << " ";
  }
  std::cout << '\n';
  std::cout << "Hole Cards: ";
  for(Card c : hand.getHoleCards()) {
    std::cout << c << " ";
  }
  std::cout << '\n';
  std::cout << '\n';

  auto possible = getPossibleHands(hand);
  auto sorted = sortPossibleHands(possible, hand.getCommCards());

  for(int i = 0; i < cap ; i++) {
    auto pr = sorted[i];
    std::cout << pr[0] << ' ' << pr[1] << '\n';
  }

  return 0;
}
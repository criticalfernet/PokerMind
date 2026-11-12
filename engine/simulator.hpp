#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <memory>

#include "deck.hpp"
#include "hand.hpp"
#include "expected.hpp"
#include "decision.hpp"
#include "player.hpp"

class Simulator {
  Variant variant;
  std::vector<Player> players;
  std::vector<Hand> hands{};
  size_t dealerPosition = 0;
  int currentBet = 0;
  int pot = 0;

  Deck deck;
  std::vector<Card> communityCards;
  std::vector<HandAction> actionHistory;

  Street street = Street::PreFlop;

  static constexpr int SMALL_BLIND = 10;

  std::vector<size_t> bestAmong(const std::vector<size_t>& eligible,
                                const std::vector<HandRank>& ranks) const {
    std::vector<size_t> winners;
    for (size_t i : eligible) {
      if (winners.empty() || ranks[winners.front()] < ranks[i]) {
        winners.clear();
        winners.push_back(i);
      } else if (ranks[i] == ranks[winners.front()]) {
        winners.push_back(i);
      }
    }
    return winners;
  }

  void payOut(int amount, const std::vector<size_t>& winners) {
    const int share = amount / static_cast<int>(winners.size());
    int remainder = amount % static_cast<int>(winners.size());
    for (size_t w : winners) {
      players[w].money += share;
      if (remainder > 0) { players[w].money++; remainder--; }
    }
    std::cout << "Pot of " << amount << " won by Player";
    if (winners.size() > 1) std::cout << "s";
    for (size_t w : winners) std::cout << " " << (w + 1);
    std::cout << "\n";
  }

public:
  explicit Simulator(Variant gameVariant) : variant(gameVariant) {}

  void addPlayer(std::int64_t startingMoney, std::unique_ptr<DecisionPolicy> policy = nullptr){
    size_t idx = hands.size();
    hands.emplace_back(variant);
    players.emplace_back(static_cast<int>(startingMoney), idx, std::move(policy));
  }

  Variant getVariant() const { return variant; }
  Street getStreet() const { return street; }
  const std::vector<Hand>& getHands() const { return hands; }
  int getPot() const { return pot; }
  const std::vector<Player>& getPlayers() const { return players; }

  void startHand() {
    const std::size_t holeCount = variant == Variant::TexasHoldem ? 2 : 4;
    if (players.size() * holeCount + 5 > 52) throw std::logic_error("Too many players for this variant");

    pot = 0;
    currentBet = 0;
    communityCards.clear();
    deck = Deck();
    deck.shuffle();
    actionHistory = {};

    size_t withChips = 0;
    for (Player& p : players) {
      p.resetForHand();
      if (!p.folded) withChips++;
    }
    if (withChips < 2) throw std::logic_error("Need at least two players with chips");

    for (size_t i = 0; i < players.size(); i++) hands[i] = Hand(variant);

    for (Player& player : players) {
      if (player.folded) continue;
      for (size_t c = 0; c < holeCount; c++)
        hands[player.handindex].addHoleCard(deck.draw());
    }

    street = Street::PreFlop;
  }

  bool dealNextStreet() {
    int count = 0;
    switch (street) {
      case Street::PreFlop:  count = 3; street = Street::Flop; break;
      case Street::Flop:     count = 1; street = Street::Turn; break;
      case Street::Turn:     count = 1; street = Street::River; break;
      case Street::River:    count = 0; street = Street::Showdown; break;
      case Street::Showdown: return false;
    }

    for (int i = 0; i < count; ++i) {
      Card card = deck.draw();
      communityCards.push_back(card);
      for (Player& player : players)
        if (!player.folded) hands[player.handindex].addCommCard(card);
    }

    return true;
  }

  size_t activePlayerCount() const { 
    std::size_t count = 0;
    for (const Player& p : players)
      if (!p.folded) count++;
    return count;
  }

  size_t actingPlayerCount() const { 
    std::size_t count = 0;
    for (const Player& p : players)
      if (p.canAct()) count++;
    return count;
  }

  void beginBettingRound() {
    const size_t n = players.size();
    for (Player& p : players) 
      p.bet = 0;
    currentBet = 0;

    size_t cursor = (dealerPosition + 1) % n;

    if (street == Street::PreFlop) {
      for (size_t off = 1; off <= n; off++) {
        size_t i = (dealerPosition + off) % n;
        Player& sb = players[i];
        if (sb.folded) continue;
        const int amount = std::min(SMALL_BLIND, sb.money);
        sb.money -= amount;
        sb.bet += amount;
        sb.totalBet += amount;
        if (sb.money == 0) sb.allIn = true;
        pot += amount;
        currentBet = sb.bet;
        std::cout << "Player " << (i + 1) << " posts small blind " << amount << "\n";
        cursor = (i + 1) % n;
        break;
      }
    }

    std::vector<bool> pending(n, false);
    for (size_t i = 0; i < n; i++) pending[i] = players[i].canAct();

    while (activePlayerCount() > 1) {
      size_t next = n;
      for (size_t off = 0; off < n; off++) {
        size_t j = (cursor + off) % n;
        if (pending[j] && players[j].canAct()) { next = j; break; }
      }
      if (next == n) break;

      Player& player = players[next];

      if (player.bet >= currentBet && actingPlayerCount() == 1) {
        pending[next] = false;
        continue;
      }

      const int prevBet = player.bet;
      HandAction log = player.play(next+1,pot,currentBet,n,street,hands[player.handindex], actionHistory);
      actionHistory.push_back(log);
      const int contribution = player.bet - prevBet;

      pot += contribution;
      player.totalBet += contribution;
      pending[next] = false;

      if (player.bet > currentBet) {
        currentBet = player.bet;
        for (size_t j = 0; j < n; j++)
          if (j != next && players[j].canAct()) pending[j] = true;
      }

      cursor = (next + 1) % n;
    }
  }

  std::pair<HandRank, std::vector<int>> getWinners() {
    std::vector<HandRank> ranks(hands.size());
    for (size_t i = 0; i < hands.size(); i++)
      if (!players[i].folded) ranks[i] = evaluateHand(hands[i]);

    std::vector<size_t> eligible;
    for (size_t i = 0; i < players.size(); i++)
      if (!players[i].folded) eligible.push_back(i);

    if (eligible.empty()) throw std::runtime_error("No eligible players remaining");

    std::vector<size_t> best = bestAmong(eligible, ranks);
    std::vector<int> winners;
    for (size_t w : best) winners.push_back(static_cast<int>(w + 1));
    return {ranks[best.front()], winners};
  }

  void distributePot() {
    const size_t n = players.size();

    std::vector<size_t> contenders;
    for (size_t i = 0; i < n; i++)
      if (!players[i].folded) contenders.push_back(i);

    if (contenders.empty()) throw std::runtime_error("No eligible players remaining");

    if (contenders.size() == 1) {
      payOut(pot, contenders);
      pot = 0;
      return;
    }

    std::vector<HandRank> ranks(n);
    for (size_t i : contenders) ranks[i] = evaluateHand(hands[i]);

    std::vector<int> levels;
    for (size_t i : contenders) levels.push_back(players[i].totalBet);
    std::sort(levels.begin(), levels.end());
    levels.erase(std::unique(levels.begin(), levels.end()), levels.end());

    int prev = 0;
    for (size_t k = 0; k < levels.size(); k++) {
      const int level = levels[k];
      const bool last = (k + 1 == levels.size());

      int amount = 0;
      for (const Player& p : players) {
        const int upper = last ? p.totalBet : std::min(p.totalBet, level);
        amount += std::max(0, upper - prev);
      }

      std::vector<size_t> eligible;
      for (size_t i : contenders)
        if (players[i].totalBet >= level) eligible.push_back(i);

      if (amount > 0) payOut(amount, bestAmong(eligible, ranks));
      prev = level;
    }
    pot = 0;
  }

  void playHand() {
    startHand();
    while (true) {
      beginBettingRound();
      if (activePlayerCount() <= 1) break;
      dealNextStreet();
      if (street == Street::Showdown) break;
    }
    distributePot();
    dealerPosition = (dealerPosition + 1) % players.size();
  }
};
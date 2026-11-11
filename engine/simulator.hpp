#pragma once

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

#include "deck.hpp"
#include "hand.hpp"
#include "expected.hpp"

enum class Street : std::uint8_t {
  PreFlop, Flop, Turn, River, Showdown
};

enum class Action : std::uint8_t {
  Allin,
  Raise,
  Call,
  Check,
  Fold,
  None
};

inline std::ostream& operator<<(std::ostream& os, Street street) {
  switch (street) {
    case Street::PreFlop: return os << "Pre-Flop";
    case Street::Flop: return os << "Flop";
    case Street::Turn: return os << "Turn";
    case Street::River: return os << "River";
    case Street::Showdown: return os << "Showdown";
    default: return os << "Unknown";
  }
}

inline void printOptions() {
  std::cout
    << "\nChoose action:\n"
    << "0. All-in\n"
    << "1. Raise\n"
    << "2. Call\n"
    << "3. Check\n"
    << "4. Fold\n"
    << "> ";
}

inline int readInt() {
  int value;
  while (true) {
    if (std::cin >> value) return value;
    if (std::cin.eof()) throw std::runtime_error("Input stream closed");
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "Invalid input.\n> ";
  }
}

class Player {
public:
  int money = 0;
  int bet = 0;
  int totalBet = 0;

  size_t handindex = static_cast<size_t>(-1);
  bool folded = false;
  bool allIn = false;
  Action lastAction = Action::None;

  explicit Player(int startingMoney, size_t handindex)
    : money(startingMoney), handindex(handindex) {}

  bool canAct() const { return !folded && !allIn && money > 0; }

  void resetForHand() {
    bet = 0;
    totalBet = 0;
    folded = (money <= 0);
    allIn = false;
    lastAction = Action::None;
  }

  void printStats(int toCall, const Hand& hand, size_t idx, size_t players, int pot) const{
    std::cout << "\n========== Player " << idx << " ==========\n";
    std::cout << "\nPot: "<< pot <<"  To call: " << toCall << "\n";
    std::cout << "Community cards:";
    std::vector<Card> communityCards = hand.getCommCards();
    if (communityCards.empty()) std::cout << " (none)";
    for (const Card& c : communityCards) std::cout << " " << c;
    std::cout << "\n";

    std::vector<Card> holeCards = hand.getHoleCards();
 
    std::cout << "Hole cards:";
    for (const Card& c : holeCards) std::cout << " " << c;
    std::cout << "\n";
 
    std::cout << "Money left: " << money << "\n";


    const ExpectedReturn r = expectedReturn(players, hand, pot, toCall, totalBet);
    const std::vector<double> strong(players - 1, 0.9);
    const double pStrong = monteCarloPartial(players, hand, 200'000, strong);

    const RaiseAdvice adv = bestRaise(r.equity,pStrong,pot,toCall, money, r.value,players-1);
    std::cout << std::format("Win probability: {:.2f}% \n",r.equity * 100);
    std::cout << std::format("Expected return: {:+.1f} chips (net this hand: {:+.1f})\n", r.value, r.netValue);
    if (adv.amount > 0)
      std::cout << std::format("Suggestion: raise +{} (EV {:+.1f})\n", adv.amount - toCall, adv.value);
    else
      std::cout << "Suggestion: don't raise\n";

  }

  Action askMove(int currentBet, size_t idx, const Hand& hand, size_t players, int pot) const {
    if (!canAct()) return Action::None;
    const int toCall = currentBet - bet;

    printStats(toCall, hand, idx, players, pot);

    while (true) {
      printOptions();
      int choice = readInt();

      switch (choice) {
        case 0:
          return Action::Allin;
        case 1:
          if (money <= toCall) {
            std::cout << "Insufficient money to raise.\n";
            break;
          }
          return Action::Raise;
        case 2:
          if (toCall <= 0) {
            std::cout << "Nothing to call. Check instead.\n";
            break;
          }
          if (money < toCall) {
            std::cout << "Insufficient money to call. Go all-in or fold.\n";
            break;
          }
          return Action::Call;
        case 3:
          if (bet != currentBet) {
            std::cout << "Cannot check. You must call or fold.\n";
            break;
          }
          return Action::Check;
        case 4:
          return Action::Fold;
        default:
          std::cout << "Invalid Action.\n";
      }
    }
  }

  void play(Action move, int currentBet) {
    if (!canAct()) {
      lastAction = Action::None;
      return;
    }

    switch (move) {
      case Action::None: break;
      case Action::Fold:
        folded = true;
        lastAction = Action::Fold;
        break;
      case Action::Allin:
        bet += money;
        money = 0;
        allIn = true;
        lastAction = Action::Allin;
        break;
      case Action::Check:
        lastAction = Action::Check;
        break;
      case Action::Call: {
        int amount = currentBet - bet;
        money -= amount;
        bet += amount;
        if (money == 0) {
          allIn = true;
          lastAction = Action::Allin;
          break;
        }
        lastAction = Action::Call;
        break;
      }
      case Action::Raise: {
        while (true) {
          std::cout << "Enter amount to put in (must exceed " << (currentBet - bet)
                    << " to call, max " << money << "): ";
          int amount = readInt();

          if (amount <= currentBet - bet) {
            std::cout << "Raise must exceed the call amount.\n";
            continue;
          }
          if (amount > money) {
            std::cout << "Insufficient money to raise.\n";
            continue;
          }

          money -= amount;
          bet += amount;

          if (money == 0) {
            allIn = true;
            lastAction = Action::Allin;
          } else {
            lastAction = Action::Raise;
          }
          return;
        }
      }
    }
  }
};


class Simulator {
  Variant variant;
  std::vector<Player> players;
  std::vector<Hand> hands{};
  size_t dealerPosition = 0;
  int currentBet = 0;
  int pot = 0;

  Deck deck;
  std::vector<Card> communityCards;

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

  void addPlayer(std::int64_t startingMoney) {
    size_t idx = hands.size();
    hands.emplace_back(variant);
    players.emplace_back(static_cast<int>(startingMoney), idx);
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
    for (Player& p : players) { p.bet = 0; p.lastAction = Action::None; }
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
      Action move = player.askMove(currentBet, next + 1, hands[player.handindex],n, pot);
      player.play(move, currentBet);
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
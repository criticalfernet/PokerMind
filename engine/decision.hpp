#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>

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


struct HandAction {
    size_t playerIndex;
    Action action;
    int amount;
    Street street;

    int potBefore = 0;
    int toCall = 0;
};


struct DecisionContext {
  size_t playerIndex = 0;
  int money = 0;
  int totalBet = 0;

  Street street = Street::PreFlop;
  int pot = 0;
  int currentBet = 0;
  int toCall = 0;
  size_t totalPlayers = 0;

  std::vector<Card> holeCards;
  std::vector<Card> communityCards;

  ExpectedReturn expected{};
  RaiseAdvice raiseAdvice{0 , 0.0};

  bool canPlay = true;
  std::vector<HandAction> actionHistory;
};

struct Decision {
  Action action = Action::None;
  int amount = 0;
};

class DecisionPolicy {
public:
  std::vector<double> weights;
  double foldBase = 0.6;

  void setFoldBase(double f) {
    foldBase = std::clamp(f, 0.0, 1.0);
  }

  virtual ~DecisionPolicy() = default;
  virtual Decision decide(const DecisionContext& context) = 0;
};
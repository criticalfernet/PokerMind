#pragma once

#include <iostream>
#include <format>

#include "decision.hpp"

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


void PrintStats(const DecisionContext& ctx) {
  std::cout << "\n========== Player " << ctx.playerIndex << " ==========\n";
  std::cout << "\nPot: "<< ctx.pot <<"  To call: " << ctx.toCall << "\n";
  std::cout << "Community cards:";
  if (ctx.communityCards.empty()) std::cout << " (none)";
  for (const Card& c : ctx.communityCards) std::cout << " " << c;
  std::cout << "\n";


  std::cout << "Hole cards:";
  for (const Card& c : ctx.holeCards) std::cout << " " << c;
  std::cout << "\n";

  std::cout << "Money left: " << ctx.money << "\n";

  std::cout << std::format("Win probability: {:.2f}% \n",ctx.expected.equity * 100);
  std::cout << std::format("Expected return: {:+.1f} chips (net this hand: {:+.1f})\n", ctx.expected.value, ctx.expected.netValue);
  if (ctx.raiseAdvice.amount > 0)
    std::cout << std::format("Suggestion: raise +{} (EV {:+.1f})\n", ctx.raiseAdvice.amount - ctx.toCall, ctx.raiseAdvice.value);
  else
    std::cout << "Suggestion: don't raise\n";
}


Action askMove(const DecisionContext& ctx){
  if (!ctx.canPlay) return Action::None;

  while (true) {
    printOptions();
    int choice = readInt();

    switch (choice) {
      case 0:
        return Action::Allin;
      case 1:
        if (ctx.money <= ctx.toCall) {
          std::cout << "Insufficient money to raise.\n";
          break;
        }
        return Action::Raise;
      case 2:
        if (ctx.toCall <= 0) {
          std::cout << "Nothing to call. Check instead.\n";
          break;
        }
        if (ctx.money < ctx.toCall) {
          std::cout << "Insufficient money to call. Go all-in or fold.\n";
          break;
        }
        return Action::Call;
      case 3:
        if (ctx.totalBet != ctx.currentBet) {
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


Decision play(Action move, const DecisionContext& ctx) {
  if (!ctx.canPlay) {
    return {Action::None, 0};
  }

  switch (move) {
    case Action::None: break;
    case Action::Fold:
      return {Action::Fold, 0};
    case Action::Allin:
      return {Action::Allin,0};
    case Action::Check:
      return {Action::Check,0};
    case Action::Call: {
      if (ctx.money <= ctx.toCall) {
        return {Action::Allin,0};
      }
      return {Action::Call, ctx.toCall};
    }
    case Action::Raise: {
      while (true) {
        std::cout << "Enter amount to put in (must exceed " << ctx.toCall
                  << " to call, max " << ctx.money << "): ";
        int amount = readInt();

        if (amount <= ctx.toCall) {
          std::cout << "Raise must exceed the call amount.\n";
          continue;
        }
        if (amount > ctx.money) {
          std::cout << "Insufficient money to raise.\n";
          continue;
        }

        if (ctx.money == amount) {
          return {Action::Allin, 0};
        } else {
          return {Action::Raise, amount};
        }
      }
    }
  }

  return {Action::None, 0};
}


class HumanPolicy : public DecisionPolicy {
public:
  Decision decide(const DecisionContext& ctx) override {
    PrintStats(ctx);
    Action a = askMove(ctx);
    return play(a, ctx);
  }
};
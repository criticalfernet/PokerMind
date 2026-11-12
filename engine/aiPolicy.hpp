#pragma once

#include <vector>
#include <iostream>

#include "bestHands.hpp"
#include "decision.hpp"

class AIPolicy : public DecisionPolicy {
public:
  double foldBase = 0.6;

  Decision decide(const DecisionContext& ctx) override {
    if (!ctx.canPlay)
      return {Action::None, 0};

    double foldThreshold = -0.25 * ctx.pot - 0.25*ctx.toCall;
    
    if(ctx.expected.value < foldThreshold)
      return {Action::Fold, 0};
    
    if(ctx.raiseAdvice.value > 0)
      return {Action::Raise, ctx.raiseAdvice.amount};
    
    if (ctx.toCall > 0)
      return {Action::Call, ctx.toCall};

    return {Action::Check, 0};
  }

  void revealHands(size_t playerIndex, const std::vector<RevealedHand>& hands) override{
    const Hand& myHand = hands[playerIndex].hand;
    auto allHands = sortPossibleHands(getPossibleHands(myHand),myHand.getCommCards());

    for(const auto& r : hands) {
      if (r.playerIndex == playerIndex) continue;
      weightsMap[r.playerIndex] = getHandWeight(r.hand.getHoleCards(),allHands);
    }

  }
};
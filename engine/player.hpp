#pragma once

#include "expected.hpp"
#include "probability.hpp"
#include "simulator.hpp"
#include "hand.hpp"
#include "card.hpp"

class Player {
public:
  std::unique_ptr<DecisionPolicy> policy;

  int money = 0;
  int bet = 0;
  int totalBet = 0;

  size_t handindex = static_cast<size_t>(-1);
  bool folded = false;
  bool allIn = false;


  explicit Player(int startingMoney, size_t handindex, std::unique_ptr<DecisionPolicy> policy)
    : policy(std::move(policy)), money(startingMoney), handindex(handindex) {}

  bool canAct() const { return !folded && !allIn && money > 0; }

  void resetForHand() {
    bet = 0;
    totalBet = 0;
    folded = (money <= 0);
    allIn = false;
  }

  DecisionContext makeDecisionContext(size_t playerIndex,int pot,int currentBet,size_t totalPlayers,Street street,
    const Hand& hand, const std::vector<HandAction>& history) const {
    DecisionContext ctx;

    ctx.playerIndex = playerIndex;
    ctx.money = money;
    ctx.totalBet = totalBet;

    ctx.pot = pot;
    ctx.currentBet = currentBet;
    ctx.toCall = currentBet - bet;
    ctx.totalPlayers = totalPlayers;
    ctx.street = street;

    ctx.holeCards = hand.getHoleCards();
    ctx.communityCards = hand.getCommCards();

    ctx.expected = expectedReturn(totalPlayers, hand, pot, ctx.toCall, totalBet, policy->weights);

    const std::vector<double> strong(totalPlayers - 1, 0.8);
    const double pStrong = monteCarloPartial(totalPlayers, hand, 200'000, strong);
    ctx.raiseAdvice = bestRaise(ctx.expected.equity,pStrong,pot,ctx.toCall, money, ctx.expected.value,totalPlayers-1, policy->foldBase);

    ctx.canPlay = canAct();
    ctx.actionHistory = history;

    return ctx;
  }

  void actDecision(const Decision& decision) {
    Action a = decision.action;
    int value = decision.amount;

    switch(a){
      case Action::None : break;
      case Action::Fold :
        folded = true;
        break;
      case Action::Check :
        break;
      case Action::Allin :
        bet += money;
        money = 0;
        allIn = true;
        break;
      case Action::Call : 
        money -= value;
        bet += value;
        break;
      case Action::Raise : 
        money -= value;
        bet += value;
        break;
    }    
  }

  HandAction play(size_t playerIndex,int pot,int currentBet,size_t totalPlayers,Street street,const Hand& hand, std::vector<HandAction>& action){
    DecisionContext ctx = makeDecisionContext(playerIndex,pot,currentBet,totalPlayers,street,hand,action);
    Decision decision = policy->decide(ctx);
    actDecision(decision);

    return {playerIndex,decision.action, decision.amount,street,pot,currentBet-bet};
  }
  
};
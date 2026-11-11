#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

#include "hand.hpp"
#include "probability.hpp"

struct ExpectedReturn {
  double equity;
  double value;
  double netValue;
  double breakEven;
  bool profit;
};

inline ExpectedReturn expectedReturn(int totalPLayers, const Hand& hand, int pot, int toCall, int total, const std::vector<double>& weights = {}) {
  int sims = 1'000'000;
  toCall = std::max(0, toCall);

  const double probability = monteCarloPartial(totalPLayers,hand,sims,weights);

  const double expectedValue = probability*pot - (1.0 - probability)*toCall;
  const double netExpectedValue = expectedValue - total;

  const double breakEven = static_cast<double>(toCall) / (pot+toCall);

  return {probability, expectedValue, netExpectedValue, breakEven, expectedValue > 0.0};
}

inline double expectedRaise(double probability,double pStrong, int pot,int toCall, int x,int opponents, double foldBase = 0.6){
  const double sizeFrac = static_cast<double>(x) / (pot+x);
  const double perOpp = foldBase * sizeFrac;
  const double fold = std::pow(perOpp, opponents);

  const double pCalled  = probability + (pStrong - probability)*sizeFrac*sizeFrac;

  const int r = x - toCall;
  const double callers = std::max(1.0, opponents * (1.0 - perOpp));
  const double evCalled = pCalled*(pot + callers * r) - (1.0 - pCalled)*x;

  return fold*pot + (1.0 - fold)*evCalled;
}

struct RaiseAdvice {
  int amount;
  double value;
};

inline std::vector<int> grids(int lower, int upper, int STEPS){
  std::vector<int> xs;
  for (int i = 0; i <= STEPS; i++) {
    const double t = static_cast<double>(i) / STEPS;
    const int x = static_cast<int>(std::lround(lower * std::pow(static_cast<double>(upper) / lower, t)));
    if (xs.empty() || x != xs.back()) xs.push_back(std::clamp(x, lower, upper));
  }

  return xs;
}

inline RaiseAdvice ternarySearch(std::vector<int> xs, size_t k,double bestV, auto f){
  int a = xs[k > 0 ? k-1 : 0];
  int b = xs[std::min(k+1,xs.size()-1)];

  while(b-a > 2) {
    const int m1 = a + (b - a) / 3;
    const int m2 = b - (b - a) / 3;
    if (f(m1) < f(m2)) a = m1; else b = m2;
  }

  int bestX = xs[k];
  for (int x = a; x <= b; ++x) {
    const double v = f(x);
    if (v > bestV) { bestV = v; bestX = x; }
  }

  return {bestX, bestV};
}

inline RaiseAdvice bestRaise(double probability, double pStrong, int pot, int toCall, int money, double callValue, int opponents, double foldBase = 0.6) {
  RaiseAdvice best{0, std::max(callValue, 0.0)};

  const int minRaise = toCall + 1;
  if (money < minRaise) return best;

  auto f = [&](int x) {
    return expectedRaise(probability,pStrong,pot,toCall,x,opponents,foldBase);
  };

  std::vector<int> xs = grids(minRaise, money, 24);
  size_t k = 0;
  double bestV = f(xs[0]);
  for (size_t i = 1; i < xs.size(); i++) {
    const double v = f(xs[i]);
    if (v > bestV) { bestV = v; k = i;}
  }

  RaiseAdvice result = ternarySearch(xs, k, bestV, f);

  return (result.value > best.value) ? result : best;
}
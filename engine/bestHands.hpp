#pragma once

#include <cstdint>
#include <array>
#include <vector>

#include "card.hpp"
#include "hand.hpp"
#include "evaluator.hpp"

inline std::vector<std::array<Card,2>> getPossibleHands(const Hand& hand) {
  bool blocked[4][15] = {};
  for (const Card& c : hand.getCommCards())
    blocked[static_cast<int>(c.suit)][c.rank] = true;
  for (const Card& c : hand.getHoleCards())
    blocked[static_cast<int>(c.suit)][c.rank] = true;

  std::vector<Card> pool;
  pool.reserve(52);
  for (int s = 0; s < 4; s++)
    for (int r = 2; r <= 14; r++)
      if (!blocked[s][r])
        pool.push_back({static_cast<uint8_t>(r), static_cast<Suit>(s)});

  int p = pool.size();
  std::vector<std::array<Card, 2>> result;
  result.reserve(p * (p-1) / 2);

  for (size_t a = 0; a < pool.size(); a++)
    for (size_t b = a + 1; b < pool.size(); b++)
      result.push_back({pool[a], pool[b]});

  return result;
}

inline HandRank rankFewCards(const std::vector<Card>& cards) {
  uint8_t cnt[15] = {};
  for (const Card& c : cards) cnt[c.rank]++;

  std::vector<std::pair<int, uint8_t>> groups;
  for (int r = 14; r >= 2; --r)
    if (cnt[r]) groups.push_back({cnt[r], static_cast<uint8_t>(r)});

  std::stable_sort(groups.begin(), groups.end(),
    [](const auto& x, const auto& y) { return x.first > y.first; }
  );

  HandCategory cat = HandCategory::HighCard;
  if (groups[0].first == 4)       
    cat = HandCategory::FourOfAKind;
  else if (groups[0].first == 3)  
    cat = HandCategory::ThreeOfAKind;
  else if (groups[0].first == 2)
    cat = (groups.size() > 1 && groups[1].first == 2) ? HandCategory::TwoPair : HandCategory::OnePair;

  HandRank result{};
  result.category = cat;
  for (size_t i = 0; i < groups.size() && i < 5; ++i) 
    result.kickers[i] = groups[i].second;
  return result;
}

inline HandRank rankHoleOnBoard(const std::array<Card, 2>& hole, const std::vector<Card>& comm) {

  std::vector<Card> all = {hole[0], hole[1]};
  all.insert(all.end(), comm.begin(), comm.end());

  if (all.size() < 5) return rankFewCards(all);
  if (all.size() == 7)
    return evaluateTexas({all[0], all[1], all[2], all[3], all[4], all[5], all[6]});
  if (all.size() == 5)
    return evaluateFive({all[0], all[1], all[2], all[3], all[4]});

  HandRank best{};
  for (size_t skip = 0; skip < all.size(); ++skip) {
    std::array<Card, 5> five;
    size_t n = 0;
    for (size_t i = 0; i < all.size(); ++i)
      if (i != skip) five[n++] = all[i];
    const HandRank current = evaluateFive(five);
    if (best < current) best = current;
  }
  return best;
}

inline std::vector<std::array<Card, 2>> sortPossibleHands(const std::vector<std::array<Card, 2>>& possibleHands, const std::vector<Card>& comm) {

  std::vector<std::pair<HandRank, std::array<Card, 2>>> scored;
  scored.reserve(possibleHands.size());
  for (const auto& h : possibleHands)
    scored.push_back({rankHoleOnBoard(h, comm), h});

  std::stable_sort(scored.begin(), scored.end(),
    [](const auto& x, const auto& y) { return y.first < x.first; });

  std::vector<std::array<Card, 2>> sorted;
  sorted.reserve(scored.size());
  for (const auto& s : scored) sorted.push_back(s.second);
  return sorted;
}

inline int findHandRank(const std::vector<Card>& target, const std::vector<std::array<Card,2>>& hands){
  for(size_t i = 0; i < hands.size(); i++) {
    const auto& h = hands[i];

    bool ordered = (h[0] == target[0] && (h[1] == target[1]));
    bool reversed = (h[0] == target[1] && (h[1] == target[0]));

    if(ordered || reversed) return i;
  }
  return -1;
}

inline double getHandWeight(const std::vector<Card>& target, const std::vector<std::array<Card,2>>& hands){
  int rank = findHandRank(target, hands);

  if(rank == -1 || hands.empty()) return -1.0;

  return 1.0 - static_cast<double>(rank) / hands.size();
}


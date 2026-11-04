#pragma once

#include <cstdint>
#include <array>
#include <algorithm>
#include <functional>
#include <vector>
#include <utility>

#include "card.hpp"
#include "hand.hpp"

enum class HandCategory : uint8_t {
  None,
  HighCard,
  OnePair,
  TwoPair,
  ThreeOfAKind,
  Straight,
  Flush,
  FullHouse,
  FourOfAKind,
  StraightFlush,
  RoyalFlush
};

struct HandRank {
  HandCategory category = HandCategory::None;
  std::array<uint8_t, 5> kickers{};

  bool operator==(const HandRank& other) const = default;

  bool operator<(const HandRank& other) const {
    if (category != other.category) {
        return category < other.category;
    }
    return kickers < other.kickers;
  }

  bool operator>(const HandRank& other) const {
    return other < *this;
  }
};


HandRank make_rank(HandCategory category, std::initializer_list<uint8_t> values) {
    HandRank result{};
    result.category = category;

    size_t i = 0;
    for (uint8_t value : values) {
        result.kickers[i++] = value;
    }

    return result;
}


using RankCounts = std::array<int, 15>;


uint8_t getStraightHigh(const RankCounts& counts) {
    for (int high = 14; high >= 6; high--) {
        bool straight = true;
        for (int rank = high; rank > high - 5; rank--) {
            if (counts[rank] == 0) {
                straight = false;
                break;
            }
        }
        if (straight) return static_cast<uint8_t>(high);
    }

    if (counts[14] && counts[2] && counts[3] && counts[4] &&counts[5]) {
        return 5; // wheel: A-2-3-4-5
    }

    return 0;
}


bool checkFlush(const std::array<Card, 5>& cards) {
    for (size_t i = 1; i < 5; ++i) {
        if (cards[i].suit != cards[0].suit) {
            return false;
        }
    }
    return true;
}

HandCategory getBestGroup(std::vector<std::pair<int, int>>& groups) {
  if (groups[0].first == 4) return HandCategory::FourOfAKind;
  if (groups[0].first == 3 && groups[1].first == 2) return HandCategory::FullHouse;
  if (groups[0].first == 3) return HandCategory::ThreeOfAKind;
  if (groups[0].first == 2 && groups[1].first == 2) return HandCategory::TwoPair;
  if (groups[0].first == 2) return HandCategory::OnePair;

  return HandCategory::None;
}


HandRank evaluateFive(const std::array<Card, 5>& cards) {
    RankCounts countRankFreq{};
    std::array<uint8_t, 5> ranks{};

    for (size_t i = 0; i < 5; ++i) { //Count rank frequencies.
        const auto rank = cards[i].rank;
        ranks[i] = rank;
        countRankFreq[rank]++;
    }

    std::sort(ranks.begin(), ranks.end(), std::greater<>());

    bool flush = checkFlush(cards);
    const uint8_t straight_high = getStraightHigh(countRankFreq);

    if (flush && straight_high) {

      if(straight_high == 14) {
        return make_rank(
          HandCategory::RoyalFlush,
          {14}
        );
      }

      return make_rank(
        HandCategory::StraightFlush,
        {straight_high}
      );
    }

    std::vector<std::pair<int, int>> groups; // {frequency, rank}
    for (int rank = 14; rank >= 2; rank--) {
        if (countRankFreq[rank] > 0) {
            groups.push_back({countRankFreq[rank], rank});
        }
    }

    std::sort(groups.begin(),groups.end(),[](const auto& a, const auto& b) {
        if (a.first != b.first)
            return a.first > b.first;
        return a.second > b.second;
      }
    );

    HandCategory groupCategory = getBestGroup(groups);

 
    if (groupCategory == HandCategory::FourOfAKind) {
        return make_rank(
            HandCategory::FourOfAKind,
            {
                static_cast<uint8_t>(groups[0].second),
                static_cast<uint8_t>(groups[1].second)
            }
        );
    }


    if (groupCategory == HandCategory::FullHouse) {
        return make_rank(
            HandCategory::FullHouse,
            {
                static_cast<uint8_t>(groups[0].second),
                static_cast<uint8_t>(groups[1].second)
            }
        );
    }

    if (flush) {
        return make_rank(
            HandCategory::Flush,
            {
                ranks[0], ranks[1], ranks[2],
                ranks[3], ranks[4]
            }
        );
    }

    if (straight_high) {
        return make_rank(
            HandCategory::Straight,
            {straight_high}
        );
    }

    if (groupCategory == HandCategory::ThreeOfAKind) {
        return make_rank(
            HandCategory::ThreeOfAKind,
            {
                static_cast<uint8_t>(groups[0].second),
                static_cast<uint8_t>(groups[1].second),
                static_cast<uint8_t>(groups[2].second)
            }
        );
    }

    if (groupCategory == HandCategory::TwoPair) {
      return make_rank(
        HandCategory::TwoPair,
        {
            static_cast<uint8_t>(groups[0].second),
            static_cast<uint8_t>(groups[1].second),
            static_cast<uint8_t>(groups[2].second)
        }
      );
    }

    if (groupCategory == HandCategory::OnePair) {
      return make_rank(
        HandCategory::OnePair,
        {
            static_cast<uint8_t>(groups[0].second),
            static_cast<uint8_t>(groups[1].second),
            static_cast<uint8_t>(groups[2].second),
            static_cast<uint8_t>(groups[3].second)
        }
      );
    }

    return make_rank(
        HandCategory::HighCard,
        {
            ranks[0], ranks[1], ranks[2],
            ranks[3], ranks[4]
        }
    );
}


std::pair<HandRank, std::array<Card, 5>> evaluateTexas(std::array<Card, 7>& available){
  HandRank best{};
  std::array<Card, 5> winningCards;

  for (int a = 0; a < 7; ++a) {
    for (int b = a + 1; b < 7; ++b) {
      for (int c = b + 1; c < 7; ++c) {
        for (int d = c + 1; d < 7; ++d) {
          for (int e = d + 1; e < 7; ++e) {
            std::array<Card, 5> candidate = {
                available[a],
                available[b],
                available[c],
                available[d],
                available[e]
            };

            HandRank current = evaluateFive(candidate);

            if (best < current) {
              best = current;
              winningCards = candidate;
            }
          }
        }
      }
    }
  }

  return {best, winningCards};
}


std::pair<HandRank, std::array<Card, 5>> evaluateOmaha(const std::vector<Card>& hole, const std::vector<Card>& comm){
  HandRank best{};
  std::array<Card, 5> winningCards;
  
  for (int h1 = 0; h1 < 4; ++h1) {
    for (int h2 = h1 + 1; h2 < 4; ++h2) {
      for (int b1 = 0; b1 < 5; ++b1) {
        for (int b2 = b1 + 1; b2 < 5; ++b2) {
          for (int b3 = b2 + 1; b3 < 5; ++b3) {
            std::array<Card, 5> candidate = {
                hole[h1],
                hole[h2],
                comm[b1],
                comm[b2],
                comm[b3]
            };

            HandRank current =
                evaluateFive(candidate);

            if (best < current) {
                best = current;
                winningCards = candidate;
            }
          }
        }
      }
    }
  }

  return {best, winningCards};
}


std::pair<HandRank, std::array<Card, 5>> evaluateHand(const Hand& hand) {
  const auto& hole = hand.getHoleCards();
  const auto& comm = hand.getCommCards();

  if (comm.size() != 5) throw std::runtime_error("Invalid Community Cards");

  if (hand.getVariant() == Variant::TexasHoldem) {
    std::array<Card, 7> available;
    for(int i = 0; i < 2; i++) available[i] = hole[i];
    for(int i = 0; i < 5; i++) available[i+2] = comm[i];

    return evaluateTexas(available);
  }

  else if (hand.getVariant() == Variant::Omaha) {
    return evaluateOmaha(hole, comm);
  }

  return {make_rank(HandCategory::None,{0}), std::array<Card, 5>{}};
}
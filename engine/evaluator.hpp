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

  auto operator<=>(const HandRank& other) const = default;
};


inline HandRank make_rank(HandCategory category, std::initializer_list<uint8_t> values) {
    HandRank result{};
    result.category = category;

    size_t i = 0;
    for (uint8_t value : values) {
        result.kickers[i++] = value;
    }

    return result;
}


using RankCounts = std::array<int, 15>;


inline uint8_t getStraightHigh(uint16_t m) { // bit r set if rank r present
  if (m & (1u << 14)) m |= 1u << 1; // (wheel)
  for (int h = 14; h >= 5; --h) {
    const uint16_t need = 0x1Fu << (h - 4);
    if ((m & need) == need) return static_cast<uint8_t>(h);
  }
  return 0;
}


inline int checkFlush(const int (&suitCnt)[4]) {
  for (int s = 0; s < 4; ++s) if (suitCnt[s] >= 5) return s;
  return -1;
}


inline HandRank evaluateFive(const std::array<Card, 5>& cards) {

    uint16_t suitMask[4] = {};
    uint16_t all = 0;
    int suitCnt[4] = {};
    uint8_t cnt[15] = {};
    uint8_t kickers[5] = {};

    for (const Card& c : cards) {
      const int s = static_cast<int>(c.suit);
        cnt[c.rank]++;
        suitCnt[s]++;
        suitMask[s] |= 1u << c.rank;
        all |= 1u << c.rank;
    }

    int flush = checkFlush(suitCnt);

    if (flush >= 0) {
      if(const uint8_t straightH = getStraightHigh(suitMask[flush])) {
        return make_rank(
          straightH == 14 ? HandCategory::RoyalFlush : HandCategory::StraightFlush,
          {straightH}
        );
      }
    }

    uint8_t quad = 0, trips=0, pairs[2] = {};
    int npairs = 0;
    for (int r = 14; r >= 2; r--) {
      switch (cnt[r]) {
        case 4: quad = static_cast<uint8_t>(r); break;
        case 3: trips = static_cast<uint8_t>(r); break;
        case 2: pairs[npairs++] = static_cast<uint8_t>(r); break;
        default: break;
      }
    }


    auto top = [&](uint8_t skip1, uint8_t skip2, int n) {
      int i = 0;
      for (int r = 14; r >= 2 && i < n; --r)
        if (cnt[r] && r != skip1 && r != skip2) kickers[i++] = static_cast<uint8_t>(r);
    };

    if (quad) {
      top(quad, 0, 1);
      return make_rank(HandCategory::FourOfAKind, {quad, kickers[0]});
    }


    if (trips && npairs >= 1) {
      return make_rank(HandCategory::FullHouse, {trips, pairs[0]});
    }

    if (flush >= 0) {
      int n = 0;
      for (int r = 14; r >= 2 && n < 5; --r)
        if (suitMask[flush] & (1u << r)) kickers[n++] = static_cast<uint8_t>(r);
      return make_rank(HandCategory::Flush, {kickers[0], kickers[1], kickers[2], kickers[3], kickers[4]});
    }

    if (const uint8_t h = getStraightHigh(all))
      return make_rank(HandCategory::Straight, {h});

    if (trips) {
      top(trips, 0, 2);
      return make_rank(HandCategory::ThreeOfAKind, {trips, kickers[0], kickers[1]});
    }

    if (npairs >= 2) {
      top(pairs[0], pairs[1], 1);
      return make_rank(HandCategory::TwoPair, {pairs[0], pairs[1], kickers[0]});
    }

    if (npairs == 1) {
      top(pairs[0], 0, 3);
      return make_rank(HandCategory::OnePair, {pairs[0], kickers[0], kickers[1], kickers[2]});
    }

    top(0, 0, 5);
    return make_rank(HandCategory::HighCard, {kickers[0], kickers[1], kickers[2], kickers[3], kickers[4]});
}


inline HandRank evaluateTexas(const std::array<Card, 7>& available){
  HandRank best{};

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
            }
          }
        }
      }
    }
  }

  return best;
}


inline HandRank evaluateOmaha(const std::vector<Card>& hole, const std::vector<Card>& comm){
  HandRank best{};
  
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
            }
          }
        }
      }
    }
  }

  return best;
}


inline HandRank evaluateHand(const std::vector<Card>& comm,const std::vector<Card>& hole, Variant var) {
  if (comm.size() != 5) throw std::runtime_error("Invalid Community Cards");

  if (var == Variant::TexasHoldem) {
    std::array<Card, 7> available;
    for(int i = 0; i < 2; i++) available[i] = hole[i];
    for(int i = 0; i < 5; i++) available[i+2] = comm[i];

    return evaluateTexas(available);
  }

  else if (var == Variant::Omaha) {
    return evaluateOmaha(hole, comm);
  }

  return make_rank(HandCategory::None,{0});
}


HandRank evaluateHand(const Hand& hand) {
  const auto& hole = hand.getHoleCards();
  const auto& comm = hand.getCommCards();

  return evaluateHand(comm, hole, hand.getVariant());
}
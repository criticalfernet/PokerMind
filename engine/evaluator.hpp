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


inline HandRank evaluateTexas(const std::array<Card, 7>& cards){
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

  uint8_t quad = 0, trips[2]={}, pairs[4] = {};
  int npairs = 0,ntrips = 0;
  for (int r = 14; r >= 2; r--) {
    switch (cnt[r]) {
      case 4: quad = static_cast<uint8_t>(r); break;
      case 3: if (ntrips < 2) trips[ntrips++] = static_cast<uint8_t>(r); break;
      case 2: if (npairs < 3) pairs[npairs++] = static_cast<uint8_t>(r); break;
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


  if (ntrips >= 1 && (ntrips >= 2 || npairs >= 1)) {
    const uint8_t kick = std::max(trips[1], pairs[0]);
    return make_rank(HandCategory::FullHouse, {trips[0], kick});
  }

  if (flush >= 0) {
    int n = 0;
    for (int r = 14; r >= 2 && n < 5; --r)
      if (suitMask[flush] & (1u << r)) kickers[n++] = static_cast<uint8_t>(r);
    return make_rank(HandCategory::Flush, {kickers[0], kickers[1], kickers[2], kickers[3], kickers[4]});
  }

  if (const uint8_t h = getStraightHigh(all))
    return make_rank(HandCategory::Straight, {h});

  if (ntrips) {
    top(trips[0], 0, 2);
    return make_rank(HandCategory::ThreeOfAKind, {trips[0], kickers[0], kickers[1]});
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


inline HandRank evaluateOmaha(const std::vector<Card>& hole,const std::vector<Card>& comm) {
  static constexpr std::array<std::array<uint8_t, 2>, 6> holePairs = {{
    {{0, 1}},
    {{0, 2}},
    {{0, 3}},
    {{1, 2}},
    {{1, 3}},
    {{2, 3}}
  }};

  static constexpr std::array<std::array<uint8_t, 3>, 10> boardTriples = {{
    {{0, 1, 2}},
    {{0, 1, 3}},
    {{0, 1, 4}},
    {{0, 2, 3}},
    {{0, 2, 4}},
    {{0, 3, 4}},
    {{1, 2, 3}},
    {{1, 2, 4}},
    {{1, 3, 4}},
    {{2, 3, 4}}
  }};

  if (hole.size() != 4 || comm.size() != 5)
    throw std::runtime_error("Invalid Omaha cards");

  HandRank best{};

  for (const auto& hp : holePairs) {
    const Card& h1 = hole[hp[0]];
    const Card& h2 = hole[hp[1]];

    for (const auto& bt : boardTriples) {

      const std::array<Card, 5> candidate = {{
        h1,
        h2,
        comm[bt[0]],
        comm[bt[1]],
        comm[bt[2]]
      }};

      const HandRank current = evaluateFive(candidate);

      if (best < current)
        best = current;
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


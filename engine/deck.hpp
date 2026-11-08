#pragma once

#include <array>
#include <vector>
#include <random>
#include <cstdint>
#include <algorithm>

#include "card.hpp"

class Deck {
  std::array<Card, 52> cards;
  size_t index = 0;
  size_t size;

public:
  Deck() {
    size_t i = 0;
    size = 52;
    for (int suit = 0; suit < 4; suit++){
      for (int rank = 2; rank <= 14; rank++){
        cards[i++] = {
          static_cast<uint8_t>(rank),
          static_cast<Suit>(suit)
        };
      }
    }
  }

  Deck(const std::vector<Card>& excluded) {
    size = 52 - excluded.size();
    bool blocked[4][15] = {};
    for(const Card& c: excluded) {
      int suit = static_cast<int>(c.suit);
      blocked[suit][c.rank] = true;
    }

    size_t i = 0;
    for (int suit = 0; suit < 4; suit++){
      for (int rank = 2; rank <= 14; rank++){
        if(blocked[suit][rank]) continue;
        cards[i++] = {
          static_cast<uint8_t>(rank),
          static_cast<Suit>(suit)
        };
      }
    }

  }

  Card draw() {
    if (index >= size) throw std::runtime_error("Exhausted");
    return cards[index++];
  }

  void shuffle() {
    static thread_local std::mt19937 rng(std::random_device{}());
    std::shuffle(cards.begin(), cards.begin() + size, rng);
    index = 0;
  }

  void remove(const Card& c) {
    for (size_t i = index; i < size; ++i) {
      if (cards[i].rank == c.rank && cards[i].suit == c.suit) {
        cards[i] = cards[size - 1];
        --size;
        return;
      }
    }
  }
};
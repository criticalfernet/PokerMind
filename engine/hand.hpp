#pragma once

#include <vector>
#include <cstdint>

#include "deck.hpp"

enum class Variant : uint8_t {
  TexasHoldem,
  Omaha
};

class Hand{
  Variant variant;
  size_t limit;
  std::vector<Card> holeCards;
  std::vector<Card> CommCards;
  bool folded;

public:
  explicit Hand(Variant v) : variant(v) {
    folded = false;
    limit = variant == Variant::TexasHoldem ? 2 : 4;
    holeCards.reserve(limit);
    CommCards.reserve(5);
  }

  Hand(const std::vector<Card>& hole, const std::vector<Card>& comm) : 
    Hand(Variant::TexasHoldem){
      holeCards = hole;
      CommCards = comm;
    }

  Variant getVariant() const{
    return variant;
  }

  const std::vector<Card>& getHoleCards() const{
    return holeCards;
  }

  const std::vector<Card>& getCommCards() const{
    return CommCards;
  }

  uint8_t getHoleCardsCount() const{
    return holeCards.size();
  }

  uint8_t getCommCardsCount() const{
    return CommCards.size();
  }

  bool getFolded() const{
    return folded;
  }

  void addHoleCard(const Card& card){
    if (holeCards.size() < limit) holeCards.push_back(card);
  }

  void addCommCard(const Card& card){
    if (CommCards.size() < 5) CommCards.push_back(card);
  }

  bool completeHoleCards() {
    return holeCards.size() == limit;
  }

  void fold() {
    folded = true;
  }
};
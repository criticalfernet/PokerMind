#pragma once
#include <cstdint>
#include <iostream>

enum class Suit {
  Clubs,
  Diamonds,
  Hearts,
  Spades
};

struct Card {
  uint8_t rank;
  Suit suit;
};

bool operator==(const Card& c1, const Card& c2) {
  return (c1.rank == c2.rank) && (c1.suit == c2.suit);
}

std::ostream& operator<<(std::ostream& os, const Card& card) {
  if (card.rank >= 2 && card.rank <= 10) {
    os << static_cast<int>(card.rank);
  } else {
    switch (card.rank) {
      case 11: os << 'J'; break;
      case 12: os << 'Q'; break;
      case 13: os << 'K'; break;
      case 14: os << 'A'; break;
      default: os << '?'; break;
    }
  }

  switch(card.suit) {
    case Suit::Clubs: os << "♣"; break;
    case Suit::Diamonds: os << "♦"; break;
    case Suit::Hearts: os << "♥"; break;
    case Suit::Spades: os << "♠"; break;
  }
  
  return os;
}
#include "stackrisk/card.hpp"

#include <cctype>
#include <stdexcept>

namespace stackrisk {
namespace {

char upper(char character) {
    return static_cast<char>(
        std::toupper(static_cast<unsigned char>(character)));
}

char lower(char character) {
    return static_cast<char>(
        std::tolower(static_cast<unsigned char>(character)));
}

std::optional<Rank> parse_rank(char character) {
    switch (upper(character)) {
        case '2': return Rank::two;
        case '3': return Rank::three;
        case '4': return Rank::four;
        case '5': return Rank::five;
        case '6': return Rank::six;
        case '7': return Rank::seven;
        case '8': return Rank::eight;
        case '9': return Rank::nine;
        case 'T': return Rank::ten;
        case 'J': return Rank::jack;
        case 'Q': return Rank::queen;
        case 'K': return Rank::king;
        case 'A': return Rank::ace;
        default: return std::nullopt;
    }
}

char rank_character(Rank rank) {
    if (rank >= Rank::two && rank <= Rank::nine) {
        return static_cast<char>('0' + value(rank));
    }
    switch (rank) {
        case Rank::ten: return 'T';
        case Rank::jack: return 'J';
        case Rank::queen: return 'Q';
        case Rank::king: return 'K';
        case Rank::ace: return 'A';
        default: throw std::invalid_argument("invalid card rank");
    }
}

}  // namespace

std::optional<Card> Card::parse(std::string_view text) {
    if (text.size() != 2) return std::nullopt;

    const auto rank = parse_rank(text[0]);
    if (!rank) return std::nullopt;

    Suit suit;
    switch (lower(text[1])) {
        case 'c': suit = Suit::clubs; break;
        case 'd': suit = Suit::diamonds; break;
        case 'h': suit = Suit::hearts; break;
        case 's': suit = Suit::spades; break;
        default: return std::nullopt;
    }
    return Card{*rank, suit};
}

std::string Card::to_string() const {
    char suit_character;
    switch (suit) {
        case Suit::clubs: suit_character = 'c'; break;
        case Suit::diamonds: suit_character = 'd'; break;
        case Suit::hearts: suit_character = 'h'; break;
        case Suit::spades: suit_character = 's'; break;
        default: throw std::invalid_argument("invalid card suit");
    }
    return {rank_character(rank), suit_character};
}

std::array<Card, 52> make_deck() {
    std::array<Card, 52> deck{};
    std::size_t index = 0;
    for (int rank = value(Rank::two); rank <= value(Rank::ace); ++rank) {
        for (const Suit suit : {Suit::clubs, Suit::diamonds, Suit::hearts,
                                Suit::spades}) {
            deck[index++] = {static_cast<Rank>(rank), suit};
        }
    }
    return deck;
}

}  // namespace stackrisk

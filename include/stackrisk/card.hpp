#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace stackrisk {

enum class Suit : std::uint8_t { clubs, diamonds, hearts, spades };
enum class Rank : std::uint8_t {
    two = 2,
    three,
    four,
    five,
    six,
    seven,
    eight,
    nine,
    ten,
    jack,
    queen,
    king,
    ace
};

struct Card {
    Rank rank;
    Suit suit;

    [[nodiscard]] static std::optional<Card> parse(std::string_view text);
    [[nodiscard]] std::string to_string() const;
    auto operator<=>(const Card&) const = default;
};

[[nodiscard]] std::array<Card, 52> make_deck();
[[nodiscard]] constexpr int value(Rank rank) {
    return static_cast<int>(rank);
}

}  // namespace stackrisk

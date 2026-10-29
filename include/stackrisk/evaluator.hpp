#pragma once

#include "stackrisk/card.hpp"

#include <array>
#include <compare>
#include <cstdint>
#include <span>

namespace stackrisk {

enum class Category : std::uint8_t {
    high_card,
    one_pair,
    two_pair,
    three_of_a_kind,
    straight,
    flush,
    full_house,
    four_of_a_kind,
    straight_flush
};

struct HandRank {
    Category category = Category::high_card;
    std::array<std::uint8_t, 5> kickers{};
    auto operator<=>(const HandRank&) const = default;
};

[[nodiscard]] HandRank evaluate_five(std::span<const Card> cards);
[[nodiscard]] HandRank evaluate_best(std::span<const Card> cards);
[[nodiscard]] const char* category_name(Category category);

}  // namespace stackrisk

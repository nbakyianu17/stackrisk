#include "stackrisk/evaluator.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace stackrisk {
namespace {

using RankCounts = std::array<int, 15>;

std::array<std::uint8_t, 5> as_kickers(
    std::initializer_list<int> values) {
    std::array<std::uint8_t, 5> kickers{};
    std::size_t index = 0;
    for (const int rank : values) {
        kickers[index++] = static_cast<std::uint8_t>(rank);
    }
    return kickers;
}

std::uint16_t rank_mask(const RankCounts& counts) {
    std::uint16_t mask = 0;
    for (int rank = 2; rank <= 14; ++rank) {
        if (counts[rank] > 0) {
            mask |= static_cast<std::uint16_t>(1U << rank);
        }
    }
    return mask;
}

int straight_high(std::uint16_t mask) {
    for (int high = 14; high >= 6; --high) {
        const auto run = static_cast<std::uint16_t>(0x1FU << (high - 4));
        if ((mask & run) == run) return high;
    }

    constexpr std::uint16_t wheel =
        static_cast<std::uint16_t>((1U << 14) | (1U << 5) | (1U << 4) |
                                   (1U << 3) | (1U << 2));
    return (mask & wheel) == wheel ? 5 : 0;
}

std::array<int, 2> highest_groups(const RankCounts& counts, int size,
                                  int excluded = 0) {
    std::array<int, 2> groups{};
    for (int rank = 14; rank >= 2; --rank) {
        if (rank != excluded && counts[rank] == size) {
            if (groups[0] == 0) {
                groups[0] = rank;
            } else {
                groups[1] = rank;
                break;
            }
        }
    }
    return groups;
}

std::array<int, 5> descending_ranks(const RankCounts& counts,
                                    int excluded_a = 0, int excluded_b = 0,
                                    int excluded_c = 0) {
    std::array<int, 5> ranks{};
    std::size_t index = 0;
    for (int rank = 14; rank >= 2 && index < ranks.size(); --rank) {
        if (rank != excluded_a && rank != excluded_b && rank != excluded_c &&
            counts[rank] > 0) {
            ranks[index++] = rank;
        }
    }
    return ranks;
}

void validate_unique(std::span<const Card> cards) {
    for (std::size_t left = 0; left < cards.size(); ++left) {
        for (std::size_t right = left + 1; right < cards.size(); ++right) {
            if (cards[left] == cards[right]) {
                throw std::invalid_argument("duplicate card: " +
                                            cards[left].to_string());
            }
        }
    }
}

}  // namespace

HandRank evaluate_five(std::span<const Card> cards) {
    if (cards.size() != 5) {
        throw std::invalid_argument("exactly five cards are required");
    }
    validate_unique(cards);

    RankCounts counts{};
    std::array<int, 4> suits{};
    for (const Card card : cards) {
        const int rank = value(card.rank);
        const auto suit = static_cast<std::size_t>(card.suit);
        if (rank < 2 || rank > 14 || suit >= suits.size()) {
            throw std::invalid_argument("invalid card");
        }
        ++counts[rank];
        ++suits[suit];
    }

    const int straight = straight_high(rank_mask(counts));
    const bool flush =
        std::ranges::any_of(suits, [](int count) { return count == 5; });

    if (flush && straight) {
        return {Category::straight_flush,
                as_kickers({straight, straight - 1, straight - 2, straight - 3,
                            straight - 4})};
    }

    const auto quads = highest_groups(counts, 4);
    if (quads[0] != 0) {
        const auto rest = descending_ranks(counts, quads[0]);
        return {Category::four_of_a_kind,
                as_kickers({quads[0], quads[0], quads[0], quads[0], rest[0]})};
    }

    const auto trips = highest_groups(counts, 3);
    if (trips[0] != 0) {
        const auto pairs = highest_groups(counts, 2, trips[0]);
        const int pair = std::max(pairs[0], trips[1]);
        if (pair != 0) {
            return {Category::full_house,
                    as_kickers({trips[0], trips[0], trips[0], pair, pair})};
        }
    }

    if (flush) {
        const auto ranks = descending_ranks(counts);
        return {Category::flush,
                as_kickers({ranks[0], ranks[1], ranks[2], ranks[3], ranks[4]})};
    }

    if (straight) {
        return {Category::straight,
                as_kickers({straight, straight - 1, straight - 2, straight - 3,
                            straight - 4})};
    }

    if (trips[0] != 0) {
        const auto rest = descending_ranks(counts, trips[0]);
        return {Category::three_of_a_kind,
                as_kickers({trips[0], trips[0], trips[0], rest[0], rest[1]})};
    }

    const auto pairs = highest_groups(counts, 2);
    if (pairs[1] != 0) {
        const auto rest = descending_ranks(counts, pairs[0], pairs[1]);
        return {Category::two_pair,
                as_kickers({pairs[0], pairs[0], pairs[1], pairs[1], rest[0]})};
    }

    if (pairs[0] != 0) {
        const auto rest = descending_ranks(counts, pairs[0]);
        return {Category::one_pair,
                as_kickers({pairs[0], pairs[0], rest[0], rest[1], rest[2]})};
    }

    const auto ranks = descending_ranks(counts);
    return {Category::high_card,
            as_kickers({ranks[0], ranks[1], ranks[2], ranks[3], ranks[4]})};
}

HandRank evaluate_best(std::span<const Card> cards) {
    if (cards.size() < 5 || cards.size() > 7) {
        throw std::invalid_argument("five to seven cards are required");
    }
    validate_unique(cards);

    HandRank best{};
    bool has_best = false;
    for (std::size_t a = 0; a < cards.size() - 4; ++a) {
        for (std::size_t b = a + 1; b < cards.size() - 3; ++b) {
            for (std::size_t c = b + 1; c < cards.size() - 2; ++c) {
                for (std::size_t d = c + 1; d < cards.size() - 1; ++d) {
                    for (std::size_t e = d + 1; e < cards.size(); ++e) {
                        const std::array<Card, 5> hand{
                            cards[a], cards[b], cards[c], cards[d], cards[e]};
                        const auto candidate = evaluate_five(hand);
                        if (!has_best || candidate > best) {
                            best = candidate;
                            has_best = true;
                        }
                    }
                }
            }
        }
    }
    return best;
}

const char* category_name(Category category) {
    switch (category) {
        case Category::high_card: return "High card";
        case Category::one_pair: return "One pair";
        case Category::two_pair: return "Two pair";
        case Category::three_of_a_kind: return "Three of a kind";
        case Category::straight: return "Straight";
        case Category::flush: return "Flush";
        case Category::full_house: return "Full house";
        case Category::four_of_a_kind: return "Four of a kind";
        case Category::straight_flush: return "Straight flush";
    }
    throw std::invalid_argument("invalid hand category");
}

}  // namespace stackrisk

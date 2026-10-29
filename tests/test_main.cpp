#include "stackrisk/card.hpp"
#include "stackrisk/evaluator.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

using stackrisk::Card;
using stackrisk::Category;

int failures = 0;

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::cerr << __FILE__ << ':' << __LINE__                            \
                      << ": check failed: " #condition "\n";                    \
            ++failures;                                                        \
        }                                                                       \
    } while (false)

Card card(std::string_view text) {
    const auto parsed = Card::parse(text);
    if (!parsed) throw std::runtime_error("invalid test card");
    return *parsed;
}

template <std::size_t Size>
stackrisk::HandRank best(const std::array<Card, Size>& cards) {
    return stackrisk::evaluate_best(cards);
}

void cards_parse_and_round_trip() {
    for (const std::string_view notation : {"2c", "9d", "Ts", "Jh", "Qc",
                                            "Kd", "As"}) {
        const auto parsed = Card::parse(notation);
        CHECK(parsed.has_value());
        CHECK(parsed->to_string() == notation);
    }
    CHECK(Card::parse("AS")->to_string() == "As");
    CHECK(!Card::parse("1s"));
    CHECK(!Card::parse("10s"));
    CHECK(!Card::parse("Ax"));
    CHECK(!Card::parse(""));

    const auto deck = stackrisk::make_deck();
    CHECK(deck.size() == 52);
    for (std::size_t left = 0; left < deck.size(); ++left) {
        for (std::size_t right = left + 1; right < deck.size(); ++right) {
            CHECK(deck[left] != deck[right]);
        }
    }
}

void recognizes_all_categories() {
    CHECK(best(std::array{card("As"), card("Kd"), card("Qh"), card("Jc"),
                          card("9s"), card("2d"), card("3c")})
              .category == Category::high_card);
    CHECK(best(std::array{card("As"), card("Ad"), card("Qh"), card("Jc"),
                          card("9s"), card("2d"), card("3c")})
              .category == Category::one_pair);
    CHECK(best(std::array{card("As"), card("Ad"), card("Kh"), card("Kc"),
                          card("9s"), card("2d"), card("3c")})
              .category == Category::two_pair);
    CHECK(best(std::array{card("As"), card("Ad"), card("Ah"), card("Kc"),
                          card("9s"), card("2d"), card("3c")})
              .category == Category::three_of_a_kind);
    CHECK(best(std::array{card("As"), card("Kd"), card("Qh"), card("Jc"),
                          card("Ts"), card("2d"), card("3c")})
              .category == Category::straight);
    CHECK(best(std::array{card("2h"), card("5h"), card("8h"), card("Jh"),
                          card("Kh"), card("3c"), card("4d")})
              .category == Category::flush);
    CHECK(best(std::array{card("Ac"), card("Ad"), card("Ah"), card("Ks"),
                          card("Kd"), card("2c"), card("3d")})
              .category == Category::full_house);
    CHECK(best(std::array{card("Ac"), card("Ad"), card("Ah"), card("As"),
                          card("Kd"), card("2c"), card("3d")})
              .category == Category::four_of_a_kind);
    CHECK(best(std::array{card("As"), card("Ks"), card("Qs"), card("Js"),
                          card("Ts"), card("2d"), card("3c")})
              .category == Category::straight_flush);
}

void handles_wheel_and_tie_breakers() {
    const auto wheel = best(std::array{card("As"), card("2d"), card("3c"),
                                       card("4h"), card("5s"), card("Kd"),
                                       card("Qc")});
    CHECK(wheel.category == Category::straight);
    CHECK(wheel.kickers[0] == 5);

    const auto aces = best(std::array{card("As"), card("Ad"), card("Kh"),
                                      card("Qc"), card("Js"), card("2d"),
                                      card("3c")});
    const auto kings = best(std::array{card("Ks"), card("Kd"), card("Ah"),
                                       card("Qc"), card("Js"), card("2d"),
                                       card("3c")});
    CHECK(aces > kings);

    const auto ace_high_straight =
        best(std::array{card("As"), card("Kd"), card("Qh"), card("Jc"),
                        card("Ts"), card("2d"), card("3c")});
    CHECK(ace_high_straight.kickers[0] == 14);
}

void rejects_invalid_hands() {
    const std::array duplicate{card("As"), card("As"), card("Qs"), card("Js"),
                               card("Ts")};
    bool duplicate_rejected = false;
    try {
        static_cast<void>(stackrisk::evaluate_best(duplicate));
    } catch (const std::invalid_argument&) {
        duplicate_rejected = true;
    }
    CHECK(duplicate_rejected);

    const std::array too_few{card("As"), card("Ks"), card("Qs"), card("Js")};
    bool size_rejected = false;
    try {
        static_cast<void>(stackrisk::evaluate_best(too_few));
    } catch (const std::invalid_argument&) {
        size_rejected = true;
    }
    CHECK(size_rejected);
}

}  // namespace

int main() {
    cards_parse_and_round_trip();
    recognizes_all_categories();
    handles_wheel_and_tie_breakers();
    rejects_invalid_hands();
    if (failures != 0) return EXIT_FAILURE;
    std::cout << "StackRisk evaluator tests passed\n";
    return EXIT_SUCCESS;
}

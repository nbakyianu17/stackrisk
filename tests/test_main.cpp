#include "stackrisk/card.hpp"
#include "stackrisk/evaluator.hpp"
#include "stackrisk/simulation.hpp"

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

    const auto straight_needs_cards_after_the_first_five =
        best(std::array{card("8c"), card("Qd"), card("Jc"), card("2h"),
                        card("4s"), card("9s"), card("Th")});
    CHECK(straight_needs_cards_after_the_first_five.category ==
          Category::straight);
    CHECK(straight_needs_cards_after_the_first_five.kickers[0] == 12);

    const auto pair_with_king_kicker =
        stackrisk::evaluate_five(std::array{card("As"), card("Ad"), card("Kc"),
                                            card("9h"), card("4s")});
    const auto pair_with_queen_kicker =
        stackrisk::evaluate_five(std::array{card("Ah"), card("Ac"), card("Qc"),
                                            card("Js"), card("8h")});
    CHECK(pair_with_king_kicker > pair_with_queen_kicker);
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

void seeded_simulation_and_board_split() {
    stackrisk::SimulationRequest request{
        .hero = {card("As"), card("Kd")},
        .opponents = 2,
        .trials = 500,
        .seed = 2026};
    const auto first = stackrisk::simulate_equity(request);
    const auto second = stackrisk::simulate_equity(request);
    request.workers = 4;
    const auto parallel = stackrisk::simulate_equity(request);
    CHECK(first.trials == 500);
    CHECK(first.wins + first.ties + first.losses == 500);
    CHECK(first.wins == second.wins);
    CHECK(first.ties == second.ties);
    CHECK(first.losses == second.losses);
    CHECK(first.equity == second.equity);
    CHECK(first.confidence_low <= first.equity);
    CHECK(first.equity <= first.confidence_high);
    CHECK(first.wins == parallel.wins);
    CHECK(first.ties == parallel.ties);
    CHECK(first.losses == parallel.losses);
    CHECK(first.equity == parallel.equity);
    CHECK(first.confidence_low == parallel.confidence_low);
    CHECK(first.confidence_high == parallel.confidence_high);

    request.hero = {card("2c"), card("3c")};
    request.board = {card("As"), card("Ks"), card("Qs"), card("Js"),
                     card("Ts")};
    request.opponents = 1;
    request.trials = 25;
    const auto board_split = stackrisk::simulate_equity(request);
    CHECK(board_split.wins == 0);
    CHECK(board_split.ties == 25);
    CHECK(board_split.losses == 0);
    CHECK(board_split.equity == 0.5);
}

void rejects_invalid_simulation_requests() {
    stackrisk::SimulationRequest request{
        .hero = {card("As"), card("Kd")},
        .opponents = 1,
        .trials = 10,
        .seed = 1};
    request.board = {card("As"), card("Ks"), card("Qs")};
    bool duplicate_rejected = false;
    try {
        static_cast<void>(stackrisk::simulate_equity(request));
    } catch (const std::invalid_argument&) {
        duplicate_rejected = true;
    }
    CHECK(duplicate_rejected);

    request.board = {card("Qs")};
    bool invalid_board_size_rejected = false;
    try {
        static_cast<void>(stackrisk::simulate_equity(request));
    } catch (const std::invalid_argument&) {
        invalid_board_size_rejected = true;
    }
    CHECK(invalid_board_size_rejected);

    request.board.clear();
    request.trials = 0;
    bool zero_trials_rejected = false;
    try {
        static_cast<void>(stackrisk::simulate_equity(request));
    } catch (const std::invalid_argument&) {
        zero_trials_rejected = true;
    }
    CHECK(zero_trials_rejected);

    request.trials = 1;
    request.opponents = 9;
    bool too_many_opponents_rejected = false;
    try {
        static_cast<void>(stackrisk::simulate_equity(request));
    } catch (const std::invalid_argument&) {
        too_many_opponents_rejected = true;
    }
    CHECK(too_many_opponents_rejected);

    request.opponents = 1;
    request.workers = 0;
    bool invalid_workers_rejected = false;
    try {
        static_cast<void>(stackrisk::simulate_equity(request));
    } catch (const std::invalid_argument&) {
        invalid_workers_rejected = true;
    }
    CHECK(invalid_workers_rejected);
}

}  // namespace

int main() {
    cards_parse_and_round_trip();
    recognizes_all_categories();
    handles_wheel_and_tie_breakers();
    rejects_invalid_hands();
    seeded_simulation_and_board_split();
    rejects_invalid_simulation_requests();
    if (failures != 0) return EXIT_FAILURE;
    std::cout << "StackRisk tests passed\n";
    return EXIT_SUCCESS;
}

#include "stackrisk/simulation.hpp"

#include "stackrisk/evaluator.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>
#include <vector>

namespace stackrisk {

EquityResult simulate_equity(const SimulationRequest& request) {
    if (request.trials == 0) {
        throw std::invalid_argument("trials must be greater than zero");
    }
    if (request.opponents == 0 || request.opponents > 8) {
        throw std::invalid_argument("opponents must be between 1 and 8");
    }
    if (request.board.size() > 5) {
        throw std::invalid_argument("the board cannot contain more than five cards");
    }

    std::vector<Card> known_cards{request.hero[0], request.hero[1]};
    known_cards.insert(known_cards.end(), request.board.begin(),
                       request.board.end());
    for (std::size_t left = 0; left < known_cards.size(); ++left) {
        for (std::size_t right = left + 1; right < known_cards.size(); ++right) {
            if (known_cards[left] == known_cards[right]) {
                throw std::invalid_argument("known cards must be unique");
            }
        }
    }

    std::vector<Card> deck;
    deck.reserve(52 - known_cards.size());
    for (const Card card : make_deck()) {
        if (std::ranges::find(known_cards, card) == known_cards.end()) {
            deck.push_back(card);
        }
    }

    std::mt19937_64 random(request.seed);
    EquityResult result;
    result.trials = request.trials;
    double equity_sum = 0.0;
    double squared_equity_sum = 0.0;

    for (std::uint64_t trial = 0; trial < request.trials; ++trial) {
        std::shuffle(deck.begin(), deck.end(), random);
        std::size_t next = 0;

        std::vector<std::array<Card, 2>> opponent_hands(request.opponents);
        for (auto& hand : opponent_hands) {
            hand = {deck[next], deck[next + 1]};
            next += 2;
        }

        std::array<Card, 5> complete_board{};
        std::copy(request.board.begin(), request.board.end(),
                  complete_board.begin());
        std::size_t board_size = request.board.size();
        while (board_size < complete_board.size()) {
            complete_board[board_size++] = deck[next++];
        }

        const auto rank_seven = [&complete_board](const std::array<Card, 2>& hole) {
            const std::array<Card, 7> cards{
                hole[0], hole[1], complete_board[0], complete_board[1],
                complete_board[2], complete_board[3], complete_board[4]};
            return evaluate_best(cards);
        };

        const auto hero_rank = rank_seven(request.hero);
        auto best_rank = hero_rank;
        std::size_t winners = 1;
        for (const auto& hand : opponent_hands) {
            const auto opponent_rank = rank_seven(hand);
            if (opponent_rank > best_rank) {
                best_rank = opponent_rank;
                winners = 1;
            } else if (opponent_rank == best_rank) {
                ++winners;
            }
        }

        double share = 0.0;
        if (hero_rank == best_rank) {
            if (winners == 1) {
                ++result.wins;
            } else {
                ++result.ties;
            }
            share = 1.0 / static_cast<double>(winners);
        } else {
            ++result.losses;
        }
        equity_sum += share;
        squared_equity_sum += share * share;
    }

    result.equity = equity_sum / static_cast<double>(request.trials);
    if (request.trials > 1) {
        const double count = static_cast<double>(request.trials);
        const double variance = std::max(
            0.0, (squared_equity_sum - equity_sum * equity_sum / count) /
                     (count - 1.0));
        const double margin = 1.96 * std::sqrt(variance / count);
        result.confidence_low = std::max(0.0, result.equity - margin);
        result.confidence_high = std::min(1.0, result.equity + margin);
    } else {
        result.confidence_low = result.equity;
        result.confidence_high = result.equity;
    }
    return result;
}

}  // namespace stackrisk

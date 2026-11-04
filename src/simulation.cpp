#include "stackrisk/simulation.hpp"

#include "stackrisk/evaluator.hpp"

#include <algorithm>
#include <cmath>
#include <future>
#include <random>
#include <stdexcept>
#include <vector>

namespace stackrisk {
namespace {

struct PartialResult {
    EquityResult result;
    double equity_sum = 0.0;
    double squared_equity_sum = 0.0;
};

std::uint64_t trial_seed(std::uint64_t seed, std::uint64_t trial) {
    std::uint64_t value = seed + trial + 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31);
}

PartialResult simulate_range(const SimulationRequest& request,
                             const std::vector<Card>& available_deck,
                             std::uint64_t first_trial,
                             std::uint64_t trial_count) {
    PartialResult partial;
    partial.result.trials = trial_count;

    for (std::uint64_t offset = 0; offset < trial_count; ++offset) {
        std::mt19937_64 random(trial_seed(request.seed, first_trial + offset));
        auto deck = available_deck;
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
                ++partial.result.wins;
            } else {
                ++partial.result.ties;
            }
            share = 1.0 / static_cast<double>(winners);
        } else {
            ++partial.result.losses;
        }
        partial.equity_sum += share;
        partial.squared_equity_sum += share * share;
    }

    return partial;
}

std::vector<Card> available_cards(const SimulationRequest& request) {
    if (request.opponents == 0 || request.opponents > 8) {
        throw std::invalid_argument("opponents must be between 1 and 8");
    }
    if (request.board.size() != 0 && request.board.size() != 3 &&
        request.board.size() != 4 && request.board.size() != 5) {
        throw std::invalid_argument(
            "the board must contain 0, 3, 4, or 5 known cards");
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
    return deck;
}

}  // namespace

EquityResult simulate_equity(const SimulationRequest& request) {
    if (request.trials == 0) {
        throw std::invalid_argument("trials must be greater than zero");
    }
    if (request.workers == 0 || request.workers > 64) {
        throw std::invalid_argument("workers must be between 1 and 64");
    }

    const auto available_deck = available_cards(request);

    const std::size_t workers = static_cast<std::size_t>(
        std::min<std::uint64_t>(request.workers, request.trials));
    const std::uint64_t base_trials = request.trials / workers;
    const std::uint64_t extra_trials = request.trials % workers;
    std::vector<std::future<PartialResult>> jobs;
    jobs.reserve(workers);

    std::uint64_t first_trial = 0;
    for (std::size_t worker = 0; worker < workers; ++worker) {
        const std::uint64_t worker_trials =
            base_trials + (worker < extra_trials ? 1 : 0);
        const auto start = first_trial;
        first_trial += worker_trials;
        jobs.push_back(std::async(
            std::launch::async, simulate_range, std::cref(request),
            std::cref(available_deck), start, worker_trials));
    }

    PartialResult total;
    total.result.trials = request.trials;
    for (auto& job : jobs) {
        const auto partial = job.get();
        total.result.wins += partial.result.wins;
        total.result.ties += partial.result.ties;
        total.result.losses += partial.result.losses;
        total.equity_sum += partial.equity_sum;
        total.squared_equity_sum += partial.squared_equity_sum;
    }

    total.result.equity =
        total.equity_sum / static_cast<double>(request.trials);
    if (request.trials > 1) {
        const double count = static_cast<double>(request.trials);
        const double variance = std::max(
            0.0, (total.squared_equity_sum -
                  total.equity_sum * total.equity_sum / count) /
                     (count - 1.0));
        const double margin = 1.96 * std::sqrt(variance / count);
        total.result.confidence_low =
            std::max(0.0, total.result.equity - margin);
        total.result.confidence_high =
            std::min(1.0, total.result.equity + margin);
    } else {
        total.result.confidence_low = total.result.equity;
        total.result.confidence_high = total.result.equity;
    }
    return total.result;
}

EquityResult exact_river_equity(const SimulationRequest& request) {
    if (request.board.size() != 5) {
        throw std::invalid_argument(
            "exact equity requires a complete five-card river board");
    }
    if (request.opponents != 1) {
        throw std::invalid_argument(
            "exact river equity currently supports exactly one opponent");
    }
    const auto deck = available_cards(request);
    const auto rank_hand = [&request](const std::array<Card, 2>& hole) {
        const std::array<Card, 7> cards{
            hole[0], hole[1], request.board[0], request.board[1],
            request.board[2], request.board[3], request.board[4]};
        return evaluate_best(cards);
    };

    const auto hero_rank = rank_hand(request.hero);
    EquityResult result;
    double equity_sum = 0.0;
    for (std::size_t first = 0; first < deck.size(); ++first) {
        for (std::size_t second = first + 1; second < deck.size(); ++second) {
            ++result.trials;
            const auto opponent_rank =
                rank_hand({deck[first], deck[second]});
            if (hero_rank > opponent_rank) {
                ++result.wins;
                equity_sum += 1.0;
            } else if (hero_rank == opponent_rank) {
                ++result.ties;
                equity_sum += 0.5;
            } else {
                ++result.losses;
            }
        }
    }

    result.equity = equity_sum / static_cast<double>(result.trials);
    result.confidence_low = result.equity;
    result.confidence_high = result.equity;
    return result;
}

}  // namespace stackrisk

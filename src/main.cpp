#include "stackrisk/card.hpp"
#include "stackrisk/simulation.hpp"

#include <charconv>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void print_help() {
    std::cout
        << "StackRisk - Texas Hold'em equity estimator\n\n"
        << "Usage: stackrisk-cli --hero As Kd [options]\n\n"
        << "Options:\n"
        << "  --hero CARD CARD       Required hole cards, e.g. As Kd\n"
        << "  --board CARDS          Known board cards, comma-separated\n"
        << "  --opponents N          Number of opponents, 1-8 (default: 1)\n"
        << "  --trials N             Monte Carlo deals (default: 100000)\n"
        << "  --seed N               Random seed (default: 42)\n"
        << "  --workers N            Parallel workers, 1-64 (default: 1)\n"
        << "  --help                 Show this help\n";
}

std::uint64_t parse_integer(std::string_view text, std::string_view name) {
    std::uint64_t result = 0;
    const auto [end, error] =
        std::from_chars(text.data(), text.data() + text.size(), result);
    if (error != std::errc{} || end != text.data() + text.size()) {
        throw std::invalid_argument(std::string(name) +
                                    " must be a non-negative integer");
    }
    return result;
}

stackrisk::Card parse_card(std::string_view text) {
    const auto card = stackrisk::Card::parse(text);
    if (!card) throw std::invalid_argument("invalid card: " + std::string(text));
    return *card;
}

std::vector<stackrisk::Card> parse_board(std::string_view text) {
    std::vector<stackrisk::Card> cards;
    if (text.empty()) return cards;
    std::size_t start = 0;
    while (start < text.size()) {
        const auto end = text.find(',', start);
        const auto token = text.substr(
            start, end == std::string_view::npos ? text.size() - start
                                                 : end - start);
        if (token.empty()) {
            throw std::invalid_argument("board contains an empty card");
        }
        cards.push_back(parse_card(token));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return cards;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        stackrisk::SimulationRequest request{
            .hero = {stackrisk::Card{stackrisk::Rank::two, stackrisk::Suit::clubs},
                     stackrisk::Card{stackrisk::Rank::two, stackrisk::Suit::clubs}}};
        bool has_hero = false;

        for (int index = 1; index < argc; ++index) {
            const std::string_view option(argv[index]);
            if (option == "--help") {
                print_help();
                return 0;
            }
            if (option == "--hero") {
                if (index + 2 >= argc) {
                    throw std::invalid_argument("--hero requires two cards");
                }
                request.hero = {parse_card(argv[++index]), parse_card(argv[++index])};
                has_hero = true;
            } else if (option == "--board") {
                if (++index >= argc) {
                    throw std::invalid_argument("--board requires a card list");
                }
                request.board = parse_board(argv[index]);
            } else if (option == "--opponents") {
                if (++index >= argc) {
                    throw std::invalid_argument("--opponents requires a number");
                }
                request.opponents = static_cast<std::size_t>(
                    parse_integer(argv[index], "opponents"));
            } else if (option == "--trials") {
                if (++index >= argc) {
                    throw std::invalid_argument("--trials requires a number");
                }
                request.trials = parse_integer(argv[index], "trials");
            } else if (option == "--seed") {
                if (++index >= argc) {
                    throw std::invalid_argument("--seed requires a number");
                }
                request.seed = parse_integer(argv[index], "seed");
            } else if (option == "--workers") {
                if (++index >= argc) {
                    throw std::invalid_argument("--workers requires a number");
                }
                request.workers =
                    static_cast<std::size_t>(parse_integer(argv[index], "workers"));
            } else {
                throw std::invalid_argument("unknown option: " +
                                            std::string(option));
            }
        }
        if (!has_hero) throw std::invalid_argument("--hero is required");

        const auto result = stackrisk::simulate_equity(request);
        const auto percent = [](double value) { return value * 100.0; };
        std::cout << std::fixed << std::setprecision(2)
                  << "Trials: " << result.trials << '\n'
                  << "Equity: " << percent(result.equity) << "%\n"
                  << "95% CI: [" << percent(result.confidence_low) << "%, "
                  << percent(result.confidence_high) << "%]\n"
                  << "Wins: " << result.wins << '\n'
                  << "Ties: " << result.ties << '\n'
                  << "Losses: " << result.losses << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}

#pragma once

#include "stackrisk/card.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace stackrisk {

struct SimulationRequest {
    std::array<Card, 2> hero;
    std::vector<Card> board;
    std::size_t opponents = 1;
    std::uint64_t trials = 100'000;
    std::uint64_t seed = 42;
    std::size_t workers = 1;
};

struct EquityResult {
    std::uint64_t trials = 0;
    std::uint64_t wins = 0;
    std::uint64_t ties = 0;
    std::uint64_t losses = 0;
    double equity = 0.0;
    double confidence_low = 0.0;
    double confidence_high = 0.0;
};

[[nodiscard]] EquityResult simulate_equity(const SimulationRequest& request);

}  // namespace stackrisk

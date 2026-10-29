cmake_minimum_required(VERSION 3.20)
project(StackRisk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(stackrisk
    src/card.cpp
    src/evaluator.cpp
    src/simulation.cpp
)
target_include_directories(stackrisk PUBLIC include)
target_compile_options(stackrisk PRIVATE -Wall -Wextra -Wpedantic)

add_executable(stackrisk-cli src/main.cpp)
target_link_libraries(stackrisk-cli PRIVATE stackrisk)
target_compile_options(stackrisk-cli PRIVATE -Wall -Wextra -Wpedantic)

include(CTest)
if(BUILD_TESTING)
    add_executable(stackrisk-tests tests/test_main.cpp)
    target_link_libraries(stackrisk-tests PRIVATE stackrisk)
    target_compile_options(stackrisk-tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME stackrisk-tests COMMAND stackrisk-tests)
    add_test(
        NAME stackrisk-cli-smoke
        COMMAND stackrisk-cli --hero As Kd --board 2c,3s,4h
                --opponents 2 --trials 1000 --seed 42
    )
    add_test(
        NAME stackrisk-cli-rejects-duplicate-cards
        COMMAND stackrisk-cli --hero As As --trials 2
    )
    set_tests_properties(stackrisk-cli-rejects-duplicate-cards
                         PROPERTIES WILL_FAIL TRUE)
endif()

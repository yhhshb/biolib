
#include <vector>
#include <random>
#include <thread>
#include <iostream>
#include "../include/logtools.hpp"
#include <cassert>

typedef std::mt19937 g_t;
typedef std::uniform_int_distribution<std::size_t> d_t; 

void check_timer(std::size_t queue_size, std::size_t trials, g_t& gen, d_t& dist);

int main()
{
    g_t gen(42); // mersenne_twister_engine seeded with rd()
    d_t distrib(1, 100000);

    logging_tools::micro_timer glob(20);
    glob.resize(1);
    glob.start();

    check_timer(1, 1, gen, distrib);
    check_timer(10, 100, gen, distrib);
    check_timer(100, 13, gen, distrib);

    std::cerr << "Elapsed: " << glob.stop(false) << "us total\n";

    std::cerr << "Everything is OK\n";

    return 0;
}

void check_timer(std::size_t queue_size, std::size_t trials, g_t& gen, d_t& dist)
{
    using namespace std::chrono_literals;
    logging_tools::micro_timer timer(queue_size);
    std::size_t elapsed = 0;
    std::vector<std::size_t> history;
    for (std::size_t i = 0; i < trials; ++i) {
        auto n = dist(gen);
        timer.start();
        std::this_thread::sleep_for(std::chrono::microseconds(n));
        elapsed = timer.stop();
        history.push_back(elapsed);
        assert(elapsed >= n);
        // std::cerr << "slept for " << elapsed << " us\n";
    }
    assert(elapsed == timer.get_last_measurement());

    assert(std::accumulate(history.rbegin(), history.rbegin() + std::min(queue_size, trials), static_cast<std::size_t>(0)) == timer.get_cumulative_measurement());

    timer.resize(queue_size / 2);
    (assert(timer.data().size() == queue_size / 2));
    std::cerr << "Resized\n";
    assert(std::accumulate(history.rbegin(), history.rbegin() + std::min(queue_size / 2, trials), static_cast<std::size_t>(0)) == timer.get_cumulative_measurement());
}
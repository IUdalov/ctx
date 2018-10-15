#include "coro_multiplexer.h"
#include "bsearch.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <random>

constexpr size_t TEST_DATA_BYTES = 500'000'000;
constexpr size_t SEARCHES = 5'000'000;
//constexpr size_t TEST_DATA_BYTES = 5000;
//constexpr size_t SEARCHES = 5;

std::vector<int64_t> generateData(size_t nelems) {
    std::vector<int64_t> data;
    data.reserve(nelems);

    int64_t acc = 1;
    for(size_t i = 0; i < nelems; i++) {
        data.push_back(acc);
        acc += 2;
    }

    return data;
}

std::chrono::milliseconds execTime(const std::function<void()> f) {
    auto begin = std::chrono::steady_clock::now();
    f();
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
}

void validateRes(int64_t target, bool found) noexcept {
    assert((target % 2 == 1) == found);
};

int main(int, char**) {
    constexpr size_t nelems = TEST_DATA_BYTES / sizeof(int64_t);
    std::vector<int64_t> data = generateData(nelems);
    std::vector<int64_t> searchData(SEARCHES, 0);

    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int64_t> dis(0, 2 * nelems);
        for(size_t i = 0; i < SEARCHES; i++)
            searchData[i] = dis(gen);
    }

    std::cout << "Running " << searchData.size() << " searches on " << (data.size() * sizeof(int64_t) / 1'000)  << " Kb" << std::endl;
    auto noCoroScore = execTime([&] {
        for (const auto &target : searchData)
            validateRes(target, bSearch(data, target));
    });

    std::cout << "#\tno coro score: " << noCoroScore.count() << " ms" << std::endl;

    auto coroScore = execTime([&] {
        size_t nextPeek = 0;
        size_t end = searchData.size();

        _gLimiter = new CoroMultiplexer<N_COROUTINES>([&]() -> bool {
            int64_t target = searchData[nextPeek];
            nextPeek++;
            validateRes(target, coroBSearch(data, target));

            return nextPeek < end;
        });

        _gLimiter->run();
    });

    std::cout << "#\tcoro score: " << coroScore.count() << " ms" << std::endl;
    delete _gLimiter;
    _gLimiter = nullptr;

    std::cout << "Speedup: " <<  (1 - static_cast<double>(coroScore.count()) / static_cast<double>(noCoroScore.count())) << "% faster" << std::endl;

    return 0;
}



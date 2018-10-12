#include <iostream>
#include <boost/context/all.hpp>
#include <chrono>
#include <list>
#include <vector>
#include <random>
#include <deque>
#include <optional>
#include <queue>


constexpr size_t TEST_DATA_BYTES = 500'000'000;
constexpr size_t SEARCHES = 5'000'000;
//constexpr size_t TEST_DATA_BYTES = 5000;
//constexpr size_t SEARCHES = 5;
constexpr size_t NCORO = 4;

// __builtin_expect()

class Limiter;
Limiter* _gLimiter = nullptr;
#define PREFETCH_ADDR(addr) { __builtin_prefetch((addr), 0, 3); if (_gLimiter) _gLimiter->next(); }

#define LOG if (false) std::cout

namespace ctx = boost::context;

class Limiter {
public:
    using Action = std::function<bool()>;
public:
    explicit Limiter(const Action& cAction) {

        alive_.resize(NCORO, true);
        for(size_t i = 0; i < NCORO; i++)
            tasks_.emplace_back(ctx::callcc([this, cAction, i] (ctx::continuation sink) {

                LOG << "created " << i << std::endl;
                self_.emplace_back(sink.resume());

                LOG << "started " << i << std::endl;
                while(cAction());

                LOG << "dead " << i << std::endl;
                alive_[i] = false;

                return std::move(self_[i]);
            }));
    };

    inline void run() {
        assert(!tasks_.empty());

        bool hasAliveCoro = true;
        while(hasAliveCoro) {
            hasAliveCoro = false;
            for(size_t i = 0; i < NCORO; i++) {
                curr = i;
                if (alive_[i]) {
                    tasks_[i] = tasks_[i].resume();
                    hasAliveCoro = true;
                }
            }
        }
    }

    inline void next() {
        LOG << "next " << curr << std::endl;
        self_[curr] = self_[curr].resume();
    }

private:
    size_t curr = 0;
    std::vector<bool> alive_;
    std::vector<ctx::continuation> tasks_;
    std::vector<ctx::continuation> self_;
};

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

bool bsearch(const std::vector<int64_t>& data, int64_t target) noexcept {
    size_t left = 0;
    size_t right = data.size();

    PREFETCH_ADDR(&data[left]);
    if (data[left] == target)
        return true;

    PREFETCH_ADDR(&data[right]);
    if (data[right] == target)
        return true;

    while(right - left > 1) {
        size_t mid = left + (right  - left) / 2;
        PREFETCH_ADDR(&data[mid]);

        if (data[mid] == target)
            return true;
        else if (data[mid] > target)
            right = mid;
        else
            left = mid;
    }

    return false;
}

std::chrono::milliseconds mesureTime(std::function<void()> f) {
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

    auto noCoroScore = mesureTime([&] {
        for(const auto& target : searchData)
            validateRes(target, bsearch(data, target));
    });

    std::cout << "#\tno coro score: " << noCoroScore.count() << "ms" << std::endl;

    auto coroScore = mesureTime([&] {
        size_t nextPeek = 0;
        size_t end = searchData.size();

        _gLimiter = new Limiter([&] () -> bool {
            int64_t target = searchData[nextPeek];
            nextPeek++;
            validateRes(target, bsearch(data, target));

            return nextPeek < end;
        });

        _gLimiter->run();
    });

    delete _gLimiter;
    _gLimiter = nullptr;


    std::cout << "#\tcoro score: " << coroScore.count() << "ms" << std::endl;

    return 0;
}



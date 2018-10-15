#pragma once

#include <boost/context/all.hpp>
#include <array>

// https://www.boost.org/doc/libs/1_67_0/libs/context/doc/html/context/performance.html#performance

constexpr size_t N_COROUTINES = 4;
#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)
#define PREFETCH_ADDR(addr) { __builtin_prefetch((addr), 0, 3); _gLimiter->next(); }

// #define LOG if (false) std::cout

template<size_t capacity>
class CoroMultiplexer {
public:
    using Action = std::function<bool()>;
public:
    explicit CoroMultiplexer(const Action& cAction) {

        for(size_t i = 0; i < capacity; i++)
            alive_[i] = true;

        for(size_t i = 0; i < capacity; i++)
            tasks_[i] = (boost::context::callcc([this, cAction, i] (boost::context::continuation sink) {

                //LOG << "created " << i << std::endl;
                self_[i] = sink.resume();

                while(LIKELY(cAction()));

                //LOG << "dead " << i << std::endl;
                alive_[i] = false;

                while(true)
                    next();

                assert(false);
                return std::move(self_[i]);
            }));
    };

    inline void run() {
        assert(!tasks_.empty());

        bool hasAliveCoro = true;
        while(LIKELY(hasAliveCoro)) {
            hasAliveCoro = false;

            #pragma unroll(capacity)
            for(size_t i = 0; i < capacity; i++) {
                curr = i;
                if (LIKELY(alive_[i])) {
                    tasks_[i] = tasks_[i].resume();
                    hasAliveCoro = true;
                }
            }
        }
    }

    inline void next() {
        // LOG << "next " << curr << std::endl;
        self_[curr] = self_[curr].resume();
    }

private:
    size_t curr = 0;
    std::array<bool, capacity> alive_;
    std::array<boost::context::continuation, capacity> tasks_;
    std::array<boost::context::continuation, capacity> self_;
};

inline CoroMultiplexer<N_COROUTINES>* _gLimiter = nullptr;

#undef LIKELY
#undef UNLIKELY

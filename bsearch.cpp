#include "bsearch.h"

#include "coro_multiplexer.h"

bool bSearch(const std::vector<int64_t>& data, int64_t target) noexcept {
    size_t left = 0;
    size_t right = data.size() - 1;

    if (data[left] == target)
        return true;

    if (data[right] == target)
        return true;

    while(right - left > 1) {
        size_t mid = left + (right  - left) / 2;

        if (data[mid] == target)
            return true;
        else if (data[mid] > target)
            right = mid;
        else
            left = mid;
    }

    return false;
}


bool coroBSearch(const std::vector<int64_t>& data, int64_t target) noexcept {
    size_t left = 0;
    size_t right = data.size() - 1;

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

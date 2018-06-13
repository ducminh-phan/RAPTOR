#ifndef RAND_UTILS_HPP
#define RAND_UTILS_HPP

#include <chrono>
#include <iterator>
#include <limits>
#include <random>
#include <utility>

// Return a random integer N such that a <= N < b
template<class T = int>
T rand_int(const T& a = std::numeric_limits<T>::min(),
           const T& b = std::numeric_limits<T>::max()) {
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();

    static std::default_random_engine generator {seed};
    std::uniform_int_distribution<T> dist(a, b - 1);

    return dist(generator);
}

// Return a random element from a container
template<class T>
typename T::value_type rand_element(T& container,
                                    const size_t& a_ = 0,
                                    const size_t& b_ = std::numeric_limits<size_t>::max()) {
    size_t size = container.size();

    // Make sure a, b are in valid range
    auto b = std::min(size, b_);
    auto a = std::min(a_, b - 1);

    auto index = rand_int<size_t>(a, b);

    auto it = container.begin();
    std::advance(it, index);

    return (*it);
}

#endif // RAND_UTILS_HPP

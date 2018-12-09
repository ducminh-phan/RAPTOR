#ifndef RAND_UTILS_HPP
#define RAND_UTILS_HPP

#include <chrono>
#include <iterator>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

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

// Return a random element from a vector with a given vector of weights
template<class T>
T weighted_rand_element(const std::vector<T>& container, const std::vector<size_t>& weights) {
    if (container.size() != weights.size()) {
        throw std::runtime_error("The container and weights vector should have the same size");
    }

    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    static std::default_random_engine generator {seed};

    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    auto idx = dist(generator);
    return container.at(idx);
}

// Return a random element from a range with a given vector of weights
template<class T>
T weighted_rand_element(const typename std::vector<T>::iterator& first, const typename std::vector<T>::iterator& last,
                        const std::vector<size_t>& weights) {
    if (static_cast<size_t>(last - first) != weights.size()) {
        throw std::runtime_error("The container and weights vector should have the same size");
    }

    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    static std::default_random_engine generator {seed};

    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());
    return *(first + dist(generator));
}

// Return a random element from a range with a given weight map
template<class T>
T weighted_rand_element(const typename std::vector<T>::iterator& first, const typename std::vector<T>::iterator& last,
                        const typename std::unordered_map<T, size_t>& weight_map) {
    std::vector<size_t> weights;

    for (auto iter = first; iter != last; ++iter) {
        T elem = *iter;
        weights.push_back(weight_map.at(elem));
    }

    return weighted_rand_element<T>(first, last, weights);
}

#endif // RAND_UTILS_HPP

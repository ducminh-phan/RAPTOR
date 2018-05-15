#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <chrono>
#include <iostream>
#include <fstream>

void check_file_exists(std::istream& file, const std::string& name);

class Timer {
private:
    // Type aliases to make accessing nested type easier
    using clock_t = std::chrono::high_resolution_clock;
    using millisecond_t = std::chrono::duration<double, std::milli>;

    std::chrono::time_point<clock_t> m_begin;
    std::string m_unit {" ms"};

public:
    Timer() {
        reset();
    }

    void reset() {
        m_begin = clock_t::now();
    }

    double elapsed() const {
        return std::chrono::duration_cast<millisecond_t>(clock_t::now() - m_begin).count();
    }

    const std::string& unit() { return m_unit; }
};

#endif // UTILITIES_HPP

#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>

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

class Profiler {
private:
    static std::unordered_map<std::string, double> m_time_log;
    static std::unordered_map<std::string, int> m_call_log;
    std::string m_name;
    Timer m_timer;

public:
    explicit Profiler(std::string name) : m_name {std::move(name)}, m_timer() {};

    ~Profiler() {
        m_time_log[m_name] += m_timer.elapsed();
        m_call_log[m_name] += 1;
    }

    static void report() {
        std::cout << std::string(80, '-') << std::endl;

        for (const auto& kv: m_time_log) {
            std::cout << "Function " << kv.first << ":" << std::endl;
            std::cout << "\tCalled: " << m_call_log[kv.first] << " times" << std::endl;
            std::cout << "\tCPU time: " << kv.second << Timer().unit() << std::endl;
        }

        std::cout << std::string(80, '-') << std::endl;
    }
};

#endif // UTILITIES_HPP

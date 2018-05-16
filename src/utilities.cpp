#include "utilities.hpp"

void check_file_exists(std::istream& file, const std::string& name) {
    if (!file) {
        std::cerr << "Error occurred while reading " << name << std::endl;
        std::cerr << "Exiting..." << std::endl;
        exit(1);
    }
}

std::unordered_map<std::string, double> Profiler::m_time_log;
std::unordered_map<std::string, int> Profiler::m_call_log;

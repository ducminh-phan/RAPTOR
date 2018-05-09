#include "utilities.hpp"

void check_file_exists(std::ifstream& file, const std::string& name) {
    if (!file) {
        std::cerr << "Error occurred while reading " << name << std::endl;
        std::cerr << "Exiting..." << std::endl;
        exit(1);
    }
}

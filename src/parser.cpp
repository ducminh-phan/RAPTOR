#include <fstream>
#include "parser.hpp"
#include "data_structure.hpp"

Data Parser::parse_data() {
    std::ifstream file {"../Public-Transit-Data/Paris/trips.txt"};
    int i {0};

    if (!file) {
        std::cerr << "Error occurred" << std::endl;
        exit(1);
    }

    for (CSVIterator<stop_id_t> loop {file, false}; loop != CSVIterator<stop_id_t>(); ++loop) {
        std::cout << *loop << std::endl;
        ++i;
        if (i == 5) {
            break;
        }
    }

    return Data();
}

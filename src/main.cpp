#include <iostream>

#include "data_structure.hpp"
#include "experiments.hpp"


void usage_exit(char* argv[]) {
    auto paragraph = [](std::string s, size_t width = 80) -> std::string {
        std::string acc;
        while (!s.empty()) {
            size_t pos = s.size();
            if (pos > width) pos = s.rfind(' ', width);
            std::string line = s.substr(0, pos);
            acc += line + "\n";
            s = s.substr(pos);

            // Remove the space starting the line
            if (s[0] == ' ') s = s.substr(1);
        }
        return acc;
    };

    std::cerr << "Usage: " << argv[0] << " name algo\n\n"
              << paragraph("Run the RAPTOR/HL-RAPTOR algorithm with the generated queries and log "
                           "the running time and arrival times of each query.\n")
              << "Positional arguments:\n"
              << " name\tThe name of the dataset to be used in the algorithm\n"
              << " algo\tThe choice of the algorithm: R for RAPTOR, HLR for HL-RAPTOR";
    exit(1);
}


int main(int argc, char* argv[]) {
    std::string algo {argc == 3 ? argv[2] : ""};

    if (argc != 3 || (algo != "R" && algo != "HLR")) {
        usage_exit(argv);
    }

    std::string name {argv[1]};

    Timetable timetable {name, algo};
    timetable.summary();

    Experiment exp {&timetable};
    exp.run(algo);

    return 0;
}

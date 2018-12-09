#include <iostream>

#include "config.hpp"
#include "clara.hpp"
#include "data_structure.hpp"
#include "experiments.hpp"

std::string name;
bool use_hl;
bool profile;
bool ranked;


int main(int argc, char* argv[]) {
    bool show_help = false;
    auto cli_parser = clara::Arg(name, "name")("The name of the dataset to be used in the algorithm") |
                      clara::Opt(use_hl)["--hl"]("Unrestricted walking with hub labelling") |
                      clara::Opt(profile)["-p"]["--profile"]("Run profile query") |
                      clara::Opt(ranked)["-r"]["--ranked"]("Use ranked queries") |
                      clara::Help(show_help);

    auto result = cli_parser.parse(clara::Args(argc, argv));
    if (!result) {
        std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
        cli_parser.writeToStream(std::cout);
        exit(1);
    }
    if (show_help) {
        cli_parser.writeToStream(std::cout);
        return 0;
    }

    Timetable timetable;
    timetable.summary();

    Experiment exp {&timetable};
    exp.run();

    return 0;
}

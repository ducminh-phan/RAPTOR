#include <iostream>

#include "data_structure.hpp"
#include "experiments.hpp"


int main() {
    Timetable timetable {"Paris"};
    timetable.summary();

    Experiment exp {&timetable};
    exp.run();

    return 0;
}

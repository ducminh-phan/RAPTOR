#include <iostream>

#include "data_structure.hpp"
#include "raptor.hpp"

int main() {
    Timetable timetable {"Paris"};
    timetable.summary();

    Raptor r {&timetable, 123, 414, 666};
    r.raptor();

    return 0;
}

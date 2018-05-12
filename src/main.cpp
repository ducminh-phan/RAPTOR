#include <iostream>

#include "data_structure.hpp"
#include "raptor.hpp"

int main() {
    Timetable timetable {"Paris"};
    timetable.summary();

    raptor(timetable, 123, 414, 666);

    return 0;
}

#include <iostream>

#include "data_structure.hpp"
#include "raptor.hpp"


bool test(const Timetable& timetable) {
    for (const auto& route: timetable.routes()) {
        for (int j = 0; j < route.stops.size() - 1; ++j) {
            for (int i = 0; i < route.trips.size(); ++i) {
                if (route.stop_times[i][j].arr > route.stop_times[i][j + 1].arr) {
                    return false;
                }
            }
        }
    }

    return true;
}


int main() {
    Timetable timetable {"Paris"};
    timetable.summary();

    Raptor r {&timetable, 123, 414, 666};
    r.raptor();

    std::cout << "Test results: " << (test(timetable) ? "passed" : "failed") << std::endl;

    return 0;
}

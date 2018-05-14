#include <iostream>

#include "data_structure.hpp"
#include "raptor.hpp"
#include "utilities.hpp"


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

    std::cout << "Test results: " << (test(timetable) ? "passed" : "failed") << std::endl;

    Timer timer;

    Raptor r {&timetable, 830, 294, 19799};
    auto res = r.raptor();

    std::cout << "Time elapsed: " << timer.elapsed() << timer.unit() << std::endl;

    std::cout << "Labels of the target stop:" << std::endl;
    for (const auto& t: res) {
        std::cout << t << std::endl;
    }


    return 0;
}

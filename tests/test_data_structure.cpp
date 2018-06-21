#include "catch.hpp"
#include "data_structure.hpp"

bool test_stop_times(const Timetable& timetable) {
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

TEST_CASE("Check if the stop times are in order", "[stop_times]") {
    Timetable timetable {"Paris", "R"};
    timetable.summary();

    CHECK(test_stop_times(timetable));
}

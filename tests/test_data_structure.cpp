#include "catch.hpp"
#include "data_structure.hpp"


// The rows in each stop_times table are ordered
bool test_stop_times_rows_ordered(const Timetable& timetable) {
    for (const auto& route: timetable.routes) {
        for (size_t j = 0; j < route.stops.size() - 1; ++j) {
            for (size_t i = 0; i < route.trips.size(); ++i) {
                if (route.stop_times[i][j].arr > route.stop_times[i][j + 1].arr) {
                    return false;
                }
            }
        }
    }

    return true;
}


// The columns in each stop_times table are ordered
bool test_stop_times_columns_ordered(const Timetable& timetable) {
    for (const auto& route: timetable.routes) {
        for (size_t i = 0; i < route.trips.size() - 1; ++i) {
            for (size_t j = 0; j < route.stops.size(); ++j) {
                if (route.stop_times[i][j].arr > route.stop_times[i + 1][j].arr) {
                    return false;
                }
            }
        }
    }

    return true;
}


// The trips of a route have the same stop pattern
bool test_unique_pattern(const Timetable& timetable) {
    for (const auto& route: timetable.routes) {
        auto stops = route.stops;

        for (const auto& row: route.stop_times) {
            // The stop pattern of this row (trip)
            std::vector<node_id_t> row_stops;
            for (const auto& st: row) {
                row_stops.push_back(st.stop_id);
            }

            if (stops != row_stops) {
                return false;
            }
        }
    }

    return true;
}


TEST_CASE("Test the sanity of the dataset and parser", "") {
    Timetable timetable {};
    timetable.summary();

    REQUIRE(test_stop_times_rows_ordered(timetable));

    REQUIRE(test_stop_times_columns_ordered(timetable));

    REQUIRE(test_unique_pattern(timetable));
}

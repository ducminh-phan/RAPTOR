#include "catch.hpp"
#include "data_structure.hpp"


// The rows in each stop_times_by_trips have the same size, which is the size of the stop pattern,
// and the number of rows equals the number of trips
bool test_stop_times_sizes(const Timetable& timetable) {
    for (const auto& route: timetable.routes) {
        const auto& n_route_stops = route.stops.size();

        if (route.stop_times_by_trips.size() != route.trips.size()) {
            return false;
        }

        for (const auto& row: route.stop_times_by_trips) {
            if (row.size() != n_route_stops) return false;
        }
    }

    return true;
}


// The rows in each stop_times_by_stops have the same size, which is the number of trips,
// and the number of rows equals the number of unique stops in the stop pattern
bool test_stop_times_by_stops_sizes(const Timetable& timetable) {
    for (const auto& route: timetable.routes) {
        const auto& n_stops = route.stops.size();
        const auto& n_trips = route.trips.size();

        if (route.stop_times_by_stops.size() != n_stops) {
            return false;
        }

        for (const auto& row: route.stop_times_by_stops) {
            if (row.size() != n_trips) return false;
        }
    }

    return true;
}


// The rows in each stop_times_by_trips table are ordered
bool test_stop_times_rows_ordered(const Timetable& timetable) {
    for (const auto& route: timetable.routes) {
        for (size_t j = 0; j < route.stops.size() - 1; ++j) {
            for (size_t i = 0; i < route.trips.size(); ++i) {
                if (route.stop_times_by_trips[i][j].arr > route.stop_times_by_trips[i][j + 1].arr) {
                    return false;
                }
            }
        }
    }

    return true;
}


// The columns in each stop_times_by_trips table are ordered
bool test_stop_times_columns_ordered(const Timetable& timetable) {
    for (const auto& route: timetable.routes) {
        for (size_t i = 0; i < route.trips.size() - 1; ++i) {
            for (size_t j = 0; j < route.stops.size(); ++j) {
                if (route.stop_times_by_trips[i][j].arr > route.stop_times_by_trips[i + 1][j].arr) {
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

        for (const auto& row: route.stop_times_by_trips) {
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

    REQUIRE(test_stop_times_sizes(timetable));

    REQUIRE(test_stop_times_by_stops_sizes(timetable));

    REQUIRE(test_stop_times_rows_ordered(timetable));

    REQUIRE(test_stop_times_columns_ordered(timetable));

    REQUIRE(test_unique_pattern(timetable));
}

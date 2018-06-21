#include <cmath>

#include "data_structure.hpp"
#include "csv_reader.hpp"
#include "gzstream.h"

extern const trip_id_t null_trip = -1;

void Timetable::parse_data() {
    Timer timer;

    std::cout << "Parsing the data..." << std::endl;

    parse_trips();
    parse_stop_routes();
    if (m_algo == "R") parse_transfers();
    if (m_algo == "HLR") parse_hubs();
    parse_stop_times();

    std::cout << "Complete parsing the data." << std::endl;
    std::cout << "Time elapsed: " << timer.elapsed() << timer.unit() << std::endl;
}

void Timetable::parse_trips() {
    auto trips_file = read_dataset_file<igzstream>(m_path + "trips.csv.gz");

    for (CSVIterator<uint32_t> iter {trips_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto route_id = static_cast<route_id_t>((*iter)[0]);
        auto trip_id = static_cast<trip_id_t>((*iter)[1]);

        // Add a new route if we encounter a new id
        while (m_routes.size() <= route_id) {
            m_routes.emplace_back();
            m_routes.back().id = static_cast<route_id_t>(m_routes.size() - 1);
        }

        // Map the trip to its position in m_routes and trips, this map is used
        // to quickly add the stop times later in the parse_stop_times function
        m_trip_positions.insert({trip_id, {route_id, m_routes[route_id].trips.size()}});

        m_routes[route_id].trips.push_back(trip_id);
        m_routes[route_id].stop_times.emplace_back();
    }
}

void Timetable::parse_stop_routes() {
    auto stop_routes_file = read_dataset_file<igzstream>(m_path + "stop_routes.csv.gz");

    for (CSVIterator<uint32_t> iter {stop_routes_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto stop_id = static_cast<node_id_t>((*iter)[0]);
        auto route_id = static_cast<route_id_t>((*iter)[1]);

        // Add a new stop if we encounter a new id,
        // note that we might have a missing id, e.g., there is no route using stop #1674
        while (m_stops.size() <= stop_id) {
            m_stops.emplace_back();
            m_stops.back().id = static_cast<node_id_t>(m_stops.size() - 1);
        }

        m_stops[stop_id].routes.push_back(route_id);
    }
}

void Timetable::parse_transfers() {
    auto transfers_file = read_dataset_file<igzstream>(m_path + "transfers.csv.gz");

    for (CSVIterator<uint32_t> iter {transfers_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto from = static_cast<node_id_t>((*iter)[0]);
        auto to = static_cast<node_id_t>((*iter)[1]);
        auto time = static_cast<Time::value_type>((*iter)[2]);

        if (m_stops[from].is_valid() && m_stops[to].is_valid()) {
            m_stops[from].transfers.emplace_back(to, time);
        }
    }
}

void Timetable::parse_hubs() {
    auto in_hub_file = read_dataset_file<igzstream>(m_path + "in_hubs.gr.gz");

    for (CSVIterator<uint32_t> iter {in_hub_file.get(), false, ' '}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto node_id = static_cast<node_id_t>((*iter)[0]);
        auto stop_id = static_cast<node_id_t>((*iter)[1]);
        auto distance = static_cast<distance_t>((*iter)[2]);
        auto time = distance_to_time(distance);

        m_stops[stop_id].in_hubs.emplace_back(node_id, time);
        m_inverse_in_hubs[node_id].emplace_back(stop_id, time);
    }

    auto out_hub_file = read_dataset_file<igzstream>(m_path + "out_hubs.gr.gz");

    for (CSVIterator<uint32_t> iter {out_hub_file.get(), false, ' '}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto stop_id = static_cast<node_id_t>((*iter)[0]);
        auto node_id = static_cast<node_id_t>((*iter)[1]);
        auto distance = static_cast<distance_t>((*iter)[2]);

        m_stops[stop_id].out_hubs.emplace_back(node_id, distance_to_time(distance));
    }
}

void Timetable::parse_stop_times() {
    auto stop_times_file = read_dataset_file<igzstream>(m_path + "stop_times.csv.gz");

    for (CSVIterator<uint32_t> iter {stop_times_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto trip_id = static_cast<trip_id_t>((*iter)[0]);
        auto arr = static_cast<Time::value_type>((*iter)[1]);
        auto dep = static_cast<Time::value_type>((*iter)[2]);
        auto stop_id = static_cast<node_id_t>((*iter)[3]);

        trip_pos_t trip_pos = m_trip_positions.at(trip_id);
        route_id_t route_id = trip_pos.first;
        size_t pos = trip_pos.second;

        m_routes[route_id].stop_times[pos].emplace_back(stop_id, arr, dep);

        // Create the stop sequence of the route corresponding to trip_id,
        // we add stop_id to the stop sequence only once by checking if
        // trip_id is the first trip of its route
        if (trip_id == m_routes[route_id].trips[0]) {
            auto& stops = m_routes[route_id].stops;
            stops.push_back(stop_id);

            // Map the stop_id to its index in the stop sequence
            m_routes[route_id].stop_positions[stop_id].push_back(stops.size() - 1);
        }
    }
}

void Timetable::summary() const {
    std::cout << std::string(80, '-') << std::endl;

    std::cout << "Summary of the dataset:" << std::endl;
    std::cout << "Name: " << m_name << std::endl;

    std::cout << m_routes.size() << " routes" << std::endl;

    int count_trips = 0;
    int count_stop_times = 0;
    for (const auto& route: m_routes) {
        count_trips += route.trips.size();
        for (const auto& stop_time: route.stop_times) {
            count_stop_times += stop_time.size();
        }
    }

    std::cout << count_trips << " trips" << std::endl;

    // Count the number of stops with at least one route using it
    int count_stops = 0;
    int count_hubs = 0;
    int count_transfers = 0;
    for (const auto& stop: m_stops) {
        if (stop.is_valid()) {
            count_stops += 1;
        }

        count_transfers += stop.transfers.size();
        count_hubs += stop.in_hubs.size();
        count_hubs += stop.out_hubs.size();
    }
    std::cout << count_stops << " stops" << std::endl;

    if (m_algo == "R") {
        std::cout << count_transfers << " transfers" << std::endl;
    }

    if (m_algo == "HLR") {
        std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.precision(3);
        std::cout << count_hubs / static_cast<double>(count_stops) << " hubs in average" << std::endl;
    }

    std::cout << count_stop_times << " events" << std::endl;

    std::cout << std::string(80, '-') << std::endl;
}

Time distance_to_time(const distance_t& d) {
    static const double v {4.0};  // km/h

    return Time {static_cast<Time::value_type>(std::lround(9 * d / 25 / v))};
}

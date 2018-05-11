#include <tuple>

#include "data_structure.hpp"
#include "csv_reader.hpp"
#include "utilities.hpp"

std::ifstream Timetable::read_dataset_file(const std::string& file_name) {
    std::ifstream file {m_path + file_name};
    check_file_exists(file, file_name);

    return file;
}

void Timetable::parse_data() {
    Timer timer;

    std::cout << "Parsing the data..." << std::endl;

    parse_trips();
    parse_stop_routes();
    parse_transfers();
    parse_stop_times();

    std::cout << "Complete parsing the data." << std::endl;
    std::cout << "Time elapsed: " << timer.elapsed() << timer.unit() << std::endl;
}

void Timetable::parse_trips() {
    std::ifstream trips_file = read_dataset_file("trips.txt");

    for (CSVIterator<uint32_t> iter {trips_file}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto route_id = static_cast<route_id_t>((*iter)[0]);
        auto trip_id = static_cast<trip_id_t>((*iter)[1]);

        // Add a new route if we encounter a new id
        while (m_routes.size() <= route_id) {
            m_routes.emplace_back();
            m_routes[m_routes.size() - 1].id = static_cast<route_id_t>(m_routes.size() - 1);
        }

        // Map the trip to its position in m_routes and trip_ids, this map is used
        // to quickly add the stop times later in the parse_stop_times function
        m_trip_positions.insert({trip_id, {route_id, m_routes[route_id].trip_ids.size()}});

        m_routes[route_id].trip_ids.push_back(trip_id);
        m_routes[route_id].stop_times.emplace_back();
    }
}

void Timetable::parse_stop_routes() {
    std::ifstream stop_routes_file = read_dataset_file("stop_routes.txt");

    for (CSVIterator<uint32_t> iter {stop_routes_file}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto stop_id = static_cast<stop_id_t>((*iter)[0]);
        auto route_id = static_cast<route_id_t>((*iter)[1]);

        // Add a new stop if we encounter a new id,
        // note that we might have a missing id, e.g., there is no route using stop #1674
        while (m_stops.size() <= stop_id) {
            m_stops.emplace_back();
            m_stops[m_stops.size() - 1].id = static_cast<stop_id_t>(m_stops.size() - 1);
        }

        m_stops[stop_id].route_ids.push_back(route_id);
    }
}

void Timetable::parse_transfers() {
    std::ifstream transfers_file = read_dataset_file("transfers_transitive.txt");

    for (CSVIterator<uint32_t> iter {transfers_file}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto from = static_cast<stop_id_t>((*iter)[0]);
        auto to = static_cast<stop_id_t>((*iter)[1]);
        auto time = static_cast<_time_t>((*iter)[2]);

        if (m_stops[from].is_valid() && m_stops[to].is_valid()) {
            m_stops[from].transfers.emplace_back(to, time);
            m_stops[to].transfers.emplace_back(from, time);
        }
    }
}

void Timetable::parse_stop_times() {
    std::ifstream stop_times_file = read_dataset_file("stop_times.txt");

    for (CSVIterator<uint32_t> iter {stop_times_file}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto trip_id = static_cast<trip_id_t>((*iter)[0]);
        auto arr = static_cast<_time_t>((*iter)[1]);
        auto dep = static_cast<_time_t>((*iter)[2]);
        auto stop_id = static_cast<stop_id_t>((*iter)[3]);

        trip_pos_t trip_pos = m_trip_positions.at(trip_id);
        route_id_t route_id = trip_pos.first;
        size_t pos = trip_pos.second;

        m_routes[route_id].stop_times[pos].emplace_back(stop_id, arr, dep);
    }
}

void Timetable::summary() {
    std::cout << std::string(80, '-') << std::endl;

    std::cout << "Summary of the dataset:" << std::endl;
    std::cout << "City: " << m_city_name << std::endl;

    std::cout << m_routes.size() << " routes" << std::endl;

    int count_trips = 0;
    int count_stop_times = 0;
    for (auto const& route: m_routes) {
        count_trips += route.trip_ids.size();
        for (auto const& stop_time: route.stop_times) {
            count_stop_times += stop_time.size();
        }
    }

    std::cout << count_trips << " trips" << std::endl;

    // Count the number of stops with at least one route using it
    int count_stops = 0;
    int count_transfers = 0;
    for (auto const& stop: m_stops) {
        if (!stop.route_ids.empty()) {
            count_stops += 1;
        }

        count_transfers += stop.transfers.size();
    }
    std::cout << count_stops << " stops" << std::endl;
    std::cout << count_transfers / 2 << " transfers" << std::endl;

    std::cout << count_stop_times << " events" << std::endl;

    std::cout << std::string(80, '-') << std::endl;
}

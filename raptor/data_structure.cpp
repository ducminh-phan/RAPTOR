#include <algorithm>
#include <cmath>

#include "data_structure.hpp"
#include "csv.h"
#include "gzstream.h"


extern const trip_id_t NULL_TRIP = -1;


void Timetable::parse_data() {
    Timer timer;

    std::cout << "Parsing the data..." << std::endl;

    parse_trips();
    parse_stop_routes();
    if (!use_hl) {
        parse_transfers();
    } else {
        parse_hubs();
    }
    parse_stop_times();

    std::cout << "Complete parsing the data." << std::endl;
    std::cout << "Time elapsed: " << timer.elapsed() << timer.unit() << std::endl;
}


void Timetable::parse_trips() {
    igzstream trips_file_stream {(path + "trips.csv.gz").c_str()};
    io::CSVReader<2> trips_file_reader {"trips.csv", trips_file_stream};
    trips_file_reader.read_header(io::ignore_no_column, "route_id", "trip_id");

    route_id_t route_id;
    trip_id_t trip_id;

    while (trips_file_reader.read_row(route_id, trip_id)) {
        // Add a new route if we encounter a new id
        while (routes.size() <= route_id) {
            routes.emplace_back();
            routes.back().id = static_cast<route_id_t>(routes.size() - 1);
        }

        // Map the trip to its position in routes and trips, this map is used
        // to quickly add the stop times later in the parse_stop_times function
        if (static_cast<size_t>(trip_id) >= trip_positions.size()) {
            trip_positions.resize(static_cast<size_t>(trip_id + 1));
        }
        trip_positions[trip_id] = {route_id, routes[route_id].trips.size()};

        routes[route_id].trips.push_back(trip_id);
        routes[route_id].stop_times.emplace_back();
    }
}


void Timetable::parse_stop_routes() {
    igzstream stop_routes_file_stream {(path + "stop_routes.csv.gz").c_str()};
    io::CSVReader<2> stop_routes_reader {"stop_routes.csv", stop_routes_file_stream};
    stop_routes_reader.read_header(io::ignore_no_column, "stop_id", "route_id");

    node_id_t stop_id;
    route_id_t route_id;

    while (stop_routes_reader.read_row(stop_id, route_id)) {
        // Add a new stop if we encounter a new id,
        // note that we might have a missing id, e.g., there is no route using stop #1674
        while (stops.size() <= stop_id) {
            stops.emplace_back();
            stops.back().id = static_cast<node_id_t>(stops.size() - 1);
        }

        stops[stop_id].routes.push_back(route_id);
    }

    max_stop_id = max_node_id = stops.back().id;
}


void Timetable::parse_transfers() {
    igzstream transfers_file_stream {(path + "transfers.csv.gz").c_str()};
    io::CSVReader<3> transfers_reader {"transfers.csv", transfers_file_stream};
    transfers_reader.read_header(io::ignore_no_column, "from_stop_id", "to_stop_id", "min_transfer_time");

    node_id_t from;
    node_id_t to;
    Time::value_type time;

    while (transfers_reader.read_row(from, to, time)) {
        if (stops[from].is_valid() && stops[to].is_valid()) {
            stops[from].transfers.emplace_back(to, time);
            stops[to].backward_transfers.emplace_back(from, time);
        }

        max_node_id = std::max(max_node_id, static_cast<std::size_t>(from));
        max_node_id = std::max(max_node_id, static_cast<std::size_t>(to));
    }

    for (auto& stop: stops) {
        std::sort(stop.transfers.begin(), stop.transfers.end());
    }
}


void Timetable::parse_hubs() {
    igzstream in_hubs_file_stream {(path + "in_hubs.gr.gz").c_str()};
    io::CSVReader<3> in_hubs_reader {"in_hubs.gr", in_hubs_file_stream};
    in_hubs_reader.set_header("node_id", "stop_id", "distance");

    node_id_t node_id;
    node_id_t stop_id;
    distance_t distance;

    while (in_hubs_reader.read_row(node_id, stop_id, distance)) {
        auto time = distance_to_time(distance);

        if (node_id > max_node_id) {
            max_node_id = node_id;
            inverse_in_hubs.resize(max_node_id + 1);
        }

        stops[stop_id].in_hubs.emplace_back(time, node_id);
        inverse_in_hubs[node_id].emplace_back(time, stop_id);
    }

    igzstream out_hubs_file_stream {(path + "out_hubs.gr.gz").c_str()};
    io::CSVReader<3> out_hubs_reader {"out_hubs.gr", out_hubs_file_stream};
    out_hubs_reader.set_header("stop_id", "node_id", "distance");

    while (out_hubs_reader.read_row(stop_id, node_id, distance)) {
        auto time = distance_to_time(distance);

        if (node_id > max_node_id) {
            max_node_id = node_id;
            inverse_out_hubs.resize(max_node_id + 1);
        }

        stops[stop_id].out_hubs.emplace_back(time, node_id);
        inverse_out_hubs[node_id].emplace_back(time, stop_id);
    }

    for (auto& stop: stops) {
        std::sort(stop.in_hubs.begin(), stop.in_hubs.end());
        std::sort(stop.out_hubs.begin(), stop.out_hubs.end());
    }

    for (auto& item: inverse_in_hubs) {
        std::sort(item.begin(), item.end());
    }

    for (auto& item: inverse_out_hubs) {
        std::sort(item.begin(), item.end());
    }
}


void Timetable::parse_stop_times() {
    igzstream stop_times_file_stream {(path + "stop_times.csv.gz").c_str()};
    io::CSVReader<4> stop_times_reader {"stop_times.csv", stop_times_file_stream};
    stop_times_reader.read_header(io::ignore_extra_column, "trip_id", "arrival_time", "departure_time", "stop_id");

    trip_id_t trip_id;
    Time::value_type arr, dep;
    node_id_t stop_id;

    while (stop_times_reader.read_row(trip_id, arr, dep, stop_id)) {
        trip_pos_t trip_pos = trip_positions[trip_id];
        route_id_t route_id = trip_pos.first;
        size_t pos = trip_pos.second;

        routes[route_id].stop_times[pos].emplace_back(stop_id, arr, dep);

        // Create the stop sequence of the route corresponding to trip_id,
        // we add stop_id to the stop sequence only once by checking if
        // trip_id is the first trip of its route
        if (trip_id == routes[route_id].trips[0]) {
            auto& stops = routes[route_id].stops;
            stops.push_back(stop_id);

            // Map the stop_id to its index in the stop sequence
            if (stop_id >= routes[route_id].stop_positions.size()) {
                routes[route_id].stop_positions.resize(stop_id + 1);
            }
            routes[route_id].stop_positions[stop_id].push_back(stops.size() - 1);
        }
    }
}


void Timetable::summary() const {
    std::cout << std::string(80, '-') << std::endl;

    std::cout << "Summary of the dataset:" << std::endl;
    std::cout << "Name: " << name << std::endl;

    std::cout << routes.size() << " routes" << std::endl;

    int count_trips = 0;
    int count_stop_times = 0;
    for (const auto& route: routes) {
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
    for (const auto& stop: stops) {
        if (stop.is_valid()) {
            count_stops += 1;
        }

        count_transfers += stop.transfers.size();
        count_hubs += stop.in_hubs.size();
        count_hubs += stop.out_hubs.size();
    }
    std::cout << count_stops << " stops" << std::endl;

    if (!use_hl) {
        std::cout << count_transfers << " transfers" << std::endl;
    } else {
        std::cout.setf(std::ios::fixed, std::ios::floatfield);
        std::cout.precision(3);
        std::cout << count_hubs / static_cast<double>(count_stops) << " hubs in average" << std::endl;
    }

    std::cout << count_stop_times << " events" << std::endl;

    std::cout << std::string(80, '-') << std::endl;
}


Time Timetable::walking_time(const node_id_t& source_id, const node_id_t& target_id) const {
    if (!use_hl) throw NotImplemented();

    std::unordered_map<node_id_t, Time> tmp_hub_labels;
    Time arrival_time {};

    // Find the earliest time to get to the out hubs of the source
    for (const auto& kv: stops[source_id].out_hubs) {
        auto walking_time = kv.first;
        auto hub_id = kv.second;

        tmp_hub_labels[hub_id] = walking_time;
    }

    // Propagate the time from the hubs to the target
    for (const auto& kv: stops[target_id].in_hubs) {
        auto walking_time = kv.first;
        auto hub_id = kv.second;

        arrival_time = std::min(
                arrival_time,
                tmp_hub_labels[hub_id] + walking_time
        );
    }

    return arrival_time;
}


Time distance_to_time(const distance_t& d) {
    static const double v {4.0};  // km/h

    return Time {static_cast<Time::value_type>(std::lround(9 * d / 25 / v))};
}

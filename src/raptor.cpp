#include <algorithm> // std::find, std::min
#include <iostream>

#include "raptor.hpp"

bool Raptor::validate_input() {
    size_t max_stop_id = timetable->stops().size();

    if ((source >= max_stop_id) || !timetable->stops(source).is_valid()) {
        std::cerr << "The source stop id of " << source << " is not a valid stop." << std::endl;
        return false;
    }

    if ((target >= max_stop_id) || !timetable->stops(target).is_valid()) {
        std::cerr << "The target stop id of " << target << " is not a valid stop." << std::endl;
        return false;
    }

    if (source == target) {
        std::cerr << "The source and the target need to be distinct." << std::endl;
        return false;
    }

    if (dep > 86400) {
        std::cerr << "The departure time of " << dep << " is invalid." << std::endl;
        return false;
    }

    return true;
}

// Check is stop1 comes before stop2 in the route
bool Raptor::check_stops_order(const route_id_t& route_id, const stop_id_t& stop1, const stop_id_t& stop2) {
    Profiler prof {__func__};

    const auto& route = timetable->routes(route_id);
    const auto& idx1 = route.stop_positions.at(stop1).front();
    const auto& idx2 = route.stop_positions.at(stop2).front();

    return idx1 < idx2;
}

route_stop_queue_t Raptor::make_queue() {
    Profiler prof {__func__};

    route_stop_queue_t queue;

    for (const auto& stop_id: marked_stops) {
        const Stop& stop = timetable->stops(stop_id);

        for (const auto& route: stop.routes) {
            bool found = false;
            auto route_iter = queue.find(route);

            // There are some (r, p) in the queue
            if (route_iter != queue.end()) {
                std::vector<stop_id_t> stops_to_erase;

                // Iterate over all p to find if there are stops come after s
                for (const auto& p: route_iter->second) {
                    if (check_stops_order(route, stop_id, p)) {
                        // Add p to the list of stops to be erased later, we do this
                        // in order to avoid modifying a set while iterating over it
                        stops_to_erase.push_back(p);

                        found = true;
                    }
                }

                // Remove all stops that are in the queue coming after stop_id in the route, if any
                if (found) {
                    for (const auto& p: stops_to_erase) {
                        route_iter->second.erase(p);
                    }
                }
            }

            queue[route].insert(stop_id);
        }
    }

    marked_stops.clear();

    return queue;
}

// Find the earliest trip in route r that one can catch at stop s in round k,
// i.e., the earliest trip t such that t_dep(t, s) >= t_(k-1) (s)
trip_id_t Raptor::earliest_trip(const uint16_t& round, const route_id_t& route_id, const stop_id_t& stop_id) {
    static std::unordered_map<key_t, trip_id_t, key_hash> cache;
    _time_t t = labels[stop_id][round - 1];

    auto* prof_c = new Profiler {"cached"};

    // We make the label of the stop as a part of the key, instead of the round.
    // This speeds up the function since the label could be the same in several rounds,
    // so that we do not need to find the earliest trip again in another round if the
    // label remains the same.
    auto key = std::make_tuple(t.val(), route_id, stop_id);
    auto search = cache.find(key);
    if (search != cache.end()) {
        delete prof_c;
        return search->second;
    }

    delete prof_c;

    Profiler prof {__func__};
    const auto& route = timetable->routes(route_id);

    const auto& stop_times = route.stop_times;
    auto iter = stop_times.begin();
    auto last = stop_times.end();
    std::vector<size_t> stop_idx = route.stop_positions.at(stop_id);

    // Iterate over the trips
    while (iter != last) {
        trip_id_t r = route.trips[iter - stop_times.begin()];

        // Iterate over the appearances of the stop in the trip
        for (const auto& idx: stop_idx) {
            // The departure time of the current trip at stop_id
            const _time_t& dep = (*iter).at(idx).dep;

            if (dep >= t) {
                cache[key] = r;
                return r;
            }
        }

        ++iter;
    }

    cache[key] = null_trip;
    return null_trip;
}

std::vector<_time_t> Raptor::raptor() {
    // std::cout << std::boolalpha << validate_input() << std::endl;

    // Initialisation
    for (const auto& stop: timetable->stops()) {
        earliest_arrival_time[stop.id] = _time_t();
        labels[stop.id].emplace_back();
    }
    labels[source] = {dep};
    marked_stops.insert(source);

    uint16_t round {1};
    while (true) {
        // First stage, set an upper bound on the earliest arrival time at every stop
        // by copying the arrival time from the previous round
        for (const auto& stop: timetable->stops()) {
            labels[stop.id].emplace_back(labels[stop.id].back());
        }

        // Second stage
        auto queue = make_queue();

        // Traverse each route
        for (const auto& route_stop: queue) {
            auto route_id = route_stop.first;

            for (const auto& stop_id: route_stop.second) {
                auto& route = timetable->routes(route_id);

                trip_id_t t = null_trip;
                size_t stop_idx = route.stop_positions.at(stop_id).front();

                // Iterate over the stops of the route beginning with stop_id
                for (size_t i = stop_idx; i < route.stops.size(); ++i) {
                    stop_id_t p_i = route.stops[i];
                    _time_t dep, arr;

                    if (t != null_trip) {
                        // Get the position of the trip t
                        trip_pos_t trip_pos = timetable->trip_positions(t);
                        size_t pos = trip_pos.second;

                        // Get the departure and arrival time of the trip t at the stop p_i
                        dep = route.stop_times[pos][i].dep;
                        arr = route.stop_times[pos][i].arr;

                        // Local and target pruning
                        if (arr < std::min(earliest_arrival_time[p_i], earliest_arrival_time[target])) {
                            labels[p_i][round] = arr;
                            earliest_arrival_time[p_i] = arr;
                            marked_stops.insert(p_i);
                        }
                    }

                    // Check if we can catch an earlier trip at p_i
                    if (labels[p_i][round - 1] <= dep) {
                        t = earliest_trip(round, route_id, p_i);
                    }
                }
            }
        }

        // Third stage, look at footpaths
        for (const auto& stop_id: marked_stops) {
            for (const auto& transfer: timetable->stops(stop_id).transfers) {
                auto dest_id = transfer.dest;
                auto transfer_time = transfer.time;

                labels[dest_id].back() = std::min(
                        labels[dest_id].back(),
                        labels[stop_id].back() + transfer_time
                );

                marked_stops.insert(dest_id);
            }
        }

        ++round;
        if (marked_stops.empty()) break;
    }

    Profiler::report();
    return labels[target];
}

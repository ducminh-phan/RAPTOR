#include <algorithm> // std::find, std::min
#include <iostream>

#include "raptor.hpp"

// Check if stop1 comes before stop2 in the route
bool Raptor::check_stops_order(const route_id_t& route_id, const node_id_t& stop1, const node_id_t& stop2) {
    Profiler prof {__func__};

    const auto& route = m_timetable->routes(route_id);
    const auto& idx1 = route.stop_positions.at(stop1).front();
    const auto& idx2 = route.stop_positions.at(stop2).front();

    return idx1 < idx2;
}

route_stop_queue_t Raptor::make_queue(std::set<node_id_t>& marked_stops) {
    Profiler prof {__func__};

    route_stop_queue_t queue;

    for (const auto& stop_id: marked_stops) {
        const Stop& stop = m_timetable->stops(stop_id);

        for (const auto& route_id: stop.routes) {
            auto route_iter = queue.find(route_id);

            // Check if there is already a pair (r, p) in the queue
            if (route_iter != queue.end()) {
                auto p = route_iter->second;

                // If s comes before p, replace p by s
                if (check_stops_order(route_id, stop_id, p)) {
                    queue[route_id] = stop_id;
                }
            } else {
                // If r is not in the queue, add (r, s) to the queue
                queue[route_id] = stop_id;
            }
        }
    }

    marked_stops.clear();

    return queue;
}

// Find the earliest trip in route r that one can catch at stop s in round k,
// i.e., the earliest trip t such that t_dep(t, s) >= t_(k-1) (s)
trip_id_t Raptor::earliest_trip(const uint16_t& round, const labels_t& labels,
                                const route_id_t& route_id, const node_id_t& stop_id) {
    static std::unordered_map<cache_key_t, trip_id_t, cache_key_hash> cache;
    Time t = labels.at(stop_id)[round - 1];

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
    const auto& route = m_timetable->routes(route_id);

    const auto& stop_times = route.stop_times;
    auto iter = stop_times.begin();
    auto last = stop_times.end();
    std::vector<size_t> stop_idx = route.stop_positions.at(stop_id);
    trip_id_t earliest_trip = null_trip;
    Time earliest_time;

    // Iterate over the trips
    while (iter != last) {
        trip_id_t r = route.trips[iter - stop_times.begin()];

        // Iterate over the appearances of the stop in the trip
        for (const auto& idx: stop_idx) {
            // The departure time of the current trip at stop_id
            const Time& dep = (*iter).at(idx).dep;

            if (dep >= t && dep < earliest_time) {
                earliest_trip = r;
                earliest_time = dep;
            }
        }

        ++iter;
    }

    cache[key] = earliest_trip;
    return earliest_trip;
}

std::vector<Time> Raptor::query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time) {
    labels_t labels;
    std::set<node_id_t> marked_stops;
    std::unordered_map<node_id_t, Time> earliest_arrival_time;

    // Initialisation
    for (const auto& stop: m_timetable->stops()) {
        earliest_arrival_time[stop.id] = {};
        labels[stop.id].emplace_back();
    }
    labels[source_id] = {departure_time};
    marked_stops.insert(source_id);
    std::unordered_map<node_id_t, Time> tmp_hub_labels;

    if (m_algo == "HLR") {
        // Find the time to get to the target using only the walking graph
        for (const auto& kv: m_timetable->stops(source_id).out_hubs) {
            auto walking_time = kv.first;
            auto hub_id = kv.second;

            tmp_hub_labels[hub_id] = std::min(tmp_hub_labels[hub_id], departure_time + walking_time);
        }
        for (const auto& kv: m_timetable->stops(target_id).in_hubs) {
            auto walking_time = kv.first;
            auto hub_id = kv.second;

            if (tmp_hub_labels[hub_id]) {
                labels[target_id].back() = std::min(
                        labels[target_id].back(),
                        tmp_hub_labels[hub_id] + walking_time
                );
            }
        }

        earliest_arrival_time[target_id] = labels[target_id].back();
        tmp_hub_labels.clear();
    }

    uint16_t round {1};
    while (true) {
        // First stage, set an upper bound on the earliest arrival time at every stop
        // by copying the arrival time from the previous round
        for (const auto& stop: m_timetable->stops()) {
            labels[stop.id].emplace_back(labels[stop.id].back());
        }

        // Second stage
        auto queue = make_queue(marked_stops);

        // Traverse each route
        for (const auto& route_stop: queue) {
            auto route_id = route_stop.first;
            auto stop_id = route_stop.second;
            auto& route = m_timetable->routes(route_id);

            trip_id_t t = null_trip;
            size_t stop_idx = route.stop_positions.at(stop_id).front();

            // Iterate over the stops of the route beginning with stop_id
            for (size_t i = stop_idx; i < route.stops.size(); ++i) {
                node_id_t p_i = route.stops[i];
                Time dep, arr;

                if (t != null_trip) {
                    // Get the position of the trip t
                    trip_pos_t trip_pos = m_timetable->trip_positions(t);
                    size_t pos = trip_pos.second;

                    // Get the departure and arrival time of the trip t at the stop p_i
                    dep = route.stop_times[pos][i].dep;
                    arr = route.stop_times[pos][i].arr;

                    // Local and target pruning
                    if (arr < std::min(earliest_arrival_time[p_i], earliest_arrival_time[target_id])) {
                        labels[p_i][round] = arr;
                        earliest_arrival_time[p_i] = arr;
                        marked_stops.insert(p_i);
                    }
                }

                // Check if we can catch an earlier trip at p_i
                if (labels[p_i][round - 1] <= dep) {
                    t = earliest_trip(round, labels, route_id, p_i);
                }
            }
        }

        ++round;
        if (marked_stops.empty()) break;

        // Third stage, look at footpaths

        if (m_algo == "R") {
            for (const auto& stop_id: marked_stops) {
                for (const auto& transfer: m_timetable->stops(stop_id).transfers) {
                    auto dest_id = transfer.dest;
                    auto transfer_time = transfer.time;

                    labels[dest_id].back() = std::min(
                            labels[dest_id].back(),
                            labels[stop_id].back() + transfer_time
                    );

                    marked_stops.insert(dest_id);
                }
            }
        }

        if (m_algo == "HLR") {
            std::unordered_set<node_id_t> improved_hubs;

            for (const auto& stop_id: marked_stops) {
                for (const auto& kv: m_timetable->stops(stop_id).out_hubs) {
                    auto walking_time = kv.first;
                    auto hub_id = kv.second;

                    auto tmp = labels[stop_id].back() + walking_time;

                    // Since we sort the links stop->out-hub in the increasing order of walking time,
                    // as soon as the arrival time propagated to a hub is after the earliest arrival time
                    // at the target, there is no need to propagate to the next hubs
                    if (tmp > earliest_arrival_time[target_id]) break;

                    if (tmp < tmp_hub_labels[hub_id]) {
                        tmp_hub_labels[hub_id] = tmp;
                        improved_hubs.insert(hub_id);
                    }
                }
            }

            for (const auto& hub_id: improved_hubs) {
                // We need to check if hub_id, which is the out-hub of some stop,
                // is the in-hub of some other stop
                if (m_timetable->inverse_in_hubs().count(hub_id)) {
                    for (const auto& kv: m_timetable->inverse_in_hubs().at(hub_id)) {
                        auto walking_time = kv.first;
                        auto stop_id = kv.second;

                        auto tmp = tmp_hub_labels[hub_id] + walking_time;
                        if (tmp > earliest_arrival_time[target_id]) break;

                        if (tmp_hub_labels[hub_id]) {
                            labels[stop_id].back() = std::min(labels[stop_id].back(), tmp);
                        }

                        if (labels[stop_id].back() < earliest_arrival_time[stop_id]) {
                            marked_stops.insert(stop_id);
                            earliest_arrival_time[stop_id] = labels[stop_id].back();
                        }
                    }
                }
            }
        }
    }

    Profiler::clear();
    return labels[target_id];
}

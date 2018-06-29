#include <algorithm> // std::find, std::min
#include <iostream>

#include "raptor.hpp"


// Check if stop1 comes before/after stop2 in the route
bool Raptor::check_stops_order(const route_id_t& route_id, const node_id_t& stop1, const node_id_t& stop2,
                               const bool& backward) {
    Profiler prof {__func__};

    const auto& route = m_timetable->routes(route_id);

    if (backward) {
        const auto& idx1 = route.stop_positions.at(stop1).back();
        const auto& idx2 = route.stop_positions.at(stop2).back();

        return idx1 > idx2;
    } else {
        const auto& idx1 = route.stop_positions.at(stop1).front();
        const auto& idx2 = route.stop_positions.at(stop2).front();

        return idx1 < idx2;
    }
}


route_stop_queue_t Raptor::make_queue(std::set<node_id_t>& marked_stops, const bool& backward) {
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
                if (check_stops_order(route_id, stop_id, p, backward)) {
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
// i.e., the earliest trip t such that t_dep(t, s) >= t_(k-1) (s),
// or the latest trip t such that t_arr(t, s) <= t_(k-1) (s) if the query is backward
trip_id_t Raptor::earliest_trip(const uint16_t& round, const labels_t& labels,
                                const route_id_t& route_id, const node_id_t& stop_id,
                                const bool& backward) {
    static std::unordered_map<cache_key_t, trip_id_t, cache_key_hash> cache;
    Time t = labels.at(stop_id)[round - 1];

    auto* prof_c = new Profiler {"cached"};

    // We make the label of the stop as a part of the key, instead of the round.
    // This speeds up the function since the label could be the same in several rounds,
    // so that we do not need to find the earliest trip again in another round if the
    // label remains the same.
    cache_key_t key = std::make_tuple(t.val(), route_id, stop_id, backward);
    auto search = cache.find(key);
    if (search != cache.end()) {
        delete prof_c;
        return search->second;
    }

    delete prof_c;

    Profiler prof {__func__};
    const auto& route = m_timetable->routes(route_id);

    const auto& stop_times = route.stop_times;
    const auto& stop_idx = route.stop_positions.at(stop_id);
    trip_id_t earliest_trip = NULL_TRIP;
    Time earliest_time;

    for (size_t j = 0; j < stop_times.size(); ++j) {
        size_t i = backward ? stop_times.size() - 1 - j : j;

        trip_id_t r = route.trips[i];

        // Iterate over the appearances of the stop in the trip
        for (const auto& idx: stop_idx) {
            // The departure and arrival times of the current trip at stop_id
            const Time& dep = stop_times.at(i).at(idx).dep;
            const Time& arr = stop_times.at(i).at(idx).arr;

            if (backward) {
                if (arr <= t && arr > earliest_time) {
                    earliest_trip = r;
                    earliest_time = arr;
                }
            } else {
                if (dep >= t && dep < earliest_time) {
                    earliest_trip = r;
                    earliest_time = dep;
                }
            }
        }
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

            labels[target_id].back() = std::min(
                    labels[target_id].back(),
                    tmp_hub_labels[hub_id] + walking_time
            );
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

            trip_id_t t = NULL_TRIP;
            size_t stop_idx = route.stop_positions.at(stop_id).front();

            // Iterate over the stops of the route beginning with stop_id
            for (size_t i = stop_idx; i < route.stops.size(); ++i) {
                node_id_t p_i = route.stops[i];
                Time dep, arr;

                if (t != NULL_TRIP) {
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

                        labels[stop_id].back() = std::min(labels[stop_id].back(), tmp);

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


Time Raptor::backward_query(const node_id_t& source_id, const node_id_t& target_id, const Time& arrival_time) {
    labels_t labels;
    std::set<node_id_t> marked_stops;
    std::unordered_map<node_id_t, Time> latest_departure_time;

    // Initialisation
    for (const auto& stop: m_timetable->stops()) {
        latest_departure_time.emplace(stop.id, 0);
        labels[stop.id].emplace_back(0);
    }
    latest_departure_time[target_id] = {arrival_time};
    marked_stops.insert(target_id);
    std::unordered_map<node_id_t, Time> tmp_hub_labels;

    uint16_t round {1};
    while (true) {
        // Second stage
        auto queue = make_queue(marked_stops, true);

        // Traverse each route
        for (const auto& route_stop: queue) {
            auto route_id = route_stop.first;
            auto stop_id = route_stop.second;
            auto& route = m_timetable->routes(route_id);

            trip_id_t t = NULL_TRIP;
            int stop_idx = route.stop_positions.at(stop_id).front();

            // Iterate over the stops of the route beginning with stop_id in the reverse order
            for (int i = stop_idx; i >= 0; --i) {
                node_id_t p_i = route.stops[i];
                Time dep, arr;

                if (t != NULL_TRIP) {
                    // Get the position of the trip t
                    trip_pos_t trip_pos = m_timetable->trip_positions(t);
                    size_t pos = trip_pos.second;

                    // Get the departure and arrival time of the trip t at the stop p_i
                    dep = route.stop_times[pos][i].dep;
                    arr = route.stop_times[pos][i].arr;

                    // Local and target pruning
                    if (dep > std::max(latest_departure_time[p_i], latest_departure_time[target_id])) {
                        labels[p_i][round] = dep;
                        latest_departure_time[p_i] = dep;
                        marked_stops.insert(p_i);
                    }
                }

                // Check if we can depart by a later trip at p_i
                if (labels[p_i][round - 1] >= arr) {
                    t = earliest_trip(round, labels, route_id, p_i, true);
                }
            }
        }

        ++round;
        if (marked_stops.empty()) break;

        // Third stage, look at footpaths

        if (m_algo == "R") {
            for (const auto& stop_id: marked_stops) {
                for (const auto& transfer: m_timetable->stops(stop_id).backward_transfers) {
                    auto dest_id = transfer.dest;
                    auto transfer_time = transfer.time;

                    labels[dest_id].back() = std::max(
                            labels[dest_id].back(),
                            labels[stop_id].back() - transfer_time
                    );

                    marked_stops.insert(dest_id);
                }
            }
        }

        if (m_algo == "HLR") {
            std::unordered_set<node_id_t> improved_hubs;

            for (const auto& stop_id: marked_stops) {
                for (const auto& kv: m_timetable->stops(stop_id).in_hubs) {
                    auto walking_time = kv.first;
                    auto hub_id = kv.second;

                    auto tmp = labels[stop_id].back() - walking_time;

                    // Since we sort the links stop->out-hub in the increasing order of walking time,
                    // as soon as the arrival time propagated to a hub is after the earliest arrival time
                    // at the target, there is no need to propagate to the next hubs
                    if (tmp < latest_departure_time[target_id]) break;

                    if (tmp > tmp_hub_labels[hub_id]) {
                        tmp_hub_labels[hub_id] = tmp;
                        improved_hubs.insert(hub_id);
                    }
                }
            }

            for (const auto& hub_id: improved_hubs) {
                // We need to check if hub_id, which is the out-hub of some stop,
                // is the in-hub of some other stop
                if (m_timetable->inverse_out_hubs().count(hub_id)) {
                    for (const auto& kv: m_timetable->inverse_out_hubs().at(hub_id)) {
                        auto walking_time = kv.first;
                        auto stop_id = kv.second;

                        auto tmp = tmp_hub_labels[hub_id] - walking_time;
                        if (tmp < latest_departure_time[target_id]) break;

                        labels[stop_id].back() = std::max(labels[stop_id].back(), tmp);

                        if (labels[stop_id].back() > latest_departure_time[stop_id]) {
                            marked_stops.insert(stop_id);
                            latest_departure_time[stop_id] = labels[stop_id].back();
                        }
                    }
                }
            }
        }
    }

    Profiler::clear();
    return labels[target_id].back();
}


// Remove infinity and dominated times from the output of a RAPTOR query
// For example, [inf, inf, 5, 5, 4, 4, 4, 2, 2, 1] -> [5, 4, 2, 1]
std::vector<Time> remove_dominated(const std::vector<Time>& times) {
    Time current_min;
    bool first = true;
    std::vector<Time> pareto_set;

    for (const auto& t : times) {
        // We keep only non-infinity times in the Pareto set
        if (t) {
            if (first) {
                first = false;
                current_min = t;
                pareto_set.push_back(t);
            } else if (t < current_min) {
                current_min = t;
                pareto_set.push_back(t);
            }
        }
    }

    return pareto_set;
}

#include <algorithm> // std::find, std::min
#include <functional>
#include <iostream>
#include <queue>

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
trip_id_t Raptor::earliest_trip(const uint16_t& round, const Time& t,
                                const route_id_t& route_id, const node_id_t& stop_id,
                                const bool& backward) {
    static std::unordered_map<cache_key_t, trip_id_t, cache_key_hash> cache;

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
    Time earliest_time {backward ? Time::neg_inf : Time::inf};

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
    std::vector<Time> target_labels;
    std::set<node_id_t> marked_stops;
    std::unordered_map<node_id_t, Time> prev_earliest_arrival_time;
    std::unordered_map<node_id_t, Time> earliest_arrival_time;

    // Initialisation
    earliest_arrival_time[source_id] = {departure_time};

    marked_stops.insert(source_id);
    std::unordered_map<node_id_t, Time> tmp_hub_labels;

    if (m_algo == "HLR") {
        auto arrival_time = departure_time + m_timetable->walking_time(source_id, target_id);

        earliest_arrival_time[target_id] = arrival_time;
    }

    target_labels.push_back(earliest_arrival_time[target_id]);

    uint16_t round {1};
    while (true) {
        // First stage, copy the earliest arrival times to the previous round
        prev_earliest_arrival_time = earliest_arrival_time;

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
                        earliest_arrival_time[p_i] = arr;
                        marked_stops.insert(p_i);
                    }
                }

                // Check if we can catch an earlier trip at p_i
                if (prev_earliest_arrival_time[p_i] <= dep) {
                    t = earliest_trip(round, prev_earliest_arrival_time[p_i], route_id, p_i);
                }
            }
        }

        ++round;
        target_labels.push_back(earliest_arrival_time[target_id]);
        if (marked_stops.empty()) break;

        // Third stage, look at footpaths

        if (m_algo == "R") {
            for (const auto& stop_id: marked_stops) {
                for (const auto& transfer: m_timetable->stops(stop_id).transfers) {
                    auto dest_id = transfer.dest;
                    auto transfer_time = transfer.time;

                    auto tmp = earliest_arrival_time[stop_id] + transfer_time;

                    if (tmp < earliest_arrival_time[dest_id]) {
                        earliest_arrival_time[dest_id] = tmp;
                    }

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

                    auto tmp = earliest_arrival_time[stop_id] + walking_time;

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

                        if (tmp < earliest_arrival_time[stop_id]) {
                            earliest_arrival_time[stop_id] = tmp;
                            marked_stops.insert(stop_id);
                        }
                    }
                }
            }
        }

        // The earliest arrival time at target_id could have been changed
        // after scanning the footpaths, thus we need to update the labels
        // of the target here
        target_labels.back() = earliest_arrival_time[target_id];
    }

    Profiler::clear();
    return target_labels;
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
    labels[target_id] = {arrival_time};
    latest_departure_time[target_id] = {arrival_time};
    marked_stops.insert(target_id);
    std::unordered_map<node_id_t, Time> tmp_hub_labels;

    // Since each round of a forward query ends with scanning foot paths,
    // we need to do a backward scanning of foot paths before entering the rounds
    if (m_algo == "R") {
        for (const auto& transfer: m_timetable->stops(target_id).backward_transfers) {
            auto dest_id = transfer.dest;
            auto transfer_time = transfer.time;

            labels[dest_id].back() = std::max(
                    labels[dest_id].back(),
                    arrival_time - transfer_time
            );

            marked_stops.insert(dest_id);
        }
    }
    if (m_algo == "HLR") {
        for (const auto& kv: m_timetable->stops(target_id).in_hubs) {
            auto walking_time = kv.first;
            auto hub_id = kv.second;

            if (walking_time > arrival_time) break;

            auto tmp = arrival_time - walking_time;

            tmp_hub_labels[hub_id] = tmp;

            if (m_timetable->inverse_out_hubs().count(hub_id)) {
                for (const auto& _kv: m_timetable->inverse_out_hubs().at(hub_id)) {
                    auto _walking_time = _kv.first;
                    auto stop_id = _kv.second;

                    auto _tmp = tmp - _walking_time;

                    if (_walking_time + walking_time > arrival_time) break;

                    if (!labels[stop_id].back()) {
                        labels[stop_id].back() = Time::neg_inf;
                    }

                    labels[stop_id].back() = std::max(labels[stop_id].back(), _tmp);
                    marked_stops.insert(stop_id);
                }
            }
        }
    }

    uint16_t round {1};
    while (true) {
        // First stage
        for (const auto& stop: m_timetable->stops()) {
            labels[stop.id].emplace_back(labels[stop.id].back());
        }

        // Second stage
        auto queue = make_queue(marked_stops, true);

        // Traverse each route
        for (const auto& route_stop: queue) {
            auto route_id = route_stop.first;
            auto stop_id = route_stop.second;
            auto& route = m_timetable->routes(route_id);

            trip_id_t t = NULL_TRIP;
            int stop_idx = route.stop_positions.at(stop_id).back();

            // Iterate over the stops of the route beginning with stop_id in the reverse order
            for (int i = stop_idx; i >= 0; --i) {
                node_id_t p_i = route.stops[i];
                Time dep;
                Time arr {Time::neg_inf};

                if (t != NULL_TRIP) {
                    // Get the position of the trip t
                    trip_pos_t trip_pos = m_timetable->trip_positions(t);
                    size_t pos = trip_pos.second;

                    // Get the departure and arrival time of the trip t at the stop p_i
                    dep = route.stop_times[pos][i].dep;
                    arr = route.stop_times[pos][i].arr;

                    // Local and target pruning
                    if (dep > std::max(latest_departure_time[p_i], latest_departure_time[source_id])) {
                        labels[p_i][round] = dep;
                        latest_departure_time[p_i] = dep;
                        marked_stops.insert(p_i);
                    }
                }

                // Check if we can depart by a later trip at p_i
                if (labels[p_i][round - 1] >= arr) {
                    t = earliest_trip(round, labels[p_i][round - 1], route_id, p_i, true);
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

                    if (!tmp_hub_labels[hub_id]) {
                        tmp_hub_labels[hub_id] = Time::neg_inf;
                    }

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

                        if (!latest_departure_time[stop_id]) {
                            latest_departure_time[stop_id] = Time::neg_inf;
                        }

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
    return labels[source_id].back();
}


std::vector<std::pair<Time, Time>> Raptor::profile_query(const node_id_t& source_id, const node_id_t& target_id) {
    std::vector<std::pair<Time, Time>> dep_arr_pairs;

    std::set<Time> arrival_times_queue;
    Time dep {0};
    Time arr;
    static const Time epsilon {1};

    while (true) {
        auto arrival_times = query(source_id, target_id, dep);
        auto pareto_arrival_times = remove_dominated(arrival_times);

        // Push all the possible arrival times from the forward query to the queue
        for (const auto& t: pareto_arrival_times) {
            arrival_times_queue.insert(t);
        }

        // Pick the earliest arrival time from the queue and perform a backward query
        auto begin = arrival_times_queue.begin();
        arr = *begin;
        arrival_times_queue.erase(begin);
        dep = backward_query(source_id, target_id, arr);

        // Add the newly obtained entry to the list of results
        dep_arr_pairs.emplace_back(dep, arr);

        // Change t_dep -> t_dep + ε to continue the forward search
        dep = dep + epsilon;

        if (arrival_times_queue.empty()) break;
    }

    // We need to remove the entries dominated by a pure walking journey
    if (m_algo == "HLR") {
        auto walking_time = m_timetable->walking_time(source_id, target_id);
        std::vector<std::pair<Time, Time>> non_dominated_entries;
        non_dominated_entries.emplace_back(0, walking_time);

        for (const auto& pair: dep_arr_pairs) {
            if (pair.second - pair.first < walking_time) {
                non_dominated_entries.push_back(pair);
            }
        }

        dep_arr_pairs = std::move(non_dominated_entries);
    }

    return dep_arr_pairs;
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

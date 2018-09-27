#include <algorithm> // std::find, std::min
#include <functional>
#include <iostream>
#include <queue>

#include "raptor.hpp"

const node_id_t MAX_STOPS = 100000;
const node_id_t MAX_NODES = 1000000;


// Check if stop1 comes before/after stop2 in the route
bool Raptor::check_stops_order(const route_id_t& route_id, const node_id_t& stop1, const node_id_t& stop2,
                               const bool& backward) {
    #ifdef PROFILE
    Profiler prof {__func__};
    #endif

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
    #ifdef PROFILE
    Profiler prof {__func__};
    #endif

    route_stop_queue_t queue;

    for (const auto& stop_id: marked_stops) {
        const Stop& stop = m_timetable->stops(stop_id);

        for (const auto& route_id: stop.routes) {
            const auto& route_iter = queue.find(route_id);

            // Check if there is already a pair (r, p) in the queue
            if (route_iter != queue.end()) {
                const auto& p = route_iter->second;

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
trip_id_t Raptor::earliest_trip(const route_id_t& route_id, const size_t& stop_idx,
                                const Time& t, const bool& backward) {
    static std::unordered_map<cache_key_t, trip_id_t, std::hash<cache_key_t>> cache;

    #ifdef PROFILE
    auto* prof_c = new Profiler {"cached"};
    #endif

    // We make the label of the stop as a part of the key, instead of the round.
    // This speeds up the function since the label could be the same in several rounds,
    // so that we do not need to find the earliest trip again in another round if the
    // label remains the same.
    cache_key_t key = std::make_tuple(route_id, stop_idx, t.val(), backward);
    auto search = cache.find(key);
    if (search != cache.end()) {
        #ifdef PROFILE
        delete prof_c;
        #endif

        return search->second;
    }

    #ifdef PROFILE
    delete prof_c;
    Profiler prof {__func__};
    #endif

    const auto& route = m_timetable->routes(route_id);

    const auto& stop_times = route.stop_times;
    trip_id_t earliest_trip = NULL_TRIP;

    if (!backward) {
        auto first = stop_times.begin();
        size_t count = stop_times.size();
        size_t step;
        while (count > 0) {
            auto it = first;
            step = count / 2;
            std::advance(it, step);

            const Time& dep = stop_times.at(static_cast<size_t>(it - stop_times.begin())).at(stop_idx).dep;
            if (dep < t) {
                first = ++it;
                count -= step + 1;
            } else {
                count = step;
            }
        }

        if (first == stop_times.end()) return NULL_TRIP;

        return route.trips.at(static_cast<size_t>(first - stop_times.begin()));
    }

    for (size_t j = 0; j < stop_times.size(); ++j) {
        size_t i = stop_times.size() - 1 - j;
        trip_id_t r = route.trips[i];

        // The arrival time of the current trip at stop_id
        const Time& arr = stop_times.at(i).at(stop_idx).arr;

        if (arr <= t) {
            // We are scanning the column corresponding to the stop from bottom to top,
            // i.e., in the decreasing order of time. Thus the first cell such that
            // the arrival time is not later than t corresponds to the earliest trip
            // we can catch at the stop in the backward direction.
            earliest_trip = r;
            break;
        }
    }

    cache[key] = earliest_trip;
    return earliest_trip;
}


std::vector<Time> Raptor::query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time) {
    std::vector<Time> target_labels;

    // Initialisation
    earliest_arrival_time[source_id] = {departure_time};
    prev_earliest_arrival_time[source_id] = {departure_time};

    marked_stops.insert(source_id);

    // If walking is unlimited, we can have a pure walking journey from the source to the target.
    // But in the case of profile queries, we need journeys to contain at least one trip, i.e.,
    // direct walking from s to t is prohibited.
    if (use_hl && !profile) {
        auto target_arrival_time = departure_time + m_timetable->walking_time(source_id, target_id);

        earliest_arrival_time[target_id] = target_arrival_time;
    }

    target_labels.push_back(earliest_arrival_time[target_id]);

    uint16_t round {0};
    while (true) {
        ++round;

        #ifdef PROFILE
        auto* prof_1 = new Profiler {"stage 1"};
        #endif

        // First stage, copy the earliest arrival times to the previous round
        for (const auto& s: marked_stops) {
            prev_earliest_arrival_time[s] = earliest_arrival_time[s];
        }

        #ifdef PROFILE
        delete prof_1;
        #endif

        // Second stage
        auto queue = make_queue(marked_stops);

        #ifdef PROFILE
        auto* prof_2 = new Profiler {"traverse routes"};
        #endif

        // Traverse each route
        for (const auto& route_stop: queue) {
            const auto& route_id = route_stop.first;
            const auto& stop_id = route_stop.second;
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
                    t = earliest_trip(route_id, i, prev_earliest_arrival_time[p_i]);
                }
            }
        }

        #ifdef PROFILE
        delete prof_2;
        #endif

        target_labels.push_back(earliest_arrival_time[target_id]);
        if (marked_stops.empty()) break;

        // Third stage, look at footpaths

        // In the first round, we need to consider also the transfers starting from the source,
        // this was not considered in the original version of RAPTOR
        if (round == 1 && !profile) {
            marked_stops.insert(source_id);
        }

        scan_footpaths(target_id);

        // After having scanned the transfers/foot paths, we remove source_id
        // from the set of marked stops. Leaving it there would change nothing,
        // as we already marked it in the initialisation step, and scanning the routes
        // starting from source_id again is just a duplication of what was already done
        // in the first round.
        if (round == 1 && !profile) {
            marked_stops.erase(source_id);
        }

        // The earliest arrival time at target_id could have been changed
        // after scanning the footpaths, thus we need to update the labels
        // of the target here
        target_labels.back() = earliest_arrival_time[target_id];
    }

    return target_labels;
}


void Raptor::scan_footpaths(const node_id_t& target_id) {
    Time tmp_time;

    if (!use_hl) {
        std::unordered_set<node_id_t> stops_to_mark;

        for (const auto& stop_id: marked_stops) {
            for (const auto& transfer: m_timetable->stops(stop_id).transfers) {
                const auto& dest_id = transfer.dest;
                const auto& transfer_time = transfer.time;

                tmp_time = earliest_arrival_time[stop_id] + transfer_time;

                if (tmp_time < earliest_arrival_time[dest_id]) {
                    earliest_arrival_time[dest_id] = tmp_time;
                    stops_to_mark.insert(dest_id);
                }

                // Since the transfers are sorted in the increasing order of walking time,
                // we can skip the scanning of the transfers as soon as the arrival time
                // of the destination is later than that of the target
                if (tmp_time > earliest_arrival_time[target_id]) break;
            }
        }

        for (const auto& stop_id: stops_to_mark) {
            marked_stops.insert(stop_id);
        }
    } else {
        std::unordered_set<node_id_t> improved_hubs;

        for (const auto& stop_id: marked_stops) {
            for (const auto& kv: m_timetable->stops(stop_id).out_hubs) {
                const auto& walking_time = kv.first;
                const auto& hub_id = kv.second;

                tmp_time = earliest_arrival_time[stop_id] + walking_time;

                // Since we sort the links stop->out-hub in the increasing order of walking time,
                // as soon as the arrival time propagated to a hub is after the earliest arrival time
                // at the target, there is no need to propagate to the next hubs
                if (tmp_time > earliest_arrival_time[target_id]) break;

                if (tmp_time < tmp_hub_labels[hub_id]) {
                    tmp_hub_labels[hub_id] = tmp_time;
                    improved_hubs.insert(hub_id);
                }
            }
        }

        for (const auto& hub_id: improved_hubs) {
            // We need to check if hub_id, which is the out-hub of some stop,
            // is the in-hub of some other stop
            if (m_timetable->inverse_in_hubs()[hub_id].empty()) {
                for (const auto& kv: m_timetable->inverse_in_hubs().at(hub_id)) {
                    const auto& walking_time = kv.first;
                    const auto& stop_id = kv.second;

                    tmp_time = tmp_hub_labels[hub_id] + walking_time;
                    if (tmp_time > earliest_arrival_time[target_id]) break;

                    if (tmp_time < earliest_arrival_time[stop_id]) {
                        earliest_arrival_time[stop_id] = tmp_time;
                        marked_stops.insert(stop_id);
                    }
                }
            }
        }
    }
}


void Raptor::init() {
    earliest_arrival_time.resize(m_timetable->max_stop_id);
    prev_earliest_arrival_time.resize(m_timetable->max_stop_id);

    if (use_hl) {
        tmp_hub_labels.resize(m_timetable->max_node_id);
    }
}


void Raptor::clear() {
    marked_stops.clear();
    earliest_arrival_time.clear();
    prev_earliest_arrival_time.clear();

    if (use_hl) {
        tmp_hub_labels.clear();
    }
}

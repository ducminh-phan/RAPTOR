#include <algorithm> // std::find, std::min
#include <iostream>

#include "raptor.hpp"

bool Raptor::validate_input() {
    size_t max_stop_id = m_timetable->stops().size();

    if ((m_source_id >= max_stop_id) || !m_timetable->stops(m_source_id).is_valid()) {
        std::cerr << "The source stop id of " << m_source_id << " is not a valid stop." << std::endl;
        return false;
    }

    if ((m_target_id >= max_stop_id) || !m_timetable->stops(m_target_id).is_valid()) {
        std::cerr << "The target stop id of " << m_target_id << " is not a valid stop." << std::endl;
        return false;
    }

    if (m_source_id == m_target_id) {
        std::cerr << "The source and the target need to be distinct." << std::endl;
        return false;
    }

    if (m_dep > 86400) {
        std::cerr << "The departure time of " << m_dep << " is invalid." << std::endl;
        return false;
    }

    return true;
}

// Check if stop1 comes before stop2 in the route
bool Raptor::check_stops_order(const route_id_t& route_id, const stop_id_t& stop1, const stop_id_t& stop2) {
    Profiler prof {__func__};

    const auto& route = m_timetable->routes(route_id);
    const auto& idx1 = route.stop_positions.at(stop1).front();
    const auto& idx2 = route.stop_positions.at(stop2).front();

    return idx1 < idx2;
}

route_stop_queue_t Raptor::make_queue() {
    Profiler prof {__func__};

    route_stop_queue_t queue;

    for (const auto& stop_id: m_marked_stops) {
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

    m_marked_stops.clear();

    return queue;
}

// Find the earliest trip in route r that one can catch at stop s in round k,
// i.e., the earliest trip t such that t_dep(t, s) >= t_(k-1) (s)
trip_id_t Raptor::earliest_trip(const uint16_t& round, const route_id_t& route_id, const stop_id_t& stop_id) {
    static std::unordered_map<cache_key_t, trip_id_t, cache_key_hash> cache;
    _time_t t = m_labels[stop_id][round - 1];

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
    _time_t earliest_time;

    // Iterate over the trips
    while (iter != last) {
        trip_id_t r = route.trips[iter - stop_times.begin()];

        // Iterate over the appearances of the stop in the trip
        for (const auto& idx: stop_idx) {
            // The departure time of the current trip at stop_id
            const _time_t& dep = (*iter).at(idx).dep;

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

std::vector<_time_t> Raptor::run() {
    // Initialisation
    for (const auto& stop: m_timetable->stops()) {
        m_earliest_arrival_time[stop.id] = _time_t();
        m_labels[stop.id].emplace_back();
    }
    m_labels[m_source_id] = {m_dep};
    m_marked_stops.insert(m_source_id);

    uint16_t round {1};
    while (true) {
        // First stage, set an upper bound on the earliest arrival time at every stop
        // by copying the arrival time from the previous round
        for (const auto& stop: m_timetable->stops()) {
            m_labels[stop.id].emplace_back(m_labels[stop.id].back());
        }

        // Second stage
        auto queue = make_queue();

        // Traverse each route
        for (const auto& route_stop: queue) {
            auto route_id = route_stop.first;
            auto stop_id = route_stop.second;
            auto& route = m_timetable->routes(route_id);

            trip_id_t t = null_trip;
            size_t stop_idx = route.stop_positions.at(stop_id).front();

            // Iterate over the stops of the route beginning with stop_id
            for (size_t i = stop_idx; i < route.stops.size(); ++i) {
                stop_id_t p_i = route.stops[i];
                _time_t dep, arr;

                if (t != null_trip) {
                    // Get the position of the trip t
                    trip_pos_t trip_pos = m_timetable->trip_positions(t);
                    size_t pos = trip_pos.second;

                    // Get the departure and arrival time of the trip t at the stop p_i
                    dep = route.stop_times[pos][i].dep;
                    arr = route.stop_times[pos][i].arr;

                    // Local and target pruning
                    if (arr < std::min(m_earliest_arrival_time[p_i], m_earliest_arrival_time[m_target_id])) {
                        m_labels[p_i][round] = arr;
                        m_earliest_arrival_time[p_i] = arr;
                        m_marked_stops.insert(p_i);
                    }
                }

                // Check if we can catch an earlier trip at p_i
                if (m_labels[p_i][round - 1] <= dep) {
                    t = earliest_trip(round, route_id, p_i);
                }
            }
        }

        // Third stage, look at footpaths
        for (const auto& stop_id: m_marked_stops) {
            for (const auto& transfer: m_timetable->stops(stop_id).transfers) {
                auto dest_id = transfer.dest;
                auto transfer_time = transfer.time;

                m_labels[dest_id].back() = std::min(
                        m_labels[dest_id].back(),
                        m_labels[stop_id].back() + transfer_time
                );

                m_marked_stops.insert(dest_id);
            }
        }

        ++round;
        if (m_marked_stops.empty()) break;
    }

    Profiler::clear();
    return m_labels[m_target_id];
}

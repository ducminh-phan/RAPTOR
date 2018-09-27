#ifndef RAPTOR_HPP
#define RAPTOR_HPP

#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <utility> // std::pair

#include "config.hpp"
#include "data_structure.hpp"


using route_stop_queue_t = std::unordered_map<route_id_t, node_id_t>;
using labels_t = std::unordered_map<node_id_t, std::vector<Time>>;


class Raptor {
private:
    const Timetable* const m_timetable;

    bool check_stops_order(const route_id_t& route, const node_id_t& stop1, const node_id_t& stop2,
                           const bool& backward = false);

    route_stop_queue_t make_queue(std::set<node_id_t>& marked_stops, const bool& backward = false);

    trip_id_t earliest_trip(const route_id_t& route_id, const size_t& stop_idx,
                            const Time& t, const bool& backward = false);

public:
    explicit Raptor(const Timetable* timetable_p) : m_timetable {timetable_p} {}

    std::vector<Time> query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time);
};


using cache_key_t = std::tuple<route_id_t, size_t, Time::value_type, bool>;


#endif // RAPTOR_HPP

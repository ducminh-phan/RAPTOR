#ifndef RAPTOR_HPP
#define RAPTOR_HPP

#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <utility> // std::pair

#include "data_structure.hpp"

using route_stop_queue_t = std::unordered_map<route_id_t, node_id_t>;
using labels_t = std::unordered_map<node_id_t, std::vector<Time>>;

class Raptor {
private:
    const std::string m_algo;
    const Timetable* const m_timetable;

    bool check_stops_order(const route_id_t& route, const node_id_t& stop1, const node_id_t& stop2);

    route_stop_queue_t make_queue(std::set<node_id_t>& marked_stops);

    trip_id_t earliest_trip(const uint16_t& round, const labels_t& labels,
                            const route_id_t& route_id, const node_id_t& stop_id);

public:
    Raptor(std::string algo, const Timetable* timetable_p) :
            m_algo {std::move(algo)}, m_timetable {timetable_p} {}

    std::vector<Time> query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time);
};

using cache_key_t = std::tuple<Time::value_type, route_id_t, node_id_t>;

struct cache_key_hash : public std::unary_function<cache_key_t, size_t> {
    size_t operator()(const cache_key_t& k) const {
        return std::get<0>(k) ^ std::get<1>(k) ^ std::get<2>(k);
    }
};

#endif // RAPTOR_HPP

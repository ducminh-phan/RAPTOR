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
    const Timetable* const m_timetable;
    const std::string m_algo;
    const std::string m_type;

    bool check_stops_order(const route_id_t& route, const node_id_t& stop1, const node_id_t& stop2,
                           const bool& backward = false);

    route_stop_queue_t make_queue(std::set<node_id_t>& marked_stops, const bool& backward = false);

    trip_id_t earliest_trip(const uint16_t& round, const labels_t& labels,
                            const route_id_t& route_id, const node_id_t& stop_id,
                            const bool& backward = false);

public:
    Raptor(const Timetable* timetable_p, std::string algo, std::string type) :
            m_timetable {timetable_p}, m_algo {std::move(algo)}, m_type {std::move(type)} {}

    std::vector<Time> query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time);

    Time backward_query(const node_id_t& source_id, const node_id_t& target_id, const Time& arrival_time);

    std::vector<std::pair<Time, Time>> profile_query(const node_id_t& source_id, const node_id_t& target_id);
};

using cache_key_t = std::tuple<Time::value_type, route_id_t, node_id_t, bool>;


struct cache_key_hash : public std::unary_function<cache_key_t, size_t> {
    size_t operator()(const cache_key_t& k) const {
        return static_cast<uint32_t>(std::get<0>(k)) ^ std::get<1>(k) ^ std::get<2>(k) ^ std::get<3>(k);
    }
};


std::vector<Time> remove_dominated(const std::vector<Time>& times);

#endif // RAPTOR_HPP

#ifndef RAPTOR_HPP
#define RAPTOR_HPP

#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <utility> // std::pair

#include "data_structure.hpp"

using route_stop_queue_t = std::unordered_map<route_id_t, stop_id_t>;

class Raptor {
private:
    const Timetable* const m_timetable;
    const stop_id_t m_source_id;
    const stop_id_t m_target_id;
    const _time_t m_dep;
    std::set<stop_id_t> m_marked_stops;
    std::unordered_map<stop_id_t, _time_t> m_earliest_arrival_time;
    std::unordered_map<stop_id_t, std::vector<_time_t>> m_labels;

    bool validate_input();

    bool check_stops_order(const route_id_t& route, const stop_id_t& stop1, const stop_id_t& stop2);

    route_stop_queue_t make_queue();

    trip_id_t earliest_trip(const uint16_t& round, const route_id_t& route_id, const stop_id_t& stop_id);

public:
    Raptor(Timetable* timetable_p, stop_id_t source_id, stop_id_t target_id, _time_t departure_time) :
            m_timetable {timetable_p}, m_source_id {source_id}, m_target_id {target_id}, m_dep {departure_time} {}

    std::vector<_time_t> raptor();
};

using cache_key_t = std::tuple<_time_t::value_type, route_id_t, stop_id_t>;

struct cache_key_hash : public std::unary_function<cache_key_t, size_t> {
    size_t operator()(const cache_key_t& k) const {
        return std::get<0>(k) ^ std::get<1>(k) ^ std::get<2>(k);
    }
};

#endif // RAPTOR_HPP

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
    const Timetable* const timetable;
    const _time_t dep;
    const stop_id_t source;
    const stop_id_t target;
    std::set<stop_id_t> marked_stops;
    std::unordered_map<stop_id_t, _time_t> earliest_arrival_time;
    std::unordered_map<stop_id_t, std::vector<_time_t>> labels;

    bool validate_input();

    bool check_stops_order(const route_id_t& route, const stop_id_t& stop1, const stop_id_t& stop2);

    route_stop_queue_t make_queue();

    trip_id_t earliest_trip(const uint16_t& round, const route_id_t& route_id, const stop_id_t& stop_id);

public:
    Raptor(Timetable* timetable_, stop_id_t source_id, stop_id_t target_id, _time_t departure) :
            timetable {timetable_}, source {source_id}, target {target_id}, dep {departure} {}

    std::vector<_time_t> raptor();
};

using key_t = std::tuple<_time_t::value_type, route_id_t, stop_id_t>;

struct key_hash : public std::unary_function<key_t, size_t> {
    size_t operator()(const key_t& k) const {
        return std::get<0>(k) ^ std::get<1>(k) ^ std::get<2>(k);
    }
};

#endif // RAPTOR_HPP

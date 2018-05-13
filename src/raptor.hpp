#ifndef RAPTOR_HPP
#define RAPTOR_HPP

#include <unordered_map>
#include <unordered_set>
#include <utility> // std::pair

#include "data_structure.hpp"

using route_stop_t = std::pair<route_id_t, stop_id_t>;
using route_stop_queue_t = std::vector<route_stop_t>;

class Raptor {
private:
    const Timetable* const timetable;
    const _time_t dep;
    const stop_id_t source;
    const stop_id_t target;
    std::unordered_set<stop_id_t> marked_stops;

    bool validate_input();

    bool check_stops_order(const route_id_t& route, const stop_id_t& stop1, const stop_id_t& stop2);

    route_stop_queue_t make_queue();

public:
    Raptor(Timetable* timetable_, stop_id_t source_id, stop_id_t target_id, _time_t departure) :
            timetable {timetable_}, source {source_id}, target {target_id}, dep {departure}
    {};

    void raptor();
};

route_stop_queue_t::iterator find(route_stop_queue_t& queue, const route_id_t& route, const stop_id_t& stop);

#endif // RAPTOR_HPP

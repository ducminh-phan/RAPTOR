#ifndef RAPTOR_HPP
#define RAPTOR_HPP

#include <utility> // std::pair

#include "data_structure.hpp"

using route_stop_t = std::pair<route_id_t, stop_id_t>;
using route_stop_queue_t = std::vector<route_stop_t>;

bool validate_input(const Timetable& timetable, const _time_t& dep, const stop_id_t& source, const stop_id_t& target);

route_stop_queue_t::iterator find(route_stop_queue_t& queue, const route_id_t& route, const stop_id_t& stop);

bool check_stops_order(const Timetable& timetable, const route_id_t& route,
                       const stop_id_t& stop1, const stop_id_t& stop2);

route_stop_queue_t make_queue(Timetable& timetable);

void raptor(Timetable& timetable, const _time_t& dep, const stop_id_t& source, const stop_id_t& target);


#endif // RAPTOR_HPP

#ifndef RAPTOR_HPP
#define RAPTOR_HPP

#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility> // std::pair

#include "config.hpp"
#include "data_structure.hpp"


using route_stop_queue_t = std::unordered_map<route_id_t, node_id_t>;


class Raptor {
private:
    const Timetable* const m_timetable;
    bool stops_improved = false;
    std::vector<bool> stop_is_marked;
    std::vector<Time> prev_earliest_arrival_time;
    std::vector<Time> earliest_arrival_time;
    std::vector<Time> tmp_hub_labels;

    bool check_stops_order(const route_id_t& route, const node_id_t& stop1, const node_id_t& stop2);

    route_stop_queue_t make_queue();

    trip_id_t earliest_trip(const route_id_t& route_id, const size_t& stop_idx, const Time& t);

    void scan_footpaths(const node_id_t& target_id);

public:
    explicit Raptor(const Timetable* timetable_p) : m_timetable {timetable_p} {}

    std::vector<Time> query(const node_id_t& source_id, const node_id_t& target_id, const Time& departure_time);

    void init();

    void clear();
};


#endif // RAPTOR_HPP

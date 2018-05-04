#ifndef DATA_STRUCTURE_HPP
#define DATA_STRUCTURE_HPP

#include <cstdint>
#include <vector>

using stop_id_t = uint32_t;
using trip_id_t = uint64_t;
using route_id_t = uint32_t;
using _time_t = uint32_t;

struct Stop;

struct Route;

struct Transfer {
    Stop* dest;
    _time_t time;
};

struct Stop {
    stop_id_t id;
    std::vector<Transfer> transfers;
    std::vector<Route*> stop_routes;
};

struct StopTime {
    Stop* stop_id;
    _time_t arr;
    _time_t dep;
};

struct Trip {
    trip_id_t id;
    std::vector<StopTime> stop_times;
};

struct Route {
    route_id_t id;
    std::vector<Trip> route_trips;
};

struct Data {
    std::vector<Route> routes;
    std::vector<Stop> stops;
};

#endif // DATA_STRUCTURE_HPP

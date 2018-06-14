#ifndef DATA_STRUCTURE_HPP
#define DATA_STRUCTURE_HPP

#include <cstdint>
#include <iostream>
#include <memory> // std::unique_ptr
#include <limits> // std::numeric_limits
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "utilities.hpp"

using node_id_t = uint32_t;
using trip_id_t = int32_t;
using route_id_t = uint16_t;
using trip_pos_t = std::pair<route_id_t, size_t>;
using distance_t = uint32_t;

extern const trip_id_t null_trip;

class _time_t {
public:
    using value_type = uint32_t;
private:
    value_type m_val;
public:
    const static value_type inf = std::numeric_limits<value_type>::max();

    _time_t() : m_val {inf} {};

    _time_t(value_type val_) : m_val {val_} {}

    const value_type& val() { return m_val; }

    friend _time_t operator+(const _time_t& t1, const _time_t t2) { return {t1.m_val + t2.m_val}; }

    friend bool operator<(const _time_t& t1, const _time_t& t2) { return t1.m_val < t2.m_val; }

    friend bool operator>(const _time_t& t1, const _time_t& t2) { return t1.m_val > t2.m_val; }

    friend bool operator>(const _time_t& t1, const value_type& t2) { return t1.m_val > t2; }

    friend bool operator>=(const _time_t& t1, const _time_t& t2) { return !(t1 < t2); }

    friend bool operator<=(const _time_t& t1, const _time_t& t2) { return !(t1 > t2); }

    friend std::ostream& operator<<(std::ostream& out, const _time_t& t) {
        out << t.m_val;
        return out;
    }
};

struct Stop {
    node_id_t id;
    std::vector<route_id_t> routes;
    std::unordered_map<node_id_t, _time_t> in_hubs;
    std::unordered_map<node_id_t, _time_t> out_hubs;

    bool is_valid() const { return !routes.empty(); }
};

struct StopTime {
    node_id_t stop_id;
    _time_t arr;
    _time_t dep;

    StopTime(node_id_t s, _time_t a, _time_t d) : stop_id {s}, arr {a}, dep {d} {};
};

struct Route {
    route_id_t id;
    std::vector<trip_id_t> trips;
    std::vector<node_id_t> stops;
    std::vector<std::vector<StopTime>> stop_times;
    std::unordered_map<node_id_t, std::vector<size_t>> stop_positions;
};

class Timetable {
private:
    std::string m_name;
    std::string m_path;
    std::vector<Route> m_routes;
    std::vector<Stop> m_stops;
    std::unordered_map<trip_id_t, trip_pos_t> m_trip_positions;

    void parse_data();

    void parse_trips();

    void parse_stop_routes();

    void parse_hubs();

    void parse_stop_times();

public:
    const std::string& name() const { return m_name; }

    const std::string& path() const { return m_path; }

    const std::vector<Route>& routes() const { return m_routes; }

    const Route& routes(route_id_t route_id) const { return m_routes[route_id]; }

    const std::vector<Stop>& stops() const { return m_stops; }

    const Stop& stops(node_id_t stop_id) const { return m_stops[stop_id]; }

    const trip_pos_t& trip_positions(trip_id_t trip_id) const { return m_trip_positions.at(trip_id); }

    explicit Timetable(std::string city_name) : m_name {std::move(city_name)} {
        m_path = "../Public-Transit-Data/" + m_name + "/";
        parse_data();
    }

    void summary() const;
};

_time_t distance_to_time(const distance_t& d);

#endif // DATA_STRUCTURE_HPP

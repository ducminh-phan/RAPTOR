#ifndef DATA_STRUCTURE_HPP
#define DATA_STRUCTURE_HPP

#include <cstdint>
#include <iostream>
#include <memory> // std::unique_ptr
#include <limits> // std::numeric_limits
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utilities.hpp"

using node_id_t = uint32_t;
using trip_id_t = int32_t;
using route_id_t = uint16_t;
using trip_pos_t = std::pair<route_id_t, size_t>;
using distance_t = uint32_t;

extern const trip_id_t NULL_TRIP;

class Time {
public:
    using value_type = uint32_t;
private:
    value_type m_val;
public:
    const static value_type inf = std::numeric_limits<value_type>::max();

    Time() : m_val {inf} {};

    explicit Time(value_type val_) : m_val {val_} {}

    const value_type& val() { return m_val; }

    friend Time operator+(const Time& t1, const Time& t2) {
        // Make sure ∞ + c = ∞
        if (t1.m_val == inf || t2.m_val == inf) return {};

        // m_val is small compared to inf, so we don't have to worry about overflow here
        return Time(t1.m_val + t2.m_val);
    }

    friend Time operator-(const Time& t1, const Time& t2) {
        if (t1.m_val <= t2.m_val) return Time(0);

        return Time(t1.m_val - t2.m_val);
    }

    friend bool operator<(const Time& t1, const Time& t2) { return t1.m_val < t2.m_val; }

    friend bool operator>(const Time& t1, const Time& t2) { return t1.m_val > t2.m_val; }

    friend bool operator>=(const Time& t1, const Time& t2) { return !(t1 < t2); }

    friend bool operator<=(const Time& t1, const Time& t2) { return !(t1 > t2); }

    explicit operator bool() const { return m_val < inf; }

    friend std::ostream& operator<<(std::ostream& out, const Time& t) {
        out << t.m_val;
        return out;
    }
};

using hubs_t = std::vector<std::pair<Time, node_id_t>>;
using inverse_hubs_t = std::unordered_map<node_id_t, hubs_t>;

struct Transfer {
    node_id_t dest;
    Time time;

    Transfer(node_id_t dest, Time::value_type time) : dest {dest}, time {time} {};
};

struct Stop {
    node_id_t id;
    std::vector<route_id_t> routes;
    std::vector<Transfer> transfers;
    std::vector<Transfer> backward_transfers;
    hubs_t in_hubs;
    hubs_t out_hubs;

    bool is_valid() const { return !routes.empty(); }
};

struct StopTime {
    node_id_t stop_id;
    Time arr;
    Time dep;

    StopTime(node_id_t s, Time::value_type a, Time::value_type d) : stop_id {s}, arr {a}, dep {d} {};
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
    std::string m_algo;
    std::string m_path;
    std::vector<Route> m_routes;
    std::vector<Stop> m_stops;
    std::unordered_map<trip_id_t, trip_pos_t> m_trip_positions;
    inverse_hubs_t m_inverse_in_hubs;
    inverse_hubs_t m_inverse_out_hubs;

    void parse_data();

    void parse_trips();

    void parse_stop_routes();

    void parse_transfers();

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

    const inverse_hubs_t& inverse_in_hubs() const { return m_inverse_in_hubs; }

    const inverse_hubs_t& inverse_out_hubs() const { return m_inverse_out_hubs; }

    Timetable(std::string name, std::string algo) : m_name {std::move(name)}, m_algo {std::move(algo)} {
        m_path = "../Public-Transit-Data/" + m_name + "/";
        parse_data();
    }

    void summary() const;
};

Time distance_to_time(const distance_t& d);

#endif // DATA_STRUCTURE_HPP

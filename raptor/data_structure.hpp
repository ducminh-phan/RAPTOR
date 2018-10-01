#ifndef DATA_STRUCTURE_HPP
#define DATA_STRUCTURE_HPP

#include <cstdint>
#include <iostream>
#include <memory> // std::unique_ptr
#include <limits> // std::numeric_limits
#include <string>
#include <utility>
#include <vector>

#include "config.hpp"
#include "utilities.hpp"


using node_id_t = uint32_t;
using trip_id_t = int32_t;
using route_id_t = uint16_t;
using trip_pos_t = std::pair<route_id_t, size_t>;
using distance_t = uint32_t;

extern const trip_id_t NULL_TRIP;


class Time {
public:
    using value_type = int32_t;
private:
    value_type m_val;
public:
    const static value_type inf = std::numeric_limits<value_type>::max();

    const static value_type neg_inf = std::numeric_limits<value_type>::min();

    Time() : m_val {inf} {};

    explicit Time(value_type val) : m_val {val} {}

    const value_type& val() const { return m_val; }

    friend Time operator+(const Time& t1, const Time& t2) {
        // Make sure ∞ + c = ∞
        if (t1.m_val == inf || t2.m_val == inf) return {};

        // m_val is small compared to inf, so we don't have to worry about overflow here
        return Time(t1.m_val + t2.m_val);
    }

    friend Time operator-(const Time& t1, const Time& t2) {
        if (t1.m_val < t2.m_val) return Time(neg_inf);

        return Time(t1.m_val - t2.m_val);
    }

    friend bool operator<(const Time& t1, const Time& t2) { return t1.m_val < t2.m_val; }

    friend bool operator>(const Time& t1, const Time& t2) { return t1.m_val > t2.m_val; }

    friend bool operator>=(const Time& t1, const Time& t2) { return !(t1 < t2); }

    friend bool operator<=(const Time& t1, const Time& t2) { return !(t1 > t2); }

    friend bool operator==(const Time& t1, const Time& t2) { return t1.m_val == t2.m_val; }

    explicit operator bool() const { return m_val < inf; }

    Time& operator=(const value_type val) {
        this->m_val = val;
        return *this;
    }

    Time& operator=(const Time& other) = default;

    friend std::ostream& operator<<(std::ostream& out, const Time& t) {
        out << t.m_val;
        return out;
    }
};


using hubs_t = std::vector<std::pair<Time, node_id_t>>;
using inverse_hubs_t = std::vector<hubs_t>;


struct Transfer {
    node_id_t dest;
    Time time;

    Transfer(node_id_t dest, Time::value_type time) : dest {dest}, time {time} {};

    friend bool operator<(const Transfer& t1, const Transfer& t2) {
        return (t1.time < t2.time) || ((t1.time == t2.time) && (t1.dest < t2.dest));
    }
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
    std::vector<size_t> stop_positions;
};


class Timetable {
private:
    void parse_data();

    void parse_trips();

    void parse_stop_routes();

    void parse_transfers();

    void parse_hubs();

    void parse_stop_times();

public:
    std::string path;
    std::size_t max_stop_id = 0;
    std::size_t max_node_id = 0;
    std::vector<Route> routes;
    std::vector<Stop> stops;
    std::vector<trip_pos_t> trip_positions;
    inverse_hubs_t inverse_in_hubs;
    inverse_hubs_t inverse_out_hubs;

    Time walking_time(const node_id_t& source_id, const node_id_t& target_id) const;

    Timetable() {
        path = "../Public-Transit-Data/" + name + "/";
        parse_data();
    }

    void summary() const;
};


Time distance_to_time(const distance_t& d);

#endif // DATA_STRUCTURE_HPP

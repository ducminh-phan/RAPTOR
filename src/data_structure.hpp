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

using stop_id_t = uint16_t;
using trip_id_t = int32_t;
using route_id_t = uint16_t;
using trip_pos_t = std::pair<route_id_t, size_t>;
extern const trip_id_t null_trip;

class _time_t {
private:
    using value_type = uint32_t;

    value_type m_val;
public:
    const static value_type inf = std::numeric_limits<value_type>::max();

    _time_t() : m_val {inf} {};

    _time_t(value_type val_) : m_val {val_} {}

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

struct Transfer {
    stop_id_t dest;
    _time_t time;

    Transfer(stop_id_t dest, _time_t time) : dest {dest}, time {time} {};
};

struct Stop {
    stop_id_t id;
    std::vector<Transfer> transfers;
    std::vector<route_id_t> routes;

    bool is_valid() const { return !routes.empty(); }
};

struct StopTime {
    stop_id_t stop_id;
    _time_t arr;
    _time_t dep;

    StopTime(stop_id_t s, _time_t a, _time_t d) : stop_id {s}, arr {a}, dep {d} {};
};

struct Route {
    route_id_t id;
    std::vector<trip_id_t> trips;
    std::vector<stop_id_t> stops;
    std::vector<std::vector<StopTime>> stop_times;
    std::unordered_map<stop_id_t, size_t> stop_positions;
};

class Timetable {
private:
    std::string m_city_name;
    std::string m_path;
    std::vector<Route> m_routes;
    std::vector<Stop> m_stops;
    std::unordered_map<trip_id_t, trip_pos_t> m_trip_positions;

    template<class T>
    std::unique_ptr<T> read_dataset_file(const std::string& file_name) {
        // Since the copy constructor of std::istream is deleted,
        // we need to return a new object by pointer
        std::unique_ptr<T> file_ptr {new T {(m_path + file_name).c_str()}};

        check_file_exists(*file_ptr, file_name);

        return file_ptr;
    }

    void parse_data();

    void parse_trips();

    void parse_stop_routes();

    void parse_transfers();

    void parse_stop_times();

public:
    const std::string& city_name() const { return m_city_name; }

    const std::vector<Route>& routes() const { return m_routes; }

    const Route& routes(route_id_t route_id) const { return m_routes[route_id]; }

    const std::vector<Stop>& stops() const { return m_stops; }

    const Stop& stops(stop_id_t stop_id) const { return m_stops[stop_id]; }

    const trip_pos_t& trip_positions(trip_id_t trip_id) const { return m_trip_positions.at(trip_id); }

    explicit Timetable(std::string city_name) : m_city_name {std::move(city_name)} {
        m_path = "../Public-Transit-Data/" + m_city_name + "/";
        parse_data();
    }

    void summary();
};

#endif // DATA_STRUCTURE_HPP

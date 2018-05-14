#ifndef DATA_STRUCTURE_HPP
#define DATA_STRUCTURE_HPP

#include <cstdint>
#include <limits> // std::numeric_limits
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using stop_id_t = uint16_t;
using trip_id_t = int32_t;
using route_id_t = uint16_t;
const trip_id_t null_trip = -1;
// const trip_id_t null_trip = std::numeric_limits<trip_id_t>::max();

class _time_t {
private:
    using value_type = uint32_t;

    value_type m_val;
public:
    const static value_type inf = std::numeric_limits<value_type>::max();

    _time_t() : m_val {inf} {};

    _time_t(value_type val_) : m_val {val_} {}

    const value_type& val() const { return m_val; }

    value_type& val() { return m_val; }

    _time_t& operator=(value_type&& new_val) { m_val = new_val; }
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
    using trip_pos_t = std::pair<route_id_t, size_t>;

    std::string m_city_name;
    std::string m_path;
    std::vector<Route> m_routes;
    std::vector<Stop> m_stops;
    std::unordered_map<trip_id_t, trip_pos_t> m_trip_positions;

    std::ifstream read_dataset_file(const std::string& file_name);

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

    explicit Timetable(std::string city_name) : m_city_name {std::move(city_name)} {
        m_path = "../Public-Transit-Data/" + m_city_name + "/";
        parse_data();
    }

    void summary();
};

#endif // DATA_STRUCTURE_HPP

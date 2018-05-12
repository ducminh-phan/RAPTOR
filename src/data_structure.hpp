#ifndef DATA_STRUCTURE_HPP
#define DATA_STRUCTURE_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using stop_id_t = uint16_t;
using trip_id_t = uint32_t;
using route_id_t = uint16_t;
using _time_t = uint32_t;

struct Transfer {
    stop_id_t dest;
    _time_t time;

    Transfer(stop_id_t dest, _time_t time) : dest {dest}, time {time} {};
};

struct Stop {
    stop_id_t id;
    std::vector<Transfer> transfers;
    std::vector<route_id_t> routes;
    bool is_marked = false;

    bool is_valid() const { return !routes.empty(); }

    bool is_valid() { return !routes.empty(); }
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

    const std::string& city_name() { return m_city_name; }

    const std::vector<Route>& routes() const { return m_routes; }

    const std::vector<Route>& routes() { return m_routes; }

    const std::vector<Stop>& stops() const { return m_stops; }

    std::vector<Stop>& stops() { return m_stops; }

    explicit Timetable(std::string city_name) : m_city_name {std::move(city_name)} {
        m_path = "../Public-Transit-Data/" + m_city_name + "/";
        parse_data();
    };

    void summary();
};

#endif // DATA_STRUCTURE_HPP

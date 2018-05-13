#include <algorithm> // std::find
#include <iostream>
#include <limits> // std::numeric_limits

#include "raptor.hpp"

const _time_t inf = std::numeric_limits<_time_t>::max();

bool Raptor::validate_input() {
    size_t max_stop_id = timetable->stops().size();

    if ((source >= max_stop_id) || !timetable->stops()[source].is_valid()) {
        std::cerr << "The source stop id of " << source << " is not a valid stop." << std::endl;
        return false;
    }

    if ((target >= max_stop_id) || !timetable->stops()[target].is_valid()) {
        std::cerr << "The target stop id of " << target << " is not a valid stop." << std::endl;
        return false;
    }

    if (source == target) {
        std::cerr << "The source and the target need to be distinct." << std::endl;
        return false;
    }

    if (dep > 86400) {
        std::cerr << "The departure time of " << dep << " is invalid." << std::endl;
        return false;
    }

    return true;
}

route_stop_queue_t::iterator find(route_stop_queue_t& queue, const route_id_t& route, const stop_id_t& stop) {
    auto iter = queue.begin();
    auto last = queue.end();

    while (iter != last) {
        if ((iter->first == route) && (iter->second != stop)) return iter;
        ++iter;
    }

    return last;
}

// Check is stop1 comes before stop2 in the route
bool Raptor::check_stops_order(const route_id_t& route, const stop_id_t& stop1, const stop_id_t& stop2) {
    const std::vector<stop_id_t>& stops = timetable->routes()[route].stops;
    auto idx1 = std::find(stops.begin(), stops.end(), stop1) - stops.begin();
    auto idx2 = std::find(stops.begin(), stops.end(), stop2) - stops.begin();

    if (idx1 >= stops.size() || idx2 >= stops.size()) {
        std::cerr << "The stops do not belong to the route" << std::endl;
        return false;
    }

    return idx1 < idx2;
}

route_stop_queue_t Raptor::make_queue() {
    route_stop_queue_t queue;
    queue.emplace_back(0, 415);

    for (const auto& stop_id: marked_stops) {
        const Stop& stop = timetable->stops()[stop_id];

        for (const auto& route: stop.routes) {
            bool found = false;

            while (true) {
                // Check if (r, p') in Q for some stop p' != p
                auto iter = find(queue, route, stop_id);
                if (iter == queue.end()) break;

                found = true;

                // If p comes before p' in r, then substitute (r, p') by (r, p)
                if (check_stops_order(route, stop_id, iter->second)) {
                    iter->second = stop_id;
                }
            }

            if (!found) queue.emplace_back(route, stop_id);
        }
    }

    marked_stops.clear();

    return queue;
}

void Raptor::raptor() {
    std::cout << std::boolalpha << validate_input() << std::endl;

    marked_stops.insert(source);
    std::cout << marked_stops.size() << std::endl;

    auto queue = make_queue();
}

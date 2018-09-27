#include <algorithm>
#include <cmath>

#include "hub_labelling.hpp"
#include "csv.h"
#include "gzstream.h"

extern const Distance infty = std::numeric_limits<Distance>::max();

void GraphLabel::parse_hub_files() {
    igzstream in_hubs_file_stream {(_path + "in_hubs.gr.gz").c_str()};
    io::CSVReader<3> in_hubs_reader {"in_hubs.gr", in_hubs_file_stream};
    in_hubs_reader.set_header("node_id", "stop_id", "distance");

    Node node_id;
    Node stop_id;
    Distance distance;

    while (in_hubs_reader.read_row(node_id, stop_id, distance)) {
        in_labels[stop_id].hubs.emplace_back(node_id);
        in_labels[stop_id].distances.emplace_back(distance);
    }

    igzstream out_hubs_file_stream {(_path + "out_hubs.gr.gz").c_str()};
    io::CSVReader<3> out_hubs_reader {"out_hubs.gr", out_hubs_file_stream};
    out_hubs_reader.set_header("stop_id", "node_id", "distance");

    while (out_hubs_reader.read_row(stop_id, node_id, distance)) {
        out_labels[stop_id].hubs.emplace_back(node_id);
        out_labels[stop_id].distances.emplace_back(distance);
    }

    sort();
}

void GraphLabel::parse_weights() {
    igzstream trips_file_stream {(_path + "trips.csv.gz").c_str()};
    io::CSVReader<1> trips_file_reader {"trips.csv", trips_file_stream};
    trips_file_reader.read_header(io::ignore_missing_column, "route_id");

    route_id_t route_id;

    // Count the number of trips a route has
    std::unordered_map<route_id_t, size_t> trips_count;

    while (trips_file_reader.read_row(route_id)) {
        trips_count[route_id] += 1;
    }

    for (const auto& kv: in_labels) {
        stop_to_weight[kv.first] = 0;
    }

    for (const auto& kv: out_labels) {
        stop_to_weight[kv.first] = 0;
    }

    igzstream stop_routes_file_stream {(_path + "stop_routes.csv.gz").c_str()};
    io::CSVReader<2> stop_routes_reader {"stop_routes.csv", stop_routes_file_stream};
    stop_routes_reader.read_header(io::ignore_no_column, "stop_id", "route_id");

    Node stop_id;

    while (stop_routes_reader.read_row(stop_id, route_id)) {
        // We only add stops in the walking graph
        if (stop_to_weight.count(stop_id)) {
            stop_to_weight[stop_id] += trips_count.at(route_id);
        }
    }

    for (const auto& kv: stop_to_weight) {
        stops.push_back(kv.first);
        weights.push_back(kv.second);
    }
}

// Sort the labels for all nodes
void GraphLabel::sort() {
    for (auto& kv: in_labels) {
        sort(kv.second);
    }

    for (auto& kv: out_labels) {
        sort(kv.second);
    }
}

// Sort the labels in the increasing order of hub id
void GraphLabel::sort(NodeLabel& node_labels) {
    std::vector<std::pair<Node, Distance>> labels;

    // Create the vector of pairs node, dist then sort it
    for (size_t i = 0; i < node_labels.size(); ++i) {
        labels.emplace_back(node_labels.hubs[i], node_labels.distances[i]);
    }

    std::sort(labels.begin(), labels.end());

    // Assign back after sorting the vector of pairs node, dist
    for (size_t i = 0; i < node_labels.size(); ++i) {
        node_labels.hubs[i] = labels[i].first;
        node_labels.distances[i] = labels[i].second;
    }
}

Distance GraphLabel::shortest_path_length(const Node& u, const Node& v) const {
    Distance d = infty;
    const auto& u_out_labels = out_labels.at(u);
    const auto& v_in_labels = in_labels.at(v);

    for (size_t i = 0, j = 0; i < u_out_labels.size() && j < v_in_labels.size();) {
        const auto& out_hub_u = u_out_labels.hubs[i];
        const auto& in_hub_v = v_in_labels.hubs[j];

        if (out_hub_u == in_hub_v) {
            d = std::min(d, u_out_labels.distances[i] + v_in_labels.distances[j]);
            ++i;
            ++j;
        } else if (out_hub_u < in_hub_v) {
            ++i;
        } else {
            ++j;
        }
    }

    return d;
}

const std::vector<std::pair<Distance, Node>> GraphLabel::single_source_shortest_path_length(const Node& source) const {
    std::vector<std::pair<Distance, Node>> res;
    std::unordered_map<Node, Distance> distance_labels;

    const auto& source_out_labels = out_labels.at(source);

    // Propagate the distance to the out-hubs of the source
    for (size_t i = 0; i < source_out_labels.size(); ++i) {
        distance_labels[source_out_labels.hubs[i]] = source_out_labels.distances[i];
    }

    // Propagate the distance from the hubs to all the nodes in the graph
    for (const auto& kv: in_labels) {
        Distance d = infty;

        auto& target = kv.first;
        auto& target_in_labels = kv.second;

        for (size_t i = 0; i < target_in_labels.size(); ++i) {
            auto hub = target_in_labels.hubs[i];
            auto d_ht = target_in_labels.distances[i];
            auto it = distance_labels.find(hub);

            // If hub is common between the source and the target,
            // propagate the distance to the target
            if (it != distance_labels.end()) {
                d = std::min(d, it->second + d_ht);
            }
        }

        if (d < infty) {
            res.emplace_back(d, target);
        }
    }

    std::sort(res.begin(), res.end());

    return res;
}

const std::vector<Node> GraphLabel::sssp_sorted_stops(const Node& source) const {
    auto sssp_length = single_source_shortest_path_length(source);

    std::vector<Node> res;
    for (const auto& elem: sssp_length) {
        res.push_back(elem.second);
    }

    return res;
}

size_t GraphLabel::compute_rank(const Node& source, const Node& target) const {
    auto sorted_stops = sssp_sorted_stops(source);

    // Get the index of the target
    auto iter = std::find(sorted_stops.begin(), sorted_stops.end(), target);
    size_t idx = static_cast<size_t>(iter - sorted_stops.begin());

    // Get the rank
    auto rank = static_cast<size_t>(std::floor(std::log2(idx)));

    return rank;
}

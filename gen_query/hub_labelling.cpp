#include <algorithm>

#include "hub_labelling.hpp"
#include "csv_reader.hpp"
#include "gzstream.h"

extern const Distance infty = std::numeric_limits<Distance>::max();

void GraphLabel::parse_hub_files() {
    auto in_hub_file = read_dataset_file<igzstream>(_path + "in_hubs.gr.gz");

    for (CSVIterator<uint32_t> iter {in_hub_file.get(), false, ' '}; iter != CSVIterator<uint32_t>(); ++iter) {
        Node hub = (*iter)[0];
        Node node_id = (*iter)[1];
        Distance dist = (*iter)[2];

        in_labels[node_id].hubs.emplace_back(hub);
        in_labels[node_id].distances.emplace_back(dist);
    }

    auto out_hub_file = read_dataset_file<igzstream>(_path + "out_hubs.gr.gz");

    for (CSVIterator<uint32_t> iter {out_hub_file.get(), false, ' '}; iter != CSVIterator<uint32_t>(); ++iter) {
        Node node_id = (*iter)[0];
        Node hub = (*iter)[1];
        Distance dist = (*iter)[2];

        out_labels[node_id].hubs.emplace_back(hub);
        out_labels[node_id].distances.emplace_back(dist);
    }

    sort();
}

void GraphLabel::parse_weights() {
    auto trips_file = read_dataset_file<igzstream>(_path + "trips.csv.gz");
    auto stop_routes_file = read_dataset_file<igzstream>(_path + "stop_routes.csv.gz");

    // Count the number of trips a route has
    std::unordered_map<route_id_t, size_t> trips_count;

    for (CSVIterator<uint32_t> iter {trips_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto route_id = static_cast<route_id_t>((*iter)[0]);
        trips_count[route_id] += 1;
    }

    for (const auto& kv: in_labels) {
        stop_to_weight[kv.first] = 0;
    }

    for (const auto& kv: out_labels) {
        stop_to_weight[kv.first] = 0;
    }

    for (CSVIterator<uint32_t> iter {stop_routes_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto stop_id = static_cast<Node>((*iter)[0]);
        auto route_id = static_cast<route_id_t>((*iter)[1]);

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

const Distance GraphLabel::shortest_path_length(const Node& u, const Node& v) const {
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

    if (!out_labels.count(source)) std::cout << "wtf " << source << std::endl;
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

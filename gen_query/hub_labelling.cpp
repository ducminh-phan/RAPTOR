#include <algorithm>

#include "hub_labelling.hpp"
#include "csv_reader.hpp"
#include "gzstream.h"

extern const Distance infty = std::numeric_limits<Distance>::max();

void GraphLabel::parse_hub_files() {
    auto in_hub_file = read_dataset_file<igzstream>(m_path + "in_hub.gr.gz");

    for (CSVIterator<uint32_t> iter {in_hub_file.get(), false, ' '}; iter != CSVIterator<uint32_t>(); ++iter) {
        Node hub = (*iter)[0];
        Node node_id = (*iter)[1];
        Distance dist = (*iter)[2];

        m_in_labels[node_id].hubs.emplace_back(hub);
        m_in_labels[node_id].distances.emplace_back(dist);
    }

    auto out_hub_file = read_dataset_file<igzstream>(m_path + "out_hub.gr.gz");

    for (CSVIterator<uint32_t> iter {out_hub_file.get(), false, ' '}; iter != CSVIterator<uint32_t>(); ++iter) {
        Node node_id = (*iter)[0];
        Node hub = (*iter)[1];
        Distance dist = (*iter)[2];

        m_out_labels[node_id].hubs.emplace_back(hub);
        m_out_labels[node_id].distances.emplace_back(dist);
    }

    sort();
}

// Sort the labels for all nodes
void GraphLabel::sort() {
    for (auto& kv: m_in_labels) {
        sort(kv.second);
    }

    for (auto& kv: m_out_labels) {
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
    const auto& u_out_labels = m_out_labels.at(u);
    const auto& v_in_labels = m_in_labels.at(v);

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

    const auto& source_out_labels = m_out_labels.at(source);

    // Propagate the distance to the out-hubs of the source
    for (size_t i = 0; i < source_out_labels.size(); ++i) {
        distance_labels[source_out_labels.hubs[i]] = source_out_labels.distances[i];
    }

    // Propagate the distance from the hubs to all the nodes in the graph
    for (const auto& kv: m_in_labels) {
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

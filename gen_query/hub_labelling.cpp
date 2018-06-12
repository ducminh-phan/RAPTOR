#include <algorithm>
#include <memory>

#include "hub_labelling.hpp"
#include "csv_reader.hpp"
#include "gzstream.h"

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

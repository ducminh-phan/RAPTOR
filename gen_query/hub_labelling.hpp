#ifndef HUB_LABELLING_HPP
#define HUB_LABELLING_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using Node = uint32_t;
using Distance = uint32_t;

struct NodeLabel {
    std::vector<Node> hubs;
    std::vector<Distance> distances;

    size_t size() { return hubs.size(); }
};

class GraphLabel {
private:
    std::string m_path;
    std::unordered_map<Node, NodeLabel> m_in_labels;
    std::unordered_map<Node, NodeLabel> m_out_labels;

    void parse_hub_files();

    void sort(NodeLabel& node_labels);

    void sort();

public:
    explicit GraphLabel(const std::string& name) : m_path {"../Public-Transit-Data/" + name + "/"} {
        parse_hub_files();
    };
};

#endif // HUB_LABELLING_HPP

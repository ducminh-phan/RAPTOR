#ifndef HUB_LABELLING_HPP
#define HUB_LABELLING_HPP

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using Node = uint32_t;
using Distance = uint32_t;
extern const Distance infty;

struct NodeLabel {
    std::vector<Node> hubs;
    std::vector<Distance> distances;

    const size_t size() const { return hubs.size(); }
};

class GraphLabel {
public:
    using label_t = std::unordered_map<Node, NodeLabel>;
private:
    std::string m_path;
    label_t m_in_labels;
    label_t m_out_labels;

    void parse_hub_files();

    void sort(NodeLabel& node_labels);

    void sort();

public:
    explicit GraphLabel(const std::string& name) : m_path {"../Public-Transit-Data/" + name + "/"} {
        parse_hub_files();
    };

    const Distance shortest_path_length(const Node& u, const Node& v) const;

    const std::vector<std::pair<Distance, Node>> single_source_shortest_path_length(const Node& source) const;

    const label_t& in_labels() const { return m_in_labels; }

    const std::string& path() const { return m_path; }
};

#endif // HUB_LABELLING_HPP

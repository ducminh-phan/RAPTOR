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
using route_id_t = uint16_t;
extern const Distance infty;

struct NodeLabel {
    std::vector<Node> hubs;
    std::vector<Distance> distances;

    size_t size() const { return hubs.size(); }
};

class GraphLabel {
public:
    using label_t = std::unordered_map<Node, NodeLabel>;
private:
    std::string _path;

    void parse_hub_files();

    void parse_weights();

    void sort(NodeLabel& node_labels);

    void sort();

public:
    label_t in_labels;
    label_t out_labels;

    std::vector<Node> stops;
    std::vector<size_t> weights;
    std::unordered_map<Node, size_t> stop_to_weight;

    explicit GraphLabel(const std::string& name) : _path {"../Public-Transit-Data/" + name + "/"} {
        parse_hub_files();

        parse_weights();
    };

    Distance shortest_path_length(const Node& u, const Node& v) const;

    const std::vector<std::pair<Distance, Node>> single_source_shortest_path_length(const Node& source) const;

    const std::vector<Node> sssp_sorted_stops(const Node& source) const;

    const std::string& path() const { return _path; }

    size_t compute_rank(const Node& source, const Node& target) const;
};

#endif // HUB_LABELLING_HPP

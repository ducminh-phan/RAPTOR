#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_set>

#include "hub_labelling.hpp"
#include "rand_utils.hpp"

struct Query {
    Node source;
    Node target;
    size_t time;

    Query(const Node& s, const Node& t, const size_t& t_) : source {s}, target {t}, time {t_} {};
};

using queries_t = std::map<size_t, std::vector<Query>>;

queries_t gen_query(const GraphLabel& gr_label) {
    const size_t max_query_size = 1000;
    const size_t min_rank = 4;
    size_t max_rank = min_rank;
    size_t inertia = 0;
    const size_t max_inertia = 1000;
    std::unordered_set<Node> used_sources;

    queries_t queries;

    size_t count = 0;
    while (true) {
        // Choose a random stop in the graph with given weights
        Node source;

        // We need to select an unused source stop
        do {
            source = weighted_rand_element(gr_label.stops, gr_label.weights);
        } while (used_sources.count(source));

        used_sources.emplace(source);
        bool added = false;

        auto shortest_path_lengths = gr_label.sssp_sorted_stops(source);

        std::ofstream log {"../log"};
        for (const auto& elem: shortest_path_lengths) {
            log << elem << '\n';
        }

        auto current_rank = static_cast<size_t>(std::floor(std::log2(shortest_path_lengths.size())));
        max_rank = std::max(max_rank, current_rank);

        for (size_t rank = min_rank; rank <= current_rank; ++rank) {
            // Choose a random node so that its index is between 2^r and 2^(r+1)
            auto first = shortest_path_lengths.begin() + (1u << rank);
            auto last = shortest_path_lengths.begin() +
                        std::min(static_cast<size_t>(2u << rank), shortest_path_lengths.size());

            auto target = weighted_rand_element<Node>(first, last, gr_label.stop_to_weight);

            auto time = rand_int<size_t>(0, 86400);

            if (queries[rank].size() < max_query_size) {
                queries[rank].emplace_back(source, target, time);
                added = true;
                ++count;
            }
        }

        std::cout << count << " queries added" << std::endl;

        // Stop the process if we cannot generate new queries after max_inertia iterations
        if (added) {
            inertia = 0;
        } else {
            ++inertia;
        }
        if (inertia >= max_inertia) {
            break;
        }

        // Stop the process if we have enough queries
        bool enough_queries = true;
        for (size_t rank = min_rank; rank < max_rank; ++rank) {
            if (queries[rank].size() < max_query_size) {
                enough_queries = false;
                break;
            }
        }
        if (enough_queries) {
            break;
        }
    }

    return queries;
};

void write_queries(const queries_t& queries, const std::string& file_path) {
    std::ofstream file {file_path};

    file << "rank,source,target,time\n";

    for (const auto& kv: queries) {
        auto rank = kv.first;
        auto rank_queries = kv.second;

        for (const auto& query: rank_queries) {
            file << rank << ',' << query.source << ',' << query.target << ',' << query.time << '\n';
        }
    }
}

int main(int argc, char* argv[]) {
    const GraphLabel gr_label {argv[1]};

    auto queries = gen_query(gr_label);

    write_queries(queries, gr_label.path() + "queries.csv");

    return 0;
}

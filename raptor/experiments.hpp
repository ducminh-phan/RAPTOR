#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP

#include <utility> // std::move
#include <vector>

#include "data_structure.hpp"


struct Query {
    uint16_t rank;
    node_id_t source_id;
    node_id_t target_id;
    Time dep;

    Query(uint16_t r, node_id_t s, node_id_t t, Time::value_type d) :
            rank {r}, source_id {s}, target_id {t}, dep {d} {};
};


using Queries = std::vector<Query>;


struct Result {
    uint16_t rank;
    double running_time;
    std::vector<Time> arrival_times;

    Result(uint16_t r, double rt, std::vector<Time> a) : rank {r}, running_time {rt}, arrival_times {std::move(a)} {};
};


using Results = std::vector<Result>;


void write_results(const Results& results, const std::string& name, const std::string& algo);


class Experiment {
private:
    const Timetable* const m_timetable;
    const Queries m_queries;

    Queries read_queries();

public:
    explicit Experiment(const Timetable* timetable) :
            m_timetable {timetable}, m_queries {read_queries()} {}

    void run(const std::string& algo, const std::string& type) const;
};

#endif // EXPERIMENTS_HPP

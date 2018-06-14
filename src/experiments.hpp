#ifndef EXPERIMENTS_HPP
#define EXPERIMENTS_HPP

#include <vector>

#include "data_structure.hpp"


struct Query {
    uint16_t rank;
    stop_id_t source_id;
    stop_id_t target_id;
    _time_t dep;

    Query(uint16_t r, stop_id_t s, stop_id_t t, _time_t::value_type d) :
            rank {r}, source_id {s}, target_id {t}, dep {d} {};
};


using Queries = std::vector<Query>;


struct Result {
    uint16_t rank;
    double running_time;

    Result(uint16_t r, double rt) : rank {r}, running_time {rt} {};
};


using Results = std::vector<Result>;


void write_results(const Results& results, const std::string& name);


class Experiment {
private:
    const Timetable* const m_timetable;
    const Queries m_queries;

    Queries read_queries();

public:
    explicit Experiment(const Timetable* timetable) :
            m_timetable {timetable}, m_queries {read_queries()} {}

    void run() const;
};

#endif // EXPERIMENTS_HPP

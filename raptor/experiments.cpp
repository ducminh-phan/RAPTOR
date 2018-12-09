#include <iomanip>
#include <fstream>

#include "experiments.hpp"
#include "raptor.hpp"
#include "csv.h"
#include "gzstream.h"


void write_results(const Results& results) {
    std::string algo_str = use_hl ? "HLR" : "R";

    std::ofstream running_time_file {"../" + name + "_" + algo_str + "_running_time.csv"};
    std::ofstream arrival_times_file {"../" + name + "_" + algo_str + "_arrival_times.csv"};

    running_time_file << "running_time\n";
    arrival_times_file << "arrival_times\n";

    running_time_file << std::fixed << std::setprecision(4);

    for (const auto& result: results) {
        running_time_file << result.running_time << '\n';

        bool first = true;
        for (const auto& elem: result.arrival_times) {
            if (first) {
                first = false;
            } else {
                arrival_times_file << ',';
            }

            arrival_times_file << elem;
        }

        arrival_times_file << '\n';
    }
}


Queries Experiment::read_queries() {
    Queries queries;
    std::string rank_str = ranked ? "rank_" : "";
    auto queries_file = read_dataset_file<std::ifstream>(m_timetable->path + rank_str + "queries.csv");

    igzstream queries_file_stream {(m_timetable->path + rank_str + "queries.csv").c_str()};
    io::CSVReader<4> queries_file_reader {"queries.csv", queries_file_stream};
    queries_file_reader.read_header(io::ignore_no_column, "rank", "source", "target", "time");

    uint16_t r;
    node_id_t s, t;
    Time::value_type d;

    while (queries_file_reader.read_row(r, s, t, d)) {
        queries.emplace_back(r, s, t, d);
    }

    return queries;
}


void Experiment::run() const {
    Results res;
    Raptor raptor {m_timetable};

    res.resize(m_queries.size());
    for (size_t i = 0; i < m_queries.size(); ++i) {
        auto query = m_queries[i];

        std::vector<Time> arrival_times;
        raptor.init();
        Timer timer;

        arrival_times = raptor.query(query.source_id, query.target_id, query.dep);

        double running_time = timer.elapsed();

        raptor.clear();
        res[i] = {query.rank, running_time, arrival_times};

        std::cout << i << std::endl;
    }

    write_results(res);

    Profiler::report();
}

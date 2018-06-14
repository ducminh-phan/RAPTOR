#include <fstream>

#include "experiments.hpp"
#include "raptor.hpp"
#include "csv_reader.hpp"


void write_results(const Results& results, const std::string& name) {
    std::ofstream results_file {"../" + name + ".csv"};

    results_file << "rank,running_time\n";

    for (const auto& result: results) {
        results_file << result.rank << ',' << result.running_time << '\n';
    }
}


Queries Experiment::read_queries() {
    Queries queries;
    auto queries_file = read_dataset_file<std::ifstream>(m_timetable->path() + "queries.csv");

    for (CSVIterator<uint32_t> iter {queries_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto r = static_cast<uint16_t>((*iter)[0]);
        auto s = static_cast<stop_id_t>((*iter)[1]);
        auto t = static_cast<stop_id_t>((*iter)[2]);
        auto d = static_cast<_time_t::value_type>((*iter)[3]);

        queries.emplace_back(r, s, t, d);
    }

    return queries;
}


void Experiment::run() const {
    Results res;

    for (const auto& query: m_queries) {
        Raptor raptor {m_timetable, query.source_id, query.target_id, query.dep};

        Timer timer;

        raptor.run();

        double running_time = timer.elapsed();

        res.emplace_back(query.rank, running_time);
    }

    write_results(res, m_timetable->name());
}

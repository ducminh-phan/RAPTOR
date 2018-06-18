#include <fstream>

#include "experiments.hpp"
#include "raptor.hpp"
#include "csv_reader.hpp"


void write_results(const Results& results, const std::string& name) {
    std::ofstream running_time_file {"../" + name + "_running_time.csv"};
    std::ofstream arrival_times_file {"../" + name + "_arrival_times.csv"};

    running_time_file << "running_time\n";
    arrival_times_file << "arrival_times\n";

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
    auto queries_file = read_dataset_file<std::ifstream>(m_timetable->path() + "queries.csv");

    for (CSVIterator<uint32_t> iter {queries_file.get()}; iter != CSVIterator<uint32_t>(); ++iter) {
        auto r = static_cast<uint16_t>((*iter)[0]);
        auto s = static_cast<stop_id_t>((*iter)[1]);
        auto t = static_cast<stop_id_t>((*iter)[2]);
        auto d = static_cast<Time::value_type>((*iter)[3]);

        queries.emplace_back(r, s, t, d);
    }

    return queries;
}


void Experiment::run() const {
    Results res;

    for (const auto& query: m_queries) {
        Raptor raptor {m_timetable, query.source_id, query.target_id, query.dep};

        Timer timer;

        auto arrival_times = raptor.run();

        double running_time = timer.elapsed();

        res.emplace_back(query.rank, running_time, arrival_times);
    }

    write_results(res, m_timetable->name());
}
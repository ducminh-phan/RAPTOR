#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <utility>
#include "csv_reader.hpp"
#include "data_structure.hpp"

class Parser {
private:
    std::string city_name;

public:
    explicit Parser(std::string name) : city_name{std::move(name)} {};
    Data parse_data();
};

#endif // PARSER_HPP

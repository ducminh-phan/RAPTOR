#ifndef BAG_HPP
#define BAG_HPP

#include <iostream>
#include <set>

#include "data_structure.hpp"

class Label {
private:
    _time_t _arrival_time;
    _time_t _walking_time;
public:
    Label(_time_t t, _time_t w) : _arrival_time {t}, _walking_time {w} {}

    friend bool operator<(const Label& label1, const Label& label2) {
        return (label1._arrival_time < label2._arrival_time);
    }

    bool dominates(const Label& other) const {
        return ((this->_arrival_time <= other._arrival_time) &&
                (this->_walking_time < other._walking_time)) ||
               ((this->_arrival_time < other._arrival_time) &&
                (this->_walking_time <= other._walking_time));
    }

    friend std::ostream& operator<<(std::ostream& out, const Label& label) {
        out << label._arrival_time << " " << label._walking_time;
        return out;
    }
};

class Bag {
private:
    std::set<Label> _labels;
public:
    void insert(const Label& in_label);

    void insert(const _time_t& t, const _time_t& w);

    std::set<Label> labels() { return _labels; };
};

#endif // BAG_HPP

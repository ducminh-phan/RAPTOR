#include "bag.hpp"

void Bag::insert(const Label& in_label) {
    std::vector<Label> dominated_labels;
    bool is_dominated = false;

    for (const auto& label: _labels) {
        if (in_label.dominates(label)) {
            dominated_labels.push_back(label);
        }

        if (label.dominates(in_label)) {
            is_dominated = true;
        }
    }

    // We use std::set to represent the set of labels, which is ordered by the travel time,
    // thus we need to erase the dominated labels BEFORE inserting the new label,
    // otherwise, the new label will not be inserted if the travel times are the same.
    // We could neglect this order of removing and inserting by adding an additional case in
    // operator<, which is (l1.a == l2.a) && (l1.w < l2.w).
    // But in this case, l1 dominates l2, and we would remove l2 from the bag anyway.
    // So just keep things simple and remove the dominated labels first.

    for (const auto& label: dominated_labels) {
        _labels.erase(label);
    }

    if (!is_dominated) {
        _labels.insert(in_label);
    }
}

void Bag::insert(const _time_t& t, const _time_t& w) {
    Label in_label {t, w};
    this->insert(in_label);
}

void Bag::merge(const Bag& other) {
    for (const auto& label: other._labels) {
        this->insert(label);
    }
}

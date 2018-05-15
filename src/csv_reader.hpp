#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <iterator>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

template<class T>
class CSVRow {
private:
    std::vector<T> m_data;

public:
    T const& operator[](std::size_t index) const {
        return m_data[index];
    }

    std::size_t size() const {
        return m_data.size();
    }

    void readNextRow(std::istream& in) {
        std::string line;
        std::getline(in, line);

        std::stringstream line_stream {line};
        std::string cell;

        m_data.clear();
        while (std::getline(line_stream, cell, ',')) {
            std::stringstream cell_stream {cell};
            T val;

            cell_stream >> val;
            m_data.push_back(val);
        }

        // This checks for a trailing comma with no data after it.
        if (!line_stream && cell.empty()) {
            // If there was a trailing comma then add an empty element.
            m_data.push_back(T());
        }
    };

    friend std::istream& operator>>(std::istream& in, CSVRow<T>& data) {
        data.readNextRow(in);
        return in;
    };

    friend std::ostream& operator<<(std::ostream& out, const CSVRow<T>& data) {
        for (const auto& i : data.m_data) {
            out << i << " ";
        }

        return out;
    };
};


template<class T>
class CSVIterator {
private:
    bool m_header;
    std::istream* m_str;
    CSVRow<T> m_row;

public:
    explicit CSVIterator(std::istream* str_ptr, bool header = true) :
            m_str {(*str_ptr).good() ? str_ptr : nullptr}, m_header {header} {
        ++(*this);

        // skip the header
        if (m_header) ++(*this);
    };

    CSVIterator() : m_str {nullptr}, m_header {true} {};

    // Prefix increment
    CSVIterator<T>& operator++() {
        if (m_str) {
            if (!((*m_str) >> m_row)) {
                m_str = nullptr;
            }
        }

        return *this;
    }

    // Postfix increment
    const CSVIterator<T> operator++(int) {
        CSVIterator<T> tmp(*this);
        ++(*this);
        return tmp;
    }

    CSVRow<T> const& operator*() const { return m_row; }

    CSVRow<T> const* operator->() const { return &m_row; }

    bool operator==(CSVIterator<T> const& rhs) {
        return ((this == &rhs) || ((this->m_str == nullptr) && (rhs.m_str == nullptr)));
    }

    bool operator!=(CSVIterator<T> const& rhs) { return !((*this) == rhs); }
};


#endif // CSV_READER_HPP

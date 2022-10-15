#pragma once

#include <iostream>
#include <string>
#include <vector>

template<class Iterator>
class IteratorRange {
private:
    Iterator begin_, end_;
    size_t size_;
public:
    IteratorRange(Iterator b, Iterator e) : begin_(b), end_(e), size_(distance(b, e)) {}

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return size_;
    }
    friend std::ostream& operator<<(std::ostream& out, const IteratorRange& range)
    {
        auto it = range.begin();
        while (it != range.end())
        {
            out << *it;
            it++;
        }
        return out;
    }
};


template<class Iterator>
class Paginator {
private:
    size_t size_;
    std::vector<IteratorRange<Iterator>> pages_;
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        auto d = distance(begin, end);
        size_ = d / page_size + (d % page_size != 0 ? 1 : 0);
        for (int i = 0; i < size_; ++i) {
            auto begin_it = next(begin, page_size * i);
            auto end_it = (i == size_ - 1 ? end : next(begin_it, page_size));
            pages_.push_back({ begin_it, end_it });
        }
    }

    size_t size() const {
        return size_;
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

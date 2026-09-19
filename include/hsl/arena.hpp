#pragma once

#include <cstddef>
#include <deque>
#include <stdexcept>
#include <utility>

namespace hsl {

template <typename T>
class Arena {
public:
    using Handle = std::size_t;

    template <typename... Arguments>
    Handle create(Arguments&&... arguments) {
        values_.emplace_back(std::forward<Arguments>(arguments)...);
        return values_.size() - 1;
    }

    T& get(Handle handle) {
        if (handle >= values_.size()) throw std::out_of_range("invalid arena handle");
        return values_[handle];
    }

    const T& get(Handle handle) const {
        if (handle >= values_.size()) throw std::out_of_range("invalid arena handle");
        return values_[handle];
    }

    std::size_t size() const noexcept { return values_.size(); }
    bool empty() const noexcept { return values_.empty(); }
    void clear() noexcept { values_.clear(); }

private:
    std::deque<T> values_;
};

}

#pragma once

#include <vector>
#include <optional>
#include <algorithm>

template <typename T>
class Pool
{
public:
    size_t size() const noexcept
    {
        return storage.size();
    }

    std::optional<T> unpool()
    {
        auto result = std::optional<T>();
        if (size() > 0)
        {
            result = storage.back();
            storage.pop_back();
        }
        return result;
    }

    void pool(const T& value)
    {
        storage.push_back(value);
    }

    void pool(T&& value)
    {
        storage.push_back(std::move(value));
    }

    void pool(const std::vector<T>& values)
    {
        std::for_each(std::begin(values), std::end(values), [&storage](auto value){ storage.push_back(value); });
    }
private:
    std::vector<T> storage;
};
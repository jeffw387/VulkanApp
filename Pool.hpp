#pragma once

#include <vector>
#include <optional>
#include <algorithm>

template <typename T>
class Pool
{
public:
    size_t size()
    {
        return storage.size();
    }

    std::optional<T> unpool()
    {
        if (size() > 0)
        {
            auto result = std::make_optional<T>(storage.back());
            storage.pop_back();
            return result;
        }
        return std::make_optional<T>();
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
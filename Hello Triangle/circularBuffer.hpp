#pragma once
#include <array>

auto modulo(int dividend, unsigned int divisor)
{
    if (dividend < 0)
    {
        return (dividend % divisor) + divisor;
    }
    else
    {
        return dividend % divisor;
    }
}

template <typename T, unsigned N>
class CircularBuffer
{
    std::array<T, N> storage;

public:
    T& operator[] (int index)
    {
        return storage[modulo(index, N)];
    }

    auto begin()
    {
        return storage.begin();
    }

    auto end()
    {
        return storage.end();
    }

    auto size()
    {
        return storage.size();
    }
};
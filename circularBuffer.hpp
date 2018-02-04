#pragma once
#include <array>

int modulo(int dividend, unsigned int divisor)
{
    while (dividend < 0)
    {
        dividend += divisor;
    }
    return dividend % divisor;
}

template <typename T, unsigned N>
class CircularBuffer
{
    std::array<T, N> storage;

public:
    T& operator[] (int index)
    {
        auto indexMod = modulo(index, N);
        return storage[indexMod];
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
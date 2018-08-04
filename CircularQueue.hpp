#pragma once
#include <array>
#include <functional>
#include <mutex>
#include <optional>

template <typename T, size_t S>
class CircularQueue {
  std::array<T, S> storage;
  size_t firstID = 0;
  size_t endID = 0;
  size_t count = 0;
  mutable std::mutex storageMutex;

 public:
  // returns a const reference to the first T if it exists
  std::optional<std::reference_wrapper<const T>> readFirst() const {
    auto result = std::optional<std::reference_wrapper<const T>>();
    if (count == 0) {
      return result;
    }
    std::lock_guard<std::mutex> lock(storageMutex);
    result = std::cref(storage[firstID]);
    return result;
  }

  // removes and returns the first T by value if it exists
  std::optional<T> popFirst() {
    auto first = readFirst();
    std::optional<T> result;
    std::lock_guard<std::mutex> lock(storageMutex);
    if (!first.has_value()) return result;
    result = first.value();
    auto newFirst = (++firstID) % S;
    count--;
    firstID = newFirst;
    return result;
  }
  template <typename PredicateType>
  std::optional<T> popFirstIf(PredicateType p) {
    auto first = readFirst();
    std::optional<T> result;
    std::lock_guard<std::mutex> lock(storageMutex);
    if (!first.has_value()) return result;
    auto firstValue = first.value();
    if (p(firstValue)) {
      result = first.value();
      auto newFirst = (++firstID) % S;
      count--;
      firstID = newFirst;
    }
    return result;
  }

  // pushes the given T to the end of the queue if space is available, returns
  // false otherwise
  bool pushLast(T newValue) {
    std::lock_guard<std::mutex> lock(storageMutex);
    if (!(count < S)) return false;
    storage[endID] = newValue;
    count++;
    auto newEnd = (++endID) % S;
    endID = newEnd;
    return true;
  }

  size_t size() { return count; }

  size_t capacity() { return S; }
};
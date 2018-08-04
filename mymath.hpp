#pragma once

#include <algorithm>

namespace helper {
inline size_t roundUp(size_t numToRound, size_t multiple) {
  if (multiple == 0) return numToRound;

  size_t remainder = numToRound % multiple;
  if (remainder == 0) return numToRound;

  return numToRound + multiple - remainder;
}

const char digit_pairs[201] = {
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899"};

static const int BUFFER_SIZE = 11;

inline std::string itostr(int val) {
  char buf[BUFFER_SIZE];
  char *it = &buf[BUFFER_SIZE - 2];

  if (val >= 0) {
    int div = val / 100;
    while (div) {
      memcpy(it, &digit_pairs[2 * (val - div * 100)], 2);
      val = div;
      it -= 2;
      div = val / 100;
    }
    memcpy(it, &digit_pairs[2 * val], 2);
    if (val < 10) it++;
  } else {
    int div = val / 100;
    while (div) {
      memcpy(it, &digit_pairs[-2 * (val - div * 100)], 2);
      val = div;
      it -= 2;
      div = val / 100;
    }
    memcpy(it, &digit_pairs[-2 * val], 2);
    if (val <= -10) it--;
    *it = '-';
  }

  return std::string(it, &buf[BUFFER_SIZE] - it);
}

inline std::string uitostr(unsigned int val) {
  char buf[BUFFER_SIZE];
  char *it = (char *)&buf[BUFFER_SIZE - 2];

  int div = val / 100;
  while (div) {
    memcpy(it, &digit_pairs[2 * (val - div * 100)], 2);
    val = div;
    it -= 2;
    div = val / 100;
  }
  memcpy(it, &digit_pairs[2 * val], 2);
  if (val < 10) it++;

  return std::string((char *)it, (char *)&buf[BUFFER_SIZE] - (char *)it);
}

template <typename T>
T Normalize(T value, T min, T max) {
  T newValue = {};
  auto clamped = std::clamp(newValue, min, max);
  auto shifted = clamped - min;
  auto maxShifted = max - min;
  return shifted / maxShifted;
}
}  // namespace helper
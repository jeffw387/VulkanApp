#pragma once
#include <vector>
#include <algorithm>

template <typename T, typename Predicate>
std::vector<T> get_matches(const std::vector<T>& container, Predicate&& p)
{
	std::vector<T> matches;
	std::copy_if(gpuQueueProps.begin(), gpuQueueProps.end(), std::back_inserter(matches), [&](T t) {
		return p(t);
	});
	return matches;
}
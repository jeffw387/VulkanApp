#pragma once
#include <map>

template <typename KeyType, typename ValueType, typename MapType = std::map<KeyType, ValueType>>
std::optional<std::reference_wrapper<ValueType>> getMapValue(MapType map, KeyType key)
{
	std::optional<std::reference_wrapper<ValueType>> result;
	auto search_pair_iterator = map.find(key);
	if (search_pair_iterator == map.end())
	{
		return result;
	}
	ValueType& value = search_pair_iterator->second;
	result = value;
	return result;
}
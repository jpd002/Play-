#pragma once

#include <string>
#include <sstream>

template <typename StringType>
static StringType TimeToString(double time)
{
	StringType separator;
	separator += ':';
	unsigned int secs = static_cast<unsigned int>(time) % 60;
	unsigned int mins = static_cast<unsigned int>(time) / 60;
	std::basic_stringstream<typename StringType::value_type> result;
	result << mins << separator;
	result.width(2);
	result.fill('0');
	result << secs;
	return result.str();
}

#ifndef _TIMETOSTRING_H_
#define _TIMETOSTRING_H_

#include <string>

template <typename StringType>
static StringType TimeToString(double time)
{
//    StringType result, separator;
    StringType separator;
    separator += ':';
    unsigned int secs = static_cast<unsigned int>(time) % 60;
    unsigned int mins = static_cast<unsigned int>(time) / 60;
    std::basic_stringstream<StringType::value_type> result;
    result << mins << separator;
    result.width(2);
    result.fill('0');
    result << secs;
//    result = 
//        boost::lexical_cast<StringType>(mins) + 
//        separator + 
//        boost::lexical_cast<StringType>(secs);
    return result.str();
}

#endif

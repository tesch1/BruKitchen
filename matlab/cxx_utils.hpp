// -*-  Mode: C++; c-basic-offset: 4 -*-
/**
 * c++ utility functions
 *
 * (c)2016 Michael Tesch. tesch1@gmail.com
 */
#ifndef CXX_UTILS_HPP
#define CXX_UTILS_HPP

#include <algorithm>
#include <string>

struct MatchSep {
    bool operator()(char ch) const { return ch == '/'; }
};

inline
std::string cxx_basename(std::string const & pathname)
{
    return std::string(std::find_if(pathname.rbegin(), pathname.rend(), MatchSep()).base(), pathname.end());
}

inline
std::string cxx_basename(const char * cpathname)
{
    std::string pathname(cpathname);
    return cxx_basename(pathname);
}

#endif // CXX_UTILS_HPP

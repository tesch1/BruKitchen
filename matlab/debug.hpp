// -*-  Mode: C++; c-basic-offset: 4 -*-
/**
 * Debugging stuff
 *
 * (c)2016 Michael Tesch. tesch1@gmail.com
 */
#pragma once
#include <cmath>
#include <map>
#ifdef CIO
#include "cio.hpp"
#else
#include <string>
using std::string;
#endif

enum DebugLevel { LEVEL_ALL, LEVEL_ERROR, LEVEL_WARN, LEVEL_INFO, LEVEL_INFO2 };
extern DebugLevel g_debug_level;
void SetDebugLevel(DebugLevel);
void DebugFunc(DebugLevel level, string str, string location);
bool GetNextErrorMsg(string & msg, DebugLevel & level);

//! aborts on failing stod conversions
double x_stod(const string & str);

#if !defined(__EMSCRIPTEN__) && !defined(WIN32)
using cstr = const char *;

constexpr cstr past_last_slash(cstr str, cstr last_slash)
{
    return
        *str == '\0' ? last_slash :
        *str == '/'  ? past_last_slash(str + 1, str + 1) :
                       past_last_slash(str + 1, last_slash);
}

constexpr cstr past_last_slash(cstr str) 
{ 
    return past_last_slash(str, str);
}
#define __SHORT_FILE__ ({constexpr cstr sf__ {past_last_slash(__FILE__)}; sf__;})
#elif defined(WIN32)
#define __SHORT_FILE__ __FILE__
#else
#define __SHORT_FILE__ ""
#endif

#define DebugMsg(level, message)                                \
    do {                                                        \
        if ((level) <= g_debug_level) {                         \
            stringstream SSTR, SLOC;                            \
            SSTR << message;                                    \
            SLOC << __SHORT_FILE__ << ":" << __LINE__;          \
            DebugFunc(level, SSTR.str(), SLOC.str());           \
        }                                                       \
    } while (0)

#define MSG(msg)   DebugMsg(LEVEL_ALL, msg)
#define ERROR(msg) DebugMsg(LEVEL_ERROR, msg)
#define WARN(msg)  DebugMsg(LEVEL_WARN, msg)
#define INFO(msg)  DebugMsg(LEVEL_INFO, msg)
#ifdef NDEBUG
#define INFO2(msg) do {} while (0)
#else
#define INFO2(msg) DebugMsg(LEVEL_INFO2, msg)
#endif

#define THROWERROR(msg) do {                                    \
        stringstream SSTR;                                      \
        /*SSTR << __SHORT_FILE__ << ":" << __LINE__ << " ";*/   \
        SSTR << msg;                                            \
        throw std::runtime_error(SSTR.str());                   \
    } while (0)

#ifndef NDEBUG
std::map<string, uint64_t> & g_debug_events();
//! count debug events
#define DEBUG_EVENT(EVENT_NAME) g_debug_events()[EVENT_NAME]++
//#define DEBUG_EVENT(EVENT_NAME) do {} while (0)
#else
#define DEBUG_EVENT(EVENT_NAME) do {} while (0)
#endif

//! utility function for putting maps of things to streams
#include <map>
template <class KEY, class VAL>
ostream & operator<<(ostream & os, const std::map<KEY, VAL> & m)
{
    os << "{";
    for (auto & it : m)
        os << it.first << ":" << it.second << ", ";
    os << "}";
    return os;
}

//! utility function for putting lists of things to streams
#include <list>
template <typename T>
ostream & operator<<(ostream & out, const std::list<T> & l)
{
    bool yet = false;
    for (auto & it : l) {
        if (yet)
            out << ",";
        out << it;
        yet = true;
    }
    return out;
}

//! utility function for putting vectors of things to streams
#include <vector>
template <typename T>
ostream & operator<<(ostream & out, const std::vector<T> & l)
{
    bool yet = false;
    for (auto & it : l) {
        if (yet)
            out << ",";
        out << it;
        yet = true;
    }
    return out;
}


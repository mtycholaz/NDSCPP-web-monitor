#ifndef GLOBAL_H
#define GLOBAL_H

#include <microhttpd.h>
#include <optional>
#include <thread>
#include <pthread.h>
#include <string>
#include <chrono>
#include <iostream>
//#include <format>
#include <iomanip>

#include "pixeltypes.h"
#include "utilities.h"

using namespace std;
using namespace chrono;

using string = string;
template<class T>
using optional = optional<T>;
using Clock = chrono::system_clock;
using TimePoint = Clock::time_point;

#include "secrets.h"

// Make it possible to disable ignoring cache (including per command-line define),
// to prevent abuse in production scenarios. To be on the safe side, we default to false.
// That means this program must be built with ALLOW_IGNORE_CACHE explicitly set to true
// for cached=no to work.
#ifndef ALLOW_IGNORE_CACHE
    #define ALLOW_IGNORE_CACHE false
#endif

#define STR(x) #x

// createTimePoint
//
// Create a TimePoint for a given year, day... second.

inline chrono::system_clock::time_point createTimePoint(int y, unsigned int m, unsigned int d, unsigned int h, unsigned int min, unsigned int sec)
{
    using namespace chrono;

    // Create a year_month_day object using the provided year, month, and day
    year_month_day ymd = year{y}/month{m}/day{d};

    // Create a time of day from hours, minutes, and seconds
    hh_mm_ss time_of_day{hours{h} + minutes{min} + seconds{sec}};

    // Combine the year_month_day and time_of_day to create a sys_time<seconds>
    sys_time<seconds> tp = sys_days{ymd} + time_of_day.to_duration();

    // Return the time_point
    return chrono::system_clock::time_point{tp};
}

static inline string to_date_string(const chrono::system_clock::time_point& tp)
{
        using namespace chrono;
        auto dp = floor<days>(tp); // Truncate to the nearest day
        year_month_day ymd = year_month_day{dp}; // Convert to year/month/day

        ostringstream oss;
        oss << int(ymd.year()) << '-'
            << setw(2) << setfill('0') << unsigned(ymd.month()) << '-'
            << setw(2) << setfill('0') << unsigned(ymd.day());
        return oss.str();
    }

static inline string to_date_time_string(const chrono::system_clock::time_point& tp)
{
    auto dp = floor<days>(tp); // Truncate to the nearest day
    year_month_day ymd = year_month_day{dp}; // Convert to year/month/day

    // Inline expansion of make_time
    auto tod = tp - dp;
    auto hp = duration_cast<hours>(tod);
    tod -= hp;
    auto mp = duration_cast<minutes>(tod);
    tod -= mp;
    auto sp = duration_cast<seconds>(tod);

    ostringstream oss;
    oss << int(ymd.year()) << '-'
        << setw(2) << setfill('0') << unsigned(ymd.month()) << '-'
        << setw(2) << setfill('0') << unsigned(ymd.day()) << ' '
        << setw(2) << setfill('0') << hp.count() << ':'
        << setw(2) << setfill('0') << mp.count() << ':'
        << setw(2) << setfill('0') << sp.count();
    return oss.str();
}

// arraysize
//
// Number of elements in a static array

#define arraysize(x) (sizeof(x)/sizeof(x[0]))


inline string str_snprintf(const char *fmt, size_t len, ...)
{
    string str(len, '\0');  // Create a string filled with null characters of 'len' length
    va_list args;

    va_start(args, len);
    int out_length = vsnprintf(&str[0], len + 1, fmt, args); // Write into the string's buffer directly
    va_end(args);

    // Resize the string to the actual output length, which vsnprintf returns
    if (out_length >= 0)
    {
        // vsnprintf returns the number of characters that would have been written if n had been sufficiently large
        // not counting the terminating null character.
        if (static_cast<size_t>(out_length) > len)
        {
            // The given length was not sufficient, resize and try again
            str.resize(out_length);  // Make sure the buffer can hold all data
            va_start(args, len);
            vsnprintf(&str[0], out_length + 1, fmt, args);  // Write again with the correct size
            va_end(args);
        }
        else
        {
            // The output fit into the buffer, resize to the actual length used
            str.resize(out_length);
        }
    }
    else
    {
        // If vsnprintf returns an error, clear the string
        str.clear();
    }

    return str;
}

//
// Helpful global functions
//

inline double millis()
{
   return (double)clock() / CLOCKS_PER_SEC * 1000;
}

inline void delay(int milliseconds)
{
    this_thread::sleep_for(chrono::milliseconds(milliseconds));
}

#endif

/*
    Copyright (c) 2023, Thierry Tremblay
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <c++/ctime>
#include <cassert>

#if UNITTEST
namespace std_test
{
#define EXTERN_C
#else
#define EXTERN_C extern "C"
#endif

    static constexpr bool IsLeapYear(int year)
    {
        return (((year % 4) == 0 && (year % 100) != 0) || (year % 400) == 0);
    }

    static constexpr int GetDaysInYear(int year)
    {
        return IsLeapYear(year) ? 366 : 365;
    }

    static constexpr int GetDaysBeforeMonth(int year, int month)
    {
        assert(month >= 0 && month <= 11);
        constexpr int daysBeforeMonth[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
        return daysBeforeMonth[month] + IsLeapYear(year);
    }

    static constexpr int GetDaysInMonth(int year, int month)
    {
        assert(month >= 0 && month <= 11);
        constexpr int daysInMonth[2][12] = {{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
                                            {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
        return daysInMonth[IsLeapYear(year)][month];
    }

    static constexpr inline void Normalize(int& units, int& tens, int base)
    {
        if (units >= base)
        {
            tens += units / base;
            units %= base;
        }
        else if (units < 0)
        {
            tens -= 1 - units / base;
            units = base + units % base;
        }
    }

    static void Normalize(tm& time)
    {
        Normalize(time.tm_sec, time.tm_min, 60);
        Normalize(time.tm_min, time.tm_hour, 60);
        Normalize(time.tm_hour, time.tm_mday, 24);

        while (time.tm_mday <= 0)
        {
            if (--time.tm_mon == -1)
            {
                time.tm_mon = 11;
                time.tm_year -= 1;
            }
            time.tm_mday += GetDaysInMonth(time.tm_year, time.tm_mon);
        }

        while (time.tm_mday > GetDaysInMonth(time.tm_year, time.tm_mon))
        {
            time.tm_mday -= GetDaysInMonth(time.tm_year, time.tm_mon);
            if (++time.tm_mon == 12)
            {
                time.tm_mon = 0;
                time.tm_year += 1;
            }
        }
    }

    EXTERN_C time_t mktime(struct tm* time)
    {
        if (!time)
            return -1;

        Normalize(*time);

        // The Gregorian calendar started on October 15, 1582. To simplify things, we won't accept any date before 1983.
        if (time->tm_year < 1583 - 1900)
            return -1;

        // We set an arbitrary upper limit
        if (time->tm_year > 9999 - 1900)
            return -1;

        // We add 1900 to tm_year to simplify the implementation.
        // We will remove it before returning to the caller.
        time->tm_year += 1900;

        // Compute day in year
        time->tm_yday = GetDaysBeforeMonth(time->tm_year, time->tm_mon) + (time->tm_mday - 1);

        // Compute seconds sinze 1970/01/01 00:00:00
        int daysSinceEpoch = time->tm_yday;

        for (int year = time->tm_year - 1; year >= 1970; --year)
            daysSinceEpoch += GetDaysInYear(year);

        for (int year = time->tm_year; year < 1970; ++year)
            daysSinceEpoch -= GetDaysInYear(year);

        // Compute day in week
        if ((time->tm_wday = (daysSinceEpoch + 4) % 7) < 0)
            time->tm_wday += 7;

        // Since we only deal with UTC, we know DST is not active
        time->tm_isdst = 0;

        // Restore tm_year to the proper range
        time->tm_year -= 1900;

        time_t result = 0;
        result += time->tm_sec;
        result += time->tm_min * 60;
        result += time->tm_hour * (60 * 60); // Compute total seconds
        result += (time_t)daysSinceEpoch * (24 * 60 * 60);

        return result;
    }

#if UNITTEST
} // namespace std_test
#endif

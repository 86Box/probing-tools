/*
 * time.c
 *
 * Copyright (C) 2021 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of the POSIX-UEFI package.
 * @brief Implementing functions which are defined in time.h
 *
 */

#include <uefi.h>

static struct tm __tm;

/* from musl */
uint64_t __year_to_secs(uint64_t year, int *is_leap)
{
    int y, cycles, centuries, leaps, rem;

    if (year-2ULL <= 136) {
        y = year;
        leaps = (y-68)>>2;
        if (!((y-68)&3)) {
            leaps--;
            if (is_leap) *is_leap = 1;
        } else if (is_leap) *is_leap = 0;
        return 31536000*(y-70) + 86400*leaps;
    }

    if (!is_leap) is_leap = &(int){0};
    cycles = (year-100) / 400;
    rem = (year-100) % 400;
    if (rem < 0) {
        cycles--;
        rem += 400;
    }
    if (!rem) {
        *is_leap = 1;
        centuries = 0;
        leaps = 0;
    } else {
        if (rem >= 200) {
            if (rem >= 300) centuries = 3, rem -= 300;
            else centuries = 2, rem -= 200;
        } else {
            if (rem >= 100) centuries = 1, rem -= 100;
            else centuries = 0;
        }
        if (!rem) {
            *is_leap = 0;
            leaps = 0;
        } else {
            leaps = rem / 4U;
            rem %= 4U;
            *is_leap = !rem;
        }
    }

    leaps += 97*cycles + 24*centuries - *is_leap;

    return (year-100) * 31536000LL + leaps * 86400LL + 946684800 + 86400;
}

time_t __mktime_efi(efi_time_t *t)
{
    __tm.tm_year = t->Year + 98;
    __tm.tm_mon = t->Month - 1;
    __tm.tm_mday = t->Day;
    __tm.tm_hour = t->Hour;
    __tm.tm_min = t->Minute;
    __tm.tm_sec = t->Second;
    __tm.tm_isdst = t->Daylight;
    return mktime(&__tm);
}

/**
 * This isn't POSIX, no arguments. Just returns the current time in struct tm
 */
struct tm *localtime (const time_t *__timer)
{
    efi_time_t t = {0};
    (void)__timer;
    ST->RuntimeServices->GetTime(&t, NULL);
    __mktime_efi(&t);
    return &__tm;
}

time_t mktime(const struct tm *tm)
{
    static const int secs_through_month[] = {
        0, 31*86400, 59*86400, 90*86400,
        120*86400, 151*86400, 181*86400, 212*86400,
        243*86400, 273*86400, 304*86400, 334*86400 };
    int is_leap;
    uint64_t year = tm->tm_year, t;
    int month = tm->tm_mon;
    if (month >= 12 || month < 0) {
        int adj = month / 12;
        month %= 12;
        if (month < 0) {
            adj--;
            month += 12;
        }
        year += adj;
    }
    t = __year_to_secs(year, &is_leap);
    t += secs_through_month[month];
    if (is_leap && month >= 2) t += 86400;
    t += 86400LL * (tm->tm_mday-1);
    t += 3600LL * tm->tm_hour;
    t += 60LL * tm->tm_min;
    t += tm->tm_sec;
    return (time_t)t;
}

time_t time(time_t *__timer)
{
    time_t ret;
    efi_time_t t = {0};
    ST->RuntimeServices->GetTime(&t, NULL);
    ret = __mktime_efi(&t);
    if(__timer) *__timer = ret;
    return ret;
}


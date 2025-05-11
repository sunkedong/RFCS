// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/Timestamp.h"

#include <sys/time.h>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

using namespace muduo;

static_assert(sizeof(Timestamp) == sizeof(int64_t),
              "Timestamp should be same size as int64_t");

string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
    snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}

string Timestamp::toFormattedString(bool showMicroseconds) const
{
    char buf[64] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    /*
     *通常是一个整数类型（如 long 或 long long），用来表示自 1970 年 1 月 1 日（协调世界时 UTC）以来的秒数
     *
        #include <stdio.h>
        #include <time.h>

        int main() {
            time_t t;
            time(&t); // 获取当前时间戳
            printf("Current time (time_t): %ld\n", t);
            return 0;
        }
     *
     */
    struct tm tm_time;
    /*
    struct tm {
        int tm_sec;   // 秒，范围：0-59
        int tm_min;   // 分，范围：0-59
        int tm_hour;  // 时，范围：0-23
        int tm_mday;  // 日，范围：1-31
        int tm_mon;   // 月，范围：0-11 (0 = 一月, 11 = 十二月)
        int tm_year;  // 从 1900 年开始的年数
        int tm_wday;  // 星期几，范围：0-6 (0 = 星期日, 6 = 星期六)
        int tm_yday;  // 一年的天数，范围：0-365
        int tm_isdst; // 夏令时标识符，正值表示夏令时，0表示非夏令时，负值表示未知
    };
     */
    gmtime_r(&seconds, &tm_time); //gmtime() non-thread safe -r thread_safe
    /*
     *
     */
    if (showMicroseconds)
    {
        int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
                 microseconds);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    }
    return buf;
}

Timestamp Timestamp::now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    /*
    * struct timeval {
    time_t tv_sec;   // 秒数，表示从1970年1月1日以来的秒数
    suseconds_t tv_usec; // 微秒数，表示当前秒内的微秒数
};

     */
    int64_t seconds = tv.tv_sec;
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

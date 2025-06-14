// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

#include "muduo/base/copyable.h"
#include "muduo/base/Types.h"

#include <boost/operators.hpp>

namespace muduo
{
    ///
    /// Time stamp in UTC, in microseconds resolution. 微秒精度 (Microseconds Resolution)：
    ///
    /// This class is immutable.
    /// It's recommended to pass it by value, since it's passed in register on x64. 在 x64 架构中，传递小的结构体对象（如时间戳）时，通常会使用寄存器来传递，而不是通过栈，这样可以提高性能。因此，建议将该类对象作为值传递，而不是传递引用。
    ///
    class Timestamp : public muduo::copyable,
                      public boost::equality_comparable<Timestamp>, // in C++ 11 just override == is fine
                      public boost::less_than_comparable<Timestamp>
    {
    public:
        ///
        /// Constucts an invalid Timestamp.
        ///
        Timestamp()
            : microSecondsSinceEpoch_(0)
        {
        }

        ///
        /// Constucts a Timestamp at specific time
        ///
        /// @param microSecondsSinceEpoch
        explicit Timestamp(int64_t microSecondsSinceEpochArg)
            : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
        {
        }

        void swap(Timestamp& that)
        {
            std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
        }

        // default copy/assignment/dtor are Okay

        string toString() const;
        string toFormattedString(bool showMicroseconds = true) const;

        bool valid() const { return microSecondsSinceEpoch_ > 0; }

        // for internal usage.
        int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

        time_t secondsSinceEpoch() const
        {
            return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
        }

        ///
        /// Get time of now.
        ///
        static Timestamp now();

        static Timestamp invalid()
        {
            return Timestamp();
        }

        static Timestamp fromUnixTime(time_t t)
        {
            return fromUnixTime(t, 0);
        }

        static Timestamp fromUnixTime(time_t t, int microseconds)
        {
            return Timestamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
        }

        static const int kMicroSecondsPerSecond = 1000 * 1000;

    private:
        int64_t microSecondsSinceEpoch_;
    };

    inline bool operator<(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }

    ///
    /// Gets time difference of two timestamps, result in seconds.
    ///
    /// @param high, low
    /// @return (high-low) in seconds
    /// @c double has 52-bit precision, enough for one-microsecond
    /// resolution for next 100 years.
    inline double timeDifference(Timestamp high, Timestamp low)
    {
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
    }

    ///
    /// Add @c seconds to given timestamp.
    ///
    /// @return timestamp+seconds as Timestamp
    ///
    inline Timestamp addTime(Timestamp timestamp, double seconds)
    {
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
        return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
    }
} // namespace muduo

#endif  // MUDUO_BASE_TIMESTAMP_H

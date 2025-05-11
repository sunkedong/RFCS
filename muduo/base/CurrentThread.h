// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include "muduo/base/Types.h"

namespace muduo // 定义 muduo 命名空间
{
    namespace CurrentThread // 定义 CurrentThread 命名空间，用于存储与当前线程相关的信息
    {
        // internal
        extern __thread int t_cachedTid; // 声明一个线程局部存储变量，用于存储线程ID
        extern __thread char t_tidString[32]; // 声明一个线程局部存储变量，用于存储线程ID的字符串表示
        extern __thread int t_tidStringLength; // 声明一个线程局部存储变量，用于存储线程ID字符串的长度
        extern __thread const char* t_threadName; // 声明一个线程局部存储变量，用于存储线程名称
        void cacheTid(); // 声明一个函数，用于缓存线程ID

        // 获取当前线程ID的函数，若线程ID未缓存，则调用 cacheTid() 缓存线程ID
        inline int tid()
        {
            if (__builtin_expect(t_cachedTid == 0, 0)) // 使用 GCC 内建函数来优化条件判断，检查线程ID是否已缓存
            {
                cacheTid(); // 缓存线程ID
            }
            return t_cachedTid; // 返回缓存的线程ID
        }

        // 返回线程ID字符串的函数，用于日志输出
        inline const char* tidString()
        {
            return t_tidString; // 返回存储线程ID字符串的变量
        }

        // 返回线程ID字符串的长度的函数，用于日志输出
        inline int tidStringLength()
        {
            return t_tidStringLength; // 返回存储线程ID字符串长度的变量
        }

        // 返回线程名称的函数
        inline const char* name()
        {
            return t_threadName; // 返回存储线程名称的变量
        }

        // 判断当前线程是否是主线程
        bool isMainThread();

        // 用于休眠指定时间（微秒），主要用于测试
        void sleepUsec(int64_t usec); // 使用 microsecond 精度的休眠函数

        // 获取当前线程的堆栈跟踪信息
        string stackTrace(bool demangle);
    } // namespace CurrentThread
} // namespace muduo


#endif  // MUDUO_BASE_CURRENTTHREAD_H

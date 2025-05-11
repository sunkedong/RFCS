// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/CurrentThread.h"

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>

namespace muduo // 定义 muduo 命名空间
{
    namespace CurrentThread // 定义 CurrentThread 命名空间，用于当前线程相关信息
    {
        __thread int t_cachedTid = 0; // 声明线程局部存储变量，用于缓存线程ID，初始值为0
        __thread char t_tidString[32]; // 声明线程局部存储变量，用于存储线程ID的字符串表示
        __thread int t_tidStringLength = 6; // 声明线程局部存储变量，用于存储线程ID字符串的长度，初始值为6
        __thread const char* t_threadName = "unknown"; // 声明线程局部存储变量，用于存储线程名称，初始值为"unknown"

        static_assert(std::is_same<int, pid_t>::value, "pid_t should be int"); // 编译时检查，确保 pid_t 类型为 int

        // 获取当前线程的堆栈跟踪信息
        string stackTrace(bool demangle)
        {
            string stack; // 用于存储堆栈跟踪结果的字符串
            const int max_frames = 200; // 最大堆栈帧数
            void* frame[max_frames]; // 存储堆栈帧的指针数组
            int nptrs = ::backtrace(frame, max_frames); // 获取堆栈帧
            char** strings = ::backtrace_symbols(frame, nptrs); // 获取堆栈帧的符号名称
            if (strings)
            {
                size_t len = 256; // 分配空间，用于存储解码后的符号名称
                char* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr; // 如果需要解码符号名称，分配内存
                for (int i = 1; i < nptrs; ++i) // 遍历堆栈帧（从索引1开始，跳过当前函数的堆栈帧）
                {
                    if (demangle)
                    {
                        // 尝试解码符号名称
                        // https://panthema.net/2008/0901-stacktrace-demangled/
                        // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
                        char* left_par = nullptr;
                        char* plus = nullptr;
                        for (char* p = strings[i]; *p; ++p) // 遍历当前堆栈帧字符串
                        {
                            if (*p == '(')
                                left_par = p; // 记录左括号的位置
                            else if (*p == '+')
                                plus = p; // 记录加号的位置
                        }

                        if (left_par && plus)
                        {
                            *plus = '\0'; // 把加号替换为终止符
                            int status = 0;
                            char* ret = abi::__cxa_demangle(left_par + 1, demangled, &len, &status); // 解码符号名称
                            *plus = '+'; // 恢复加号
                            if (status == 0)
                            {
                                demangled = ret; // 如果解码成功，更新 demangled 指针
                                stack.append(strings[i], left_par + 1); // 追加符号前部分
                                stack.append(demangled); // 追加解码后的符号
                                stack.append(plus); // 追加加号后部分
                                stack.push_back('\n'); // 添加换行符
                                continue;
                            }
                        }
                    }
                    // 如果解码失败，回退到原始的符号名称
                    stack.append(strings[i]); // 追加堆栈帧符号
                    stack.push_back('\n'); // 添加换行符
                }
                free(demangled); // 释放分配的内存
                free(strings); // 释放堆栈符号名称数组
            }
            return stack; // 返回堆栈信息
        }
    } // namespace CurrentThread
} // namespace muduo

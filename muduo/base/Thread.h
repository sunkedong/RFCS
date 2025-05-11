// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include "muduo/base/Atomic.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Types.h"

#include <functional>
#include <memory>
#include <pthread.h>

namespace muduo // 定义一个名为muduo的命名空间
{
    class Thread : noncopyable  // 定义一个Thread类，继承自noncopyable，禁止拷贝操作
    {
    public:
        typedef std::function<void ()> ThreadFunc;  // 定义一个类型别名ThreadFunc，表示没有返回值的函数类型

        explicit Thread(ThreadFunc, const string& name = string()); // 构造函数，接受一个ThreadFunc类型的函数和一个名字（默认为空字符串）
        // FIXME: make it movable in C++11 // FIXME注释：需要在C++11中使Thread类可以移动（现在不支持）
        ~Thread();  // 析构函数，负责销毁线程资源

        void start();  // 启动线程，开始执行ThreadFunc
        int join(); // 等待线程执行结束，相当于调用pthread_join

        bool started() const { return started_; }  // 返回线程是否已启动的标志
        // pthread_t pthreadId() const { return pthreadId_; }  // 获取线程的pthread_id（被注释掉了）
        pid_t tid() const { return tid_; }  // 返回线程的ID（系统PID）
        const string& name() const { return name_; }  // 返回线程的名字

        static int numCreated() { return numCreated_.get(); }  // 返回已创建线程的数量

    private:
        void setDefaultName();  // 设置默认的线程名称（私有函数）

        bool started_;  // 线程是否已启动的标志
        bool joined_;  // 线程是否已经加入（即已经结束并且资源回收）
        pthread_t pthreadId_;  // 存储线程的pthread_t类型ID
        pid_t tid_;  // 存储线程的系统PID
        ThreadFunc func_;  // 存储线程要执行的函数
        string name_;  // 存储线程的名字
        CountDownLatch latch_;  // 用于线程同步的倒计时锁（latch）

        static AtomicInt32 numCreated_;  // 静态成员变量，记录已创建的线程数量
    };
} // namespace muduo  // 结束muduo命名空间

#endif  // MUDUO_BASE_THREAD_H

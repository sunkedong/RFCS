// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/Thread.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Exception.h"
#include "muduo/base/Logging.h"

#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

namespace muduo // 定义muduo命名空间
{
    namespace detail // 定义detail命名空间，通常用于实现细节，不暴露给外部
    {
        pid_t gettid() // 获取当前线程的ID
        {
            return static_cast<pid_t>(::syscall(SYS_gettid)); // 调用系统调用获取线程ID
        }

        void afterFork() // 在fork之后调用的函数
        {
            muduo::CurrentThread::t_cachedTid = 0; // 清空缓存的线程ID
            muduo::CurrentThread::t_threadName = "main"; // 设置线程名称为"main"
            CurrentThread::tid(); // 获取当前线程的ID
            // no need to call pthread_atfork(NULL, NULL, &afterFork);  // 说明不需要再次设置fork时的回调
        }

        class ThreadNameInitializer // 线程名称初始化类
        {
        public:
            ThreadNameInitializer() // 构造函数
            {
                muduo::CurrentThread::t_threadName = "main"; // 设置线程名称为"main"
                CurrentThread::tid(); // 获取当前线程的ID
                pthread_atfork(NULL, NULL, &afterFork); // 注册fork时的回调函数
            }
        };

        ThreadNameInitializer init; // 创建并初始化ThreadNameInitializer对象

        struct ThreadData // 线程数据结构，用于存储线程信息和传递给线程的参数
        {
            typedef muduo::Thread::ThreadFunc ThreadFunc; // 类型别名，表示线程的执行函数类型
            ThreadFunc func_; // 线程函数
            string name_; // 线程名称
            pid_t* tid_; // 线程ID
            CountDownLatch* latch_; // 线程完成时使用的倒计时锁

            ThreadData(ThreadFunc func,
                       const string& name,
                       pid_t* tid,
                       CountDownLatch* latch)
                : func_(std::move(func)), // 初始化线程函数
                  name_(name), // 初始化线程名称
                  tid_(tid), // 初始化线程ID指针
                  latch_(latch) // 初始化倒计时锁
            {
            }

            void runInThread() // 线程执行的实际函数
            {
                *tid_ = muduo::CurrentThread::tid(); // 设置线程ID
                tid_ = NULL; // 清空tid_指针，防止被误用
                latch_->countDown(); // 倒计时锁减1
                latch_ = NULL; // 清空latch_

                muduo::CurrentThread::t_threadName = name_.empty() ? "muduoThread" : name_.c_str(); // 设置线程名称
                ::prctl(PR_SET_NAME, muduo::CurrentThread::t_threadName); // 设置线程的实际名称（Linux特有）

                try // 捕获线程执行中的异常
                {
                    func_(); // 执行传入的线程函数
                    muduo::CurrentThread::t_threadName = "finished"; // 设置线程名称为"finished"
                }
                catch (const Exception& ex) // 捕获自定义异常
                {
                    muduo::CurrentThread::t_threadName = "crashed"; // 设置线程名称为"crashed"
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason: %s\n", ex.what());
                    fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
                    abort(); // 程序崩溃，输出异常信息
                }
                catch (const std::exception& ex) // 捕获标准库异常
                {
                    muduo::CurrentThread::t_threadName = "crashed"; // 设置线程名称为"crashed"
                    fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                    fprintf(stderr, "reason: %s\n", ex.what());
                    abort(); // 程序崩溃，输出异常信息
                }
                catch (...) // 捕获其他未知异常
                {
                    muduo::CurrentThread::t_threadName = "crashed"; // 设置线程名称为"crashed"
                    fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
                    throw; // 重新抛出异常
                }
            }
        };

        void* startThread(void* obj) // 启动线程的函数
        {
            ThreadData* data = static_cast<ThreadData*>(obj); // 将传入的对象转换为ThreadData指针
            data->runInThread(); // 执行线程中的任务
            delete data; // 释放ThreadData对象内存
            return NULL; // 返回空指针
        }
    } // namespace detail

    void CurrentThread::cacheTid() // 缓存当前线程ID的函数
    {
        if (t_cachedTid == 0) // 如果缓存的线程ID为空
        {
            t_cachedTid = detail::gettid(); // 获取当前线程的ID
            t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid); // 格式化线程ID为字符串
        }
    }

    bool CurrentThread::isMainThread() // 判断当前线程是否为主线程
    {
        return tid() == ::getpid(); // 如果当前线程ID等于主进程的PID，则是主线程
    }

    void CurrentThread::sleepUsec(int64_t usec) // 线程睡眠指定的微秒数
    {
        struct timespec ts = {0, 0}; // 定义时间结构
        ts.tv_sec = static_cast<time_t>(usec / Timestamp::kMicroSecondsPerSecond); // 计算秒数
        ts.tv_nsec = static_cast<long>(usec % Timestamp::kMicroSecondsPerSecond * 1000); // 计算纳秒数
        ::nanosleep(&ts, NULL); // 调用系统的nanosleep函数进行睡眠
    }

    AtomicInt32 Thread::numCreated_; // 静态成员，记录创建的线程数量

    Thread::Thread(ThreadFunc func, const string& n) // 构造函数，初始化线程的各种状态
        : started_(false), // 线程未启动
          joined_(false), // 线程未加入
          pthreadId_(0), // 线程ID初始化为0
          tid_(0), // 线程ID初始化为0
          func_(std::move(func)), // 存储线程函数
          name_(n), // 存储线程名称
          latch_(1) // 倒计时锁初始化为1
    {
        setDefaultName(); // 设置默认线程名称
    }

    Thread::~Thread() // 析构函数
    {
        if (started_ && !joined_) // 如果线程已启动且未加入
        {
            pthread_detach(pthreadId_); // 分离线程，防止线程退出后不清理资源
        }
    }

    void Thread::setDefaultName() // 设置线程的默认名称
    {
        int num = numCreated_.incrementAndGet(); // 获取已创建线程数并增加
        if (name_.empty()) // 如果线程没有名称
        {
            char buf[32];
            snprintf(buf, sizeof buf, "Thread%d", num); // 设置默认名称为"ThreadX"
            name_ = buf;
        }
    }

    void Thread::start() // 启动线程的函数
    {
        assert(!started_); // 确保线程未启动
        started_ = true; // 设置线程已启动
        // FIXME: move(func_)
        detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_, &latch_); // 创建ThreadData对象，存储线程信息
        if (pthread_create(&pthreadId_, NULL, &detail::startThread, data)) // 创建线程并启动
        {
            started_ = false; // 如果创建失败，设置线程未启动
            delete data; // 删除ThreadData对象
            LOG_SYSFATAL << "Failed in pthread_create"; // 记录错误日志
        }
        else
        {
            latch_.wait(); // 等待线程执行，latch的计数器减为0
            assert(tid_ > 0); // 确保线程ID有效
        }
    }

    int Thread::join() // 等待线程执行完毕
    {
        assert(started_); // 确保线程已启动
        /*
         *
         *
         *
        如果你在 start() 前调用 join()，操作系统甚至可能没有机会调度这个线程执行，
            这时 join() 会一直等待一个从未开始的线程，结果可能会导致死锁或者异常。
            而如果你在 start() 后调用 join()，操作系统会确保目标线程已经开始执行，
                join() 可以正常工作，等待线程完成。
         */
        assert(!joined_); // 确保线程未加入
        joined_ = true; // 设置线程已加入
        return pthread_join(pthreadId_, NULL); // 等待线程结束，返回pthread_join的结果
    }
} // namespace muduo

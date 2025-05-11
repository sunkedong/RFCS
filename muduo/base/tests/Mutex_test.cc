#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"
#include "muduo/base/Timestamp.h"

#include <vector>
#include <stdio.h>

// 使用muduo库的命名空间
using namespace muduo;
// 使用标准库的命名空间
using namespace std;

// 全局互斥锁
MutexLock g_mutex;
// 全局整数 vector，用于存储数据
vector<int> g_vec;
// 常量，表示循环的次数
const int kCount = 10 * 1000 * 1000; // 1000万次

// 线程执行的函数
void threadFunc()
{
    Timestamp start(Timestamp::now());
    // 循环kCount次，每次往vector中添加一个数字
    for (int i = 0; i < kCount; ++i)
    {
        // 使用MutexLockGuard来自动管理互斥锁的加锁和解锁
        MutexLockGuard lock(g_mutex);
        // 向全局vector中添加数据
        g_vec.push_back(i);
    }
    printf("thread(s) with lock %f\n", timeDifference(Timestamp::now(), start));
}

// foo函数声明，指定该函数不会内联
int foo() __attribute__ ((noinline));

// 全局计数器
int g_count = 0;

// foo函数定义
int foo()
{
    // 使用MutexLockGuard加锁，确保当前线程对g_mutex的独占访问
    MutexLockGuard lock(g_mutex);
    // 检查当前线程是否持有该互斥锁
    if (!g_mutex.isLockedByThisThread())
    {
        printf("FAIL\n"); // 如果没有持有锁，输出错误信息
        return -1;
    }

    // 增加全局计数器
    ++g_count;
    return 0;
}

// 主函数
int main()
{
    // 输出pthread_mutex_t类型的大小
    printf("sizeof pthread_mutex_t: %zd\n", sizeof(pthread_mutex_t));
    // 输出MutexLock类型的大小
    printf("sizeof Mutex: %zd\n", sizeof(MutexLock));
    // 输出pthread_cond_t类型的大小
    printf("sizeof pthread_cond_t: %zd\n", sizeof(pthread_cond_t));
    // 输出Condition类型的大小
    printf("sizeof Condition: %zd\n", sizeof(Condition));

    // 调用foo函数，并通过MCHECK宏检查返回值是否为0
    MCHECK(foo());
    // 如果g_count不等于1，说明MCHECK被调用了两次，输出错误信息并终止程序
    if (g_count != 1)
    {
        printf("MCHECK calls twice.\n");
        abort(); // 终止程序
    }

    // 设置最大线程数
    const int kMaxThreads = 8;
    // 预留g_vec空间，避免多线程操作时频繁重新分配内存
    g_vec.reserve(kMaxThreads * kCount);

    // 记录开始时间，进行单线程的无锁插入操作
    Timestamp start(Timestamp::now());
    for (int i = 0; i < kCount; ++i)
    {
        g_vec.push_back(i); // 向g_vec中添加数据
    }
    // 打印无锁单线程操作的耗时
    printf("single thread without lock %f\n", timeDifference(Timestamp::now(), start));

    // 重新记录开始时间，进行单线程的加锁插入操作
    start = Timestamp::now();
    threadFunc(); // 执行带锁的线程操作
    // 打印带锁单线程操作的耗时
    printf("single thread with lock %f\n", timeDifference(Timestamp::now(), start));

    // 使用多线程进行插入操作
    for (int nthreads = 1; nthreads < kMaxThreads; ++nthreads)
    {
        // 创建一个线程指针的向量
        std::vector<std::unique_ptr<Thread>> threads;
        g_vec.clear(); // 清空g_vec，重新开始插入数据
        // 记录开始时间，进行多线程插入操作
        start = Timestamp::now();
        // 启动nthreads个线程，每个线程执行threadFunc函数
        for (int i = 0; i < nthreads; ++i)
        {
            threads.emplace_back(new Thread(&threadFunc)); // 创建线程并添加到线程向量中
            threads.back()->start(); // 启动线程
        }
        // 等待所有线程完成
        for (int i = 0; i < nthreads; ++i)
        {
            threads[i]->join(); // 等待线程结束
        }
        // 打印多线程操作的耗时
        printf("%d thread(s) with lock %f\n", nthreads, timeDifference(Timestamp::now(), start));
    }
}

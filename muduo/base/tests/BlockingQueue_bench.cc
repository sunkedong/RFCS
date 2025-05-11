#include "muduo/base/BlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Thread.h"
#include "muduo/base/Timestamp.h"

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

bool g_verbose = false;

// Many threads, one queue.
// Bench类用于测试多个线程并发时的延迟表现。
class Bench
{
public:
    // 构造函数：初始化多个线程
    Bench(int numThreads)
        : latch_(numThreads)  // 初始化CountDownLatch，等待numThreads个线程完成启动
    {
        threads_.reserve(numThreads);  // 为线程预留空间，避免动态扩容
        for (int i = 0; i < numThreads; ++i)  // 创建numThreads个线程
        {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);  // 给每个线程命名
            threads_.emplace_back(new muduo::Thread(  // 创建并启动线程
                std::bind(&Bench::threadFunc, this), muduo::string(name)));  // 绑定线程函数
        }
        for (auto& thr : threads_)  // 启动所有线程
        {
            thr->start();
        }
    }

    // 运行测试函数，执行指定次数的操作
    void run(int times)
    {
        printf("waiting for count down latch\n");  // 打印等待信息，等待所有线程启动
        latch_.wait();  // 等待所有线程完成启动
        LOG_INFO << threads_.size() << " threads started";  // 输出线程启动信息
        int64_t total_delay = 0;  // 总延迟初始化
        for (int i = 0; i < times; ++i)  // 重复运行`times`次
        {
            muduo::Timestamp now(muduo::Timestamp::now());  // 获取当前时间戳
            queue_.put(now);  // 将时间戳放入队列
            total_delay += delay_queue_.take();  // 从delay队列中获取延迟并累加
        }
        // 输出平均延迟（单位：微秒）
        printf("Average delay: %.3fus\n", static_cast<double>(total_delay) / times);
    }

    // 等待所有线程执行完毕
    void joinAll()
    {
        // 通过放入无效时间戳来通知每个线程退出
        for (size_t i = 0; i < threads_.size(); ++i)
        {
            queue_.put(muduo::Timestamp::invalid());  // 向队列中放入无效时间戳
        }

        // 等待所有线程结束
        for (auto& thr : threads_)
        {
            thr->join();  // 阻塞，直到线程执行完毕
        }
        LOG_INFO << threads_.size() << " threads stopped";  // 输出线程停止信息
    }

private:
    // 线程的执行函数
    void threadFunc()
    {
        // 如果开启了详细模式，则打印线程启动信息
        if (g_verbose)
        {
            printf("tid=%d, %s started\n",  // 输出线程ID和线程名称
                   muduo::CurrentThread::tid(),
                   muduo::CurrentThread::name());
        }

        std::map<int, int> delays;  // 用于统计不同延迟值的出现次数
        latch_.countDown();  // 计数器减1，表示一个线程已经启动
        bool running = true;
        while (running)  // 线程继续运行，直到接收到无效时间戳
        {
            muduo::Timestamp t(queue_.take());  // 从队列中取出时间戳
            muduo::Timestamp now(muduo::Timestamp::now());  // 获取当前时间戳
            if (t.valid())  // 如果取出的时间戳有效
            {
                int delay = static_cast<int>(timeDifference(now, t) * 1000000);  // 计算延迟，单位微秒
                // printf("tid=%d, latency = %d us\n",  // 打印延迟（注释掉了）
                //        muduo::CurrentThread::tid(), delay);
                ++delays[delay];  // 统计该延迟值的出现次数
                delay_queue_.put(delay);  // 将延迟值放入delay队列
            }
            running = t.valid();  // 如果接收到无效时间戳，停止线程
        }

        // 如果开启了详细模式，则打印线程停止信息和延迟统计
        if (g_verbose)
        {
            printf("tid=%d, %s stopped\n",  // 输出线程ID和线程名称
                   muduo::CurrentThread::tid(),
                   muduo::CurrentThread::name());
            // 打印该线程所有延迟统计数据
            for (const auto& delay : delays)
            {
                printf("tid = %d, delay = %d, count = %d\n",  // 输出延迟值和其出现次数
                       muduo::CurrentThread::tid(),
                       delay.first, delay.second);
            }
        }
    }

    // 阻塞队列：存储时间戳
    muduo::BlockingQueue<muduo::Timestamp> queue_;
    // 阻塞队列：存储延迟值
    muduo::BlockingQueue<int> delay_queue_;
    // 用于等待线程启动的计数器
    muduo::CountDownLatch latch_;
    // 存储线程对象
    std::vector<std::unique_ptr<muduo::Thread>> threads_;
};

// 主函数
int main(int argc, char* argv[])
{
    // 从命令行参数获取线程数，默认使用1个线程
    int threads = argc > 1 ? atoi(argv[1]) : 1;

    // 创建一个Bench对象，并传入线程数
    Bench t(threads);
    t.run(100000);  // 执行100000次测试
    t.joinAll();  // 等待所有线程完成
}

#include "muduo/base/BlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Thread.h"
#include "muduo/base/Timestamp.h"

#include <map>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

// hot potato benchmarking https://en.wikipedia.org/wiki/Hot_potato
// N threads, one hot potato.
class Bench
{
public:
    // 构造函数，初始化线程数量和其他资源
    /*
    
    */
    Bench(int numThreads)
        : startLatch_(numThreads), // 初始化线程启动的计数门闩
          stopLatch_(1)           // 初始化线程停止的计数门闩，初始值为1
    {
        // 预分配队列和线程的空间，避免后续动态分配
        queues_.reserve(numThreads);
        threads_.reserve(numThreads);

        // 创建线程和对应的工作队列
        for (int i = 0; i < numThreads; ++i)
        {
            queues_.emplace_back(new muduo::BlockingQueue<int>()); // 创建工作队列
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);      // 为线程命名
            threads_.emplace_back(new muduo::Thread(              // 创建线程对象
                [this, i] { threadFunc(i); },                     // 设置线程执行的函数
                muduo::string(name)));                            // 设置线程名
        }
    }

    // 启动所有线程
    //调用 startLatch_ 等待所有线程启动完成，确保线程启动后才进入主任务运行阶段。
    void Start()
    {
        muduo::Timestamp start = muduo::Timestamp::now(); // 记录开始时间
        for (auto& thr : threads_)
        {
            thr->start(); // 启动线程
        }
        // 会去执行函数，函数中会有countdown，等全部countdown之后，我们回统计事件，此函数也将结束
        startLatch_.wait(); // 等待所有线程启动完成
        muduo::Timestamp started = muduo::Timestamp::now(); // 记录所有线程启动后的时间
        printf("all %zd threads started, %.3fms\n",
               threads_.size(), 1e3 * timeDifference(started, start)); // 输出线程启动时间
    }

    // 运行主任务
    void Run()
    {
        muduo::Timestamp start = muduo::Timestamp::now(); // 记录开始时间
        const int rounds = 100003;                        // 设定任务的总轮次
        queues_[0]->put(rounds);                          // 将任务放入第一个队列

        auto done = done_.take();                         // 等待任务完成信号
        double elapsed = timeDifference(done.second, start); // 计算总耗时
        printf("thread id=%d done, total %.3fms, %.3fus / round\n",
               done.first, 1e3 * elapsed, 1e6 * elapsed / rounds); // 输出完成信息
    }

    // 停止所有线程
    void Stop()
    {
        muduo::Timestamp stop = muduo::Timestamp::now(); // 记录停止时间
        for (const auto& queue : queues_)
        {
            queue->put(-1); // 向每个队列发送停止信号
        }
        for (auto& thr : threads_)
        {
            thr->join(); // 等待线程结束
        }

        muduo::Timestamp t2 = muduo::Timestamp::now(); // 记录所有线程结束的时间
        printf("all %zd threads joined, %.3fms\n",
               threads_.size(), 1e3 * timeDifference(t2, stop)); // 输出线程停止时间
    }

private:
    // 每个线程执行的工作函数
    void threadFunc(int id)
    {
        startLatch_.countDown(); // 线程启动后减少计数

        muduo::BlockingQueue<int>* input = queues_[id].get();       // 当前线程的输入队列
        muduo::BlockingQueue<int>* output = queues_[(id + 1) % queues_.size()].get(); // 下一个线程的输出队列
        while (true)
        {
            int value = input->take(); // 从输入队列取任务
            if (value > 0)
            {
                output->put(value - 1); // 将任务转移到下一个队列，值减1
                if (verbose_)
                {
                    // printf("thread %d, got %d\n", id, value); // 如果启用调试信息，打印日志
                }
                continue;
            }

            if (value == 0) // 如果任务值为0，表示完成任务
            {
                done_.put(std::make_pair(id, muduo::Timestamp::now())); // 记录完成信息
            }
            break; // 停止线程
        }
    }

    using TimestampQueue = muduo::BlockingQueue<std::pair<int, muduo::Timestamp>>; // 定义完成任务队列的类型
    TimestampQueue done_;  // 存储任务完成信息的队列
    muduo::CountDownLatch startLatch_, stopLatch_; // 启动和停止用的计数门闩
    std::vector<std::unique_ptr<muduo::BlockingQueue<int>>> queues_; // 工作队列
    std::vector<std::unique_ptr<muduo::Thread>> threads_;            // 线程
    const bool verbose_ = true; // 是否打印调试信息
};

int main(int argc, char* argv[])
{
    int threads = argc > 1 ? atoi(argv[1]) : 1; // 从命令行参数获取线程数量

    printf("sizeof BlockingQueue = %zd\n", sizeof(muduo::BlockingQueue<int>)); // 打印BlockingQueue的大小
    printf("sizeof deque<int> = %zd\n", sizeof(std::deque<int>));              // 打印std::deque的大小
    Bench t(threads);   // 创建Bench对象
    t.Start();          // 启动线程
    t.Run();            // 运行任务
    t.Stop();           // 停止线程
    // exit(0);          // 程序退出
}


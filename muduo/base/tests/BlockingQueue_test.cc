#include "muduo/base/BlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/Thread.h"

#include <memory>
#include <string>
#include <vector>
#include <stdio.h>
#include <unistd.h>

class Test
{
public:
  // 构造函数，接受线程数 numThreads
  Test(int numThreads)
      : latch_(numThreads)  // 初始化 CountDownLatch，确保所有线程同步启动
  {
    // 为每个线程创建并命名
    for (int i = 0; i < numThreads; ++i)
    {
      char name[32];
      snprintf(name, sizeof name, "work thread %d", i);  // 格式化线程名称
      // 创建线程，并绑定线程函数 threadFunc
      threads_.emplace_back(new muduo::Thread(
          std::bind(&Test::threadFunc, this), muduo::string(name)));
    }
    // 启动所有线程
    for (auto &thr : threads_)
    {
      thr->start();
    }
  }

  // 运行测试，执行指定次数的任务
  void run(int times)
  {
    printf("waiting for count down latch\n");
    latch_.wait();  // 等待所有线程同步启动
    printf("all threads started\n");

    // 执行指定次数的任务
    for (int i = 0; i < times; ++i)
    {
      char buf[32];
      snprintf(buf, sizeof buf, "hello %d", i);  // 格式化数据
      queue_.put(buf);  // 将数据放入队列
      printf("tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());  // 打印线程ID、数据和队列大小
    }
  }

  // 等待所有线程结束
  void joinAll()
  {
    // 向每个线程发送 "stop" 消息，通知线程停止
    for (size_t i = 0; i < threads_.size(); ++i)
    {
      queue_.put("stop");
    }

    // 等待每个线程执行完毕
    for (auto &thr : threads_)
    {
      thr->join();
    }
  }

private:
  // 线程函数，执行队列中的任务
  void threadFunc()
  {
    printf("tid=%d, %s started\n",
           muduo::CurrentThread::tid(),
           muduo::CurrentThread::name());  // 打印线程ID和名称

    latch_.countDown();  // 线程启动后，CountDownLatch减一，允许主线程继续执行
    bool running = true;
    while (running)
    {
      // 从队列中获取数据
      std::string d(queue_.take());
      printf("tid=%d, get data = %s, size = %zd\n", muduo::CurrentThread::tid(), d.c_str(), queue_.size());  // 打印线程ID、获取的数据和队列大小

      // 如果数据是 "stop"，则退出循环
      running = (d != "stop");
    }

    // 打印线程结束的信息
    printf("tid=%d, %s stopped\n",
           muduo::CurrentThread::tid(),
           muduo::CurrentThread::name());
  }

  // 定义阻塞队列，用于线程之间传递数据
  muduo::BlockingQueue<std::string> queue_;
  // CountDownLatch，确保所有线程启动完成后再执行主线程
  muduo::CountDownLatch latch_;
  // 存储所有线程的容器
  std::vector<std::unique_ptr<muduo::Thread>> threads_;
};

// 测试移动语义
void testMove()
{
  muduo::BlockingQueue<std::unique_ptr<int>> queue;  // 创建一个存储智能指针的阻塞队列
  queue.put(std::unique_ptr<int>(new int(42)));  // 将一个智能指针放入队列
  std::unique_ptr<int> x = queue.take();  // 从队列中取出智能指针
  printf("took %d\n", *x);  // 打印取出的值
  *x = 123;  // 修改取出的值
  queue.put(std::move(x));  // 使用 std::move 转移智能指针，放入队列
  std::unique_ptr<int> y = queue.take();  // 从队列中取出智能指针
  printf("took %d\n", *y);  // 打印取出的修改后的值
}

int main()
{
  printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());  // 打印当前进程ID和线程ID
  Test t(5);  // 创建Test对象，启动5个线程
  t.run(100);  // 执行100次任务
  t.joinAll();  // 等待所有线程结束

  testMove();  // 测试移动语义

  printf("number of created threads %d\n", muduo::Thread::numCreated());  // 打印创建的线程数量
}

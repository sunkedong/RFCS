#include "muduo/net/EventLoopThread.h"  // 引入EventLoopThread类的头文件，EventLoopThread用于在独立线程中运行EventLoop
#include "muduo/net/EventLoop.h"        // 引入EventLoop类的头文件，EventLoop是用于处理I/O多路复用的事件循环
#include "muduo/base/Thread.h"          // 引入Thread类的头文件，提供线程创建和管理的功能
#include "muduo/base/CountDownLatch.h"  // 引入CountDownLatch类的头文件，提供线程同步机制

#include <stdio.h>                      // 引入stdio.h，用于打印输出
#include <unistd.h>                     // 引入unistd.h，提供对系统调用的访问（例如sleep）

using namespace muduo; // 使用muduo命名空间
using namespace muduo::net; // 使用muduo::net命名空间，包含网络相关功能

// 打印当前进程ID、线程ID和EventLoop指针
void print(EventLoop *p = NULL) {
    printf("print: pid = %d, tid = %d, loop = %p\n",
           getpid(), CurrentThread::tid(), p);
}

// 退出EventLoop并打印相关信息
void quit(EventLoop *p) {
    print(p); // 打印当前的EventLoop信息
    p->quit(); // 调用EventLoop的quit()方法退出事件循环
}

int main() {
    print(); {
        // 创建一个EventLoopThread对象，且该线程不会启动EventLoop
        EventLoopThread thr1; // never start
    } {
        // 析构时调用quit()，即在EventLoopThread的析构函数中调用quit()
        EventLoopThread thr2; // 创建EventLoopThread对象
        EventLoop *loop = thr2.startLoop(); // 启动EventLoopThread，返回对应的EventLoop对象
        loop->runInLoop(std::bind(print, loop)); // 在EventLoop的事件循环中执行print函数，传入当前loop指针
        CurrentThread::sleepUsec(500 * 1000); // 当前线程休眠500ms，确保事件循环能够运行
    } {
        // 在析构之前调用quit()退出EventLoop
        EventLoopThread thr3; // 创建EventLoopThread对象
        EventLoop *loop = thr3.startLoop(); // 启动EventLoopThread并返回对应的EventLoop对象
        loop->runInLoop(std::bind(quit, loop)); // 在事件循环中执行quit函数，退出事件循环
        CurrentThread::sleepUsec(500 * 1000); // 当前线程休眠500ms，确保quit能够执行
    }
}


/*
print: pid = 20593, tid = 20593, loop = (nil)
print: pid = 20593, tid = 20597, loop = 0x71d1217ff980
print: pid = 20593, tid = 20598, loop = 0x71d1217ff980

*/
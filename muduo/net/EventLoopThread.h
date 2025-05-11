// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"

namespace muduo
{
    namespace net
    {
        class EventLoop;

        class EventLoopThread : noncopyable
        {
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;
            /*
                cb（类型为ThreadInitCallback）：这是一个回调函数类型的参数，默认值是ThreadInitCallback()，
                即一个默认构造的std::function<void(EventLoop*)>。这意味着如果没有传递回调函数，
                cb将会是一个空的std::function。
            */
            EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                            const string& name = string());
            ~EventLoopThread();
            EventLoop* startLoop();

        private:
            void threadFunc();

            EventLoop* loop_ GUARDED_BY(mutex_);
            bool exiting_;
            Thread thread_;
            MutexLock mutex_;
            Condition cond_ GUARDED_BY(mutex_);
            ThreadInitCallback callback_;
        };
    } // namespace net
} // namespace muduo

#endif  // MUDUO_NET_EVENTLOOPTHREAD_H

// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

// 这是一个内部头文件，外部不应该直接包含它
#ifndef MUDUO_NET_CHANNEL_H
#define MUDUO_NET_CHANNEL_H

// 引入 Muduo 库中的非拷贝构造特性和时间戳类
#include "muduo/base/noncopyable.h"
#include "muduo/base/Timestamp.h"

// 引入 C++ 标准库的函数对象和智能指针
#include <functional>
#include <memory>

namespace muduo
{
    namespace net
    {
        // 前向声明 EventLoop 类
        class EventLoop;

        ///
        /// 一个可选择的 I/O 通道。
        ///
        /// 该类不拥有文件描述符。
        /// 文件描述符可以是套接字、eventfd、timerfd 或 signalfd 等
        ///
        /// Channel 类不直接管理文件描述符（fd），而是将文件描述符封装在其他对象（如 Poller 或 EventLoop）中，
        /// 目的是实现更加灵活和解耦的事件驱动模型。文件描述符的管理和事件分发是两个不同的职责，通过解耦这两个职责，
        /// 可以使得代码更加清晰和可维护。
        ///
        class Channel : noncopyable
        {
        public:
            // 定义事件回调类型
            typedef std::function<void()> EventCallback;
            typedef std::function<void(Timestamp)> ReadEventCallback;

            // 构造函数，传入 EventLoop 和文件描述符
            Channel(EventLoop* loop, int fd);
            // 析构函数
            ~Channel();

            // 处理事件
            void handleEvent(Timestamp receiveTime);

            // 设置读事件回调
            void setReadCallback(ReadEventCallback cb)
            {
                readCallback_ = std::move(cb);  // 使用 std::move 来避免不必要的拷贝
            }

            // 设置写事件回调
            void setWriteCallback(EventCallback cb)
            {
                writeCallback_ = std::move(cb);
            }

            // 设置关闭事件回调
            void setCloseCallback(EventCallback cb)
            {
                closeCallback_ = std::move(cb);
            }

            // 设置错误事件回调
            void setErrorCallback(EventCallback cb)
            {
                errorCallback_ = std::move(cb);
            }

            /// 将 Channel 绑定到一个由 shared_ptr 管理的对象，防止在 handleEvent 时该对象被销毁
            void tie(const std::shared_ptr<void>&);

            // 获取文件描述符
            int fd() const { return fd_; }
            // 获取当前事件
            int events() const { return events_; }
            // 设置接收到的事件类型
            void set_revents(int revt) { revents_ = revt; } // 由 Poller 使用
            // 检查当前是否没有事件
            bool isNoneEvent() const { return events_ == kNoneEvent; }

            // 启用读事件
            void enableReading()
            {
                events_ |= kReadEvent;  // 将读事件加入事件集合
                update();  // 更新事件
            }

            // 禁用读事件
            void disableReading()
            {
                events_ &= ~kReadEvent;  // 从事件集合中移除读事件
                update();
            }

            // 启用写事件
            void enableWriting()
            {
                events_ |= kWriteEvent;  // 将写事件加入事件集合
                update();
            }

            // 禁用写事件
            void disableWriting()
            {
                events_ &= ~kWriteEvent;  // 从事件集合中移除写事件
                update();
            }

            // 禁用所有事件
            void disableAll()
            {
                events_ = kNoneEvent;  // 将事件清空
                update();
            }

            // 判断当前是否为写事件
            bool isWriting() const { return events_ & kWriteEvent; }
            // 判断当前是否为读事件
            bool isReading() const { return events_ & kReadEvent; }

            // 供 Poller 使用，获取和设置索引
            int index() { return index_; }
            void set_index(int idx) { index_ = idx; }

            // 调试方法，将接收到的事件转换为字符串
            string reventsToString() const;
            // 调试方法，将当前事件转换为字符串
            string eventsToString() const;

            // 设置是否不记录 HUP 事件的日志
            void doNotLogHup() { logHup_ = false; }

            // 获取事件循环对象
            EventLoop* ownerLoop() { return loop_; }
            // 从事件循环中移除该通道
            void remove();

        private:
            // 将事件类型转换为字符串，供调试使用
            static string eventsToString(int fd, int ev);

            // 更新事件
            void update();
            // 处理事件的实际逻辑
            void handleEventWithGuard(Timestamp receiveTime);

            // 定义事件常量
            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            // 成员变量
            EventLoop* loop_;  // 事件循环对象
            const int fd_;  // 文件描述符
            int events_;  // 当前事件类型
            int revents_; // 接收到的事件类型（由 Poller 或 epoll 等设置）
            int index_;  // Poller 使用的索引
            bool logHup_;  // 是否记录 HUP 事件日志

            // 绑定对象的 weak_ptr，防止持有对象导致循环引用
            std::weak_ptr<void> tie_;
            bool tied_;  // 是否已绑定对象
            bool eventHandling_;  // 是否正在处理事件
            bool addedToLoop_;  // 是否已添加到事件循环
            // 事件回调函数
            ReadEventCallback readCallback_;
            EventCallback writeCallback_;
            EventCallback closeCallback_;
            EventCallback errorCallback_;
        };
    } // namespace net
} // namespace muduo

#endif  // MUDUO_NET_CHANNEL_H

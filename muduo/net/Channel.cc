// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

// 引入 Muduo 库中的日志、Channel 和 EventLoop 头文件
#include "muduo/base/Logging.h"
#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"

// 引入标准库的字符串流
#include <sstream>

// 引入 `poll` 头文件，使用它来定义事件类型（如 POLLIN, POLLOUT 等）
#include <poll.h>

using namespace muduo;  // 使用 muduo 命名空间
using namespace muduo::net;  // 使用 muduo::net 命名空间

// 定义 Channel 类中事件常量，表示不同的 I/O 事件类型
const int Channel::kNoneEvent = 0;  // 没有事件
const int Channel::kReadEvent = POLLIN | POLLPRI;  // 可读事件（包括普通数据和优先数据）
const int Channel::kWriteEvent = POLLOUT;  // 可写事件

// Channel 构造函数，初始化通道相关的属性
Channel::Channel(EventLoop *loop, int fd__)
    : loop_(loop),  // 事件循环对象
      fd_(fd__),     // 文件描述符
      events_(0),    // 初始事件为 0
      revents_(0),   // 事件响应初始化为 0
      index_(-1),    // 默认通道索引为 -1
      logHup_(true), // 默认开启 HUP 日志
      tied_(false),  // 默认没有绑定对象
      eventHandling_(false),  // 默认没有事件处理
      addedToLoop_(false) {  // 默认没有添加到事件循环
}

// Channel 析构函数，确保对象销毁时，相关状态正确
Channel::~Channel() {
    assert(!eventHandling_);  // 确保事件没有在处理
    assert(!addedToLoop_);    // 确保通道没有被加入事件循环
    if (loop_->isInLoopThread()) {
        assert(!loop_->hasChannel(this));  // 确保通道没有被事件循环管理
    }
}

// 绑定一个共享指针对象到当前 Channel，通常用于绑定某个外部对象（例如控制对象）
void Channel::tie(const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true;
}

// 更新 Channel，向事件循环注册该通道的状态
void Channel::update() {
    addedToLoop_ = true;  // 标记通道已经添加到事件循环
    loop_->updateChannel(this);  // 更新事件循环中的通道
}

// 从事件循环中移除该通道
void Channel::remove() {
    assert(isNoneEvent());  // 确保当前没有事件
    addedToLoop_ = false;  // 标记通道未添加到事件循环
    loop_->removeChannel(this);  // 从事件循环中移除该通道
}

// 处理事件回调，传入接收到的时间戳
void Channel::handleEvent(Timestamp receiveTime) {
    std::shared_ptr<void> guard;
    // 如果有绑定对象，先检查它是否还有效
    if (tied_) {
        guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);  // 处理事件
        }
    } else {
        handleEventWithGuard(receiveTime);  // 直接处理事件
    }
}

// 实际的事件处理函数
void Channel::handleEventWithGuard(Timestamp receiveTime) {
    eventHandling_ = true;  // 标记正在处理事件
    LOG_TRACE << reventsToString();  // 打印当前事件状态

    // 处理挂起事件（POLLHUP），如果没有可读事件则关闭
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        if (logHup_) {
            LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLHUP";  // 打印警告日志
        }
        if (closeCallback_) closeCallback_();  // 如果设置了关闭回调，执行关闭操作
    }

    // 处理无效事件（POLLNVAL），打印警告日志
    if (revents_ & POLLNVAL) {
        LOG_WARN << "fd = " << fd_ << " Channel::handle_event() POLLNVAL";
    }

    // 处理错误事件（POLLERR 或 POLLNVAL）
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();  // 如果设置了错误回调，执行错误处理
    }

    // 处理可读事件（POLLIN, POLLPRI 或 POLLRDHUP）
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) readCallback_(receiveTime);  // 如果设置了读回调，执行读取操作
    }

    // 处理可写事件（POLLOUT）
    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();  // 如果设置了写回调，执行写入操作
    }

    eventHandling_ = false;  // 标记事件处理完毕
}

// 将 `revents_` 转换为字符串，方便调试
string Channel::reventsToString() const {
    return eventsToString(fd_, revents_);
}

// 将 `events_` 转换为字符串，方便调试
string Channel::eventsToString() const {
    return eventsToString(fd_, events_);
}

// 将给定的事件和文件描述符转换为字符串
string Channel::eventsToString(int fd, int ev) {
    std::ostringstream oss;  // 使用字符串流构建输出
    oss << fd << ": ";  // 文件描述符
    if (ev & POLLIN) oss << "IN ";  // 可读事件
    if (ev & POLLPRI) oss << "PRI ";  // 优先数据事件
    if (ev & POLLOUT) oss << "OUT ";  // 可写事件
    if (ev & POLLHUP) oss << "HUP ";  // 挂起事件
    if (ev & POLLRDHUP) oss << "RDHUP ";  // 关闭连接事件
    if (ev & POLLERR) oss << "ERR ";  // 错误事件
    if (ev & POLLNVAL) oss << "NVAL ";  // 无效事件

    return oss.str();  // 返回构建好的字符串
}

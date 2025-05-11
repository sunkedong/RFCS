// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

// 引入 Muduo 库中的基础类和类型
#include "muduo/base/copyable.h"
#include "muduo/base/StringPiece.h"

// 引入系统网络编程相关的头文件，定义了网络地址结构
#include <netinet/in.h>

namespace muduo {
    namespace net {
        namespace sockets {
            // 将 sockaddr_in6 类型转换为 sockaddr 类型的通用指针
            const struct sockaddr *sockaddr_cast(const struct sockaddr_in6 *addr);
        }

        ///
        /// `InetAddress` 类封装了 `sockaddr_in`（IPv4）和 `sockaddr_in6`（IPv6）结构。
        /// 这是一个 POD（Plain Old Data）接口类，主要用于表示网络地址。
        ///
        class InetAddress : public muduo::copyable {
        public:
            ///
            /// 构造函数：通过指定端口号构造网络地址。
            /// 主要用于 TcpServer 进行监听。
            ///
            explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

            ///
            /// 构造函数：通过给定的 IP 地址和端口构造网络地址。
            /// `ip` 格式应为 "1.2.3.4"。
            ///
            InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);

            ///
            /// 构造函数：通过给定的 `sockaddr_in`（IPv4）结构构造网络地址。
            /// 主要用于接受新的连接时获取远程地址。
            ///
            explicit InetAddress(const struct sockaddr_in &addr)
                : addr_(addr) { // 初始化 `addr_`（IPv4 地址）
            }

            ///
            /// 构造函数：通过给定的 `sockaddr_in6`（IPv6）结构构造网络地址。
            ///
            explicit InetAddress(const struct sockaddr_in6 &addr)
                : addr6_(addr) { // 初始化 `addr6_`（IPv6 地址）
            }

            ///
            /// 获取地址族（如 IPv4 或 IPv6）
            ///
            sa_family_t family() const { return addr_.sin_family; }

            ///
            /// 获取地址的 IP 部分，返回字符串格式的 IP 地址（如 "127.0.0.1"）
            ///
            string toIp() const;

            ///
            /// 获取地址的 IP 地址和端口号的字符串表示（如 "127.0.0.1:8080"）
            ///
            string toIpPort() const;

            ///
            /// 获取地址的端口号
            ///
            uint16_t port() const;

            // 默认的拷贝构造和赋值操作是可用的。

            ///
            /// 获取 `sockaddr` 类型的指针，通常用于与底层套接字通信。
            ///
            const struct sockaddr *getSockAddr() const { 
                return sockets::sockaddr_cast(&addr6_); // 转换并返回 `sockaddr` 类型的指针
            }

            ///
            /// 设置 IPv6 地址结构
            ///
            void setSockAddrInet6(const struct sockaddr_in6 &addr6) { 
                addr6_ = addr6; 
            }

            ///
            /// 获取 IPv4 地址的网络字节序（大端表示）形式。
            ///
            uint32_t ipv4NetEndian() const;

            ///
            /// 获取端口号的网络字节序（大端表示）形式。
            ///
            uint16_t portNetEndian() const { return addr_.sin_port; }

            ///
            /// 解析主机名为 IP 地址，解析成功返回 true。
            /// 该方法是线程安全的。
            ///
            static bool resolve(StringArg hostname, InetAddress *result);

            // 解析所有 IP 地址（可能存在多个 IP）并返回。
            // static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t port = 0);

            ///
            /// 设置 IPv6 地址的作用域 ID（Scope ID），用于指定地址的作用范围（如链路本地或全局地址）。
            ///
            void setScopeId(uint32_t scope_id);

        private:
            // 使用联合体存储 IPv4 和 IPv6 地址，因为它们不会同时使用。
            union {
                struct sockaddr_in addr_;  // IPv4 地址
                struct sockaddr_in6 addr6_; // IPv6 地址
            };
        };
    } // namespace net
} // namespace muduo

#endif  // MUDUO_NET_INETADDRESS_H

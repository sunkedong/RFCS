// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

// 引入 `InetAddress` 类的头文件
#include "muduo/net/InetAddress.h"

// 引入 Muduo 库中的其他公共头文件：日志、字节序操作和套接字操作
#include "muduo/base/Logging.h"
#include "muduo/net/Endian.h"
#include "muduo/net/SocketsOps.h"

// 引入标准的网络头文件，用于主机名解析等操作
#include <netdb.h>
#include <netinet/in.h>

// 设置特定的警告抑制，避免因旧式类型转换产生的警告
#pragma GCC diagnostic ignored "-Wold-style-cast"
// 定义常量 `INADDR_ANY` 和 `INADDR_LOOPBACK`，分别表示任何地址和回环地址
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
// 恢复对旧式类型转换警告的显示
#pragma GCC diagnostic error "-Wold-style-cast"

// 定义结构体 `sockaddr_in`，它描述了一个 IPv4 套接字地址
// 该结构体包含地址族、端口和 `in_addr`（IPv4 地址）

// 定义结构体 `in_addr`，它是一个 32 位的网络字节序地址

// 定义结构体 `sockaddr_in6`，它描述了一个 IPv6 套接字地址
// 该结构体包含地址族、端口、流信息、IPv6 地址和作用域 ID

using namespace muduo;  // 使用 muduo 命名空间
using namespace muduo::net;  // 使用 muduo::net 命名空间

// 确保 `InetAddress` 的大小与 `sockaddr_in6` 的大小相同
static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
// 确保 `sin_family` 字段在 `sockaddr_in` 中的偏移量为 0
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
// 确保 `sin6_family` 字段在 `sockaddr_in6` 中的偏移量为 0
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
// 确保 `sin_port` 字段在 `sockaddr_in` 中的偏移量为 2
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
// 确保 `sin6_port` 字段在 `sockaddr_in6` 中的偏移量为 2
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

// `InetAddress` 构造函数，使用端口号、回环地址标志和是否使用 IPv6 来初始化
InetAddress::InetAddress(uint16_t portArg, bool loopbackOnly, bool ipv6) {
    static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
    static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
    
    // 根据是否使用 IPv6 初始化地址
    if (ipv6) {
        memZero(&addr6_, sizeof addr6_);  // 清空地址
        addr6_.sin6_family = AF_INET6;  // 设置地址族为 IPv6
        in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;  // 选择回环地址或任意地址
        addr6_.sin6_addr = ip;  // 设置 IPv6 地址
        addr6_.sin6_port = sockets::hostToNetwork16(portArg);  // 设置端口号（网络字节序）
    } else {
        memZero(&addr_, sizeof addr_);  // 清空地址
        addr_.sin_family = AF_INET;  // 设置地址族为 IPv4
        in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;  // 选择回环地址或任意地址
        addr_.sin_addr.s_addr = sockets::hostToNetwork32(ip);  // 设置 IPv4 地址
        addr_.sin_port = sockets::hostToNetwork16(portArg);  // 设置端口号（网络字节序）
    }
}

// `InetAddress` 构造函数，使用给定的 IP 地址和端口号进行初始化
InetAddress::InetAddress(StringArg ip, uint16_t portArg, bool ipv6) {
    if (ipv6 || strchr(ip.c_str(), ':')) {  // 判断是否是 IPv6 地址
        memZero(&addr6_, sizeof addr6_);
        sockets::fromIpPort(ip.c_str(), portArg, &addr6_);  // 从 IP 地址和端口初始化 IPv6 地址
    } else {
        memZero(&addr_, sizeof addr_);
        sockets::fromIpPort(ip.c_str(), portArg, &addr_);  // 从 IP 地址和端口初始化 IPv4 地址
    }
}

// 将地址转换为 IP 地址和端口的字符串形式（如 "127.0.0.1:8080"）
string InetAddress::toIpPort() const {
    char buf[64] = "";
    sockets::toIpPort(buf, sizeof buf, getSockAddr());  // 使用底层函数填充字符串
    return buf;
}

// 将地址转换为 IP 地址的字符串形式（如 "127.0.0.1"）
string InetAddress::toIp() const {
    char buf[64] = "";
    sockets::toIp(buf, sizeof buf, getSockAddr());  // 使用底层函数填充字符串
    return buf;
}

// 获取 IPv4 地址的网络字节序（大端序）
uint32_t InetAddress::ipv4NetEndian() const {
    assert(family() == AF_INET);  // 确保地址族是 IPv4
    return addr_.sin_addr.s_addr;  // 返回 IPv4 地址
}

// 获取端口号，并转换为主机字节序（小端序）
uint16_t InetAddress::port() const {
    return sockets::networkToHost16(portNetEndian());  // 转换端口号
}

// 静态缓冲区，用于存储解析结果
static __thread char t_resolveBuffer[64 * 1024];

// 解析主机名为 IP 地址，成功时返回 true
bool InetAddress::resolve(StringArg hostname, InetAddress *out) {
    assert(out != NULL);  // 确保输出指针不为空
    struct hostent hent;
    struct hostent *he = NULL;
    int herrno = 0;
    memZero(&hent, sizeof(hent));  // 清空 hostent 结构体

    // 调用系统函数进行主机名解析
    int ret = gethostbyname_r(hostname.c_str(), &hent, t_resolveBuffer, sizeof t_resolveBuffer, &he, &herrno);
    if (ret == 0 && he != NULL) {
        assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));  // 确保返回的是 IPv4 地址
        out->addr_.sin_addr = *reinterpret_cast<struct in_addr *>(he->h_addr);  // 解析结果填充到 `out`
        return true;
    } else {
        if (ret) {
            LOG_SYSERR << "InetAddress::resolve";  // 记录错误
        }
        return false;
    }
}

// 设置 IPv6 地址的作用域 ID
void InetAddress::setScopeId(uint32_t scope_id) {
    if (family() == AF_INET6) {
        addr6_.sin6_scope_id = scope_id;  // 设置作用域 ID
    }
}

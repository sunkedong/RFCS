// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include "muduo/base/CurrentThread.h"
#include "muduo/base/noncopyable.h"
#include <assert.h>
#include <pthread.h>

// Thread safety annotations {
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html

// Enable thread safety attributes only with clang.
// The attributes can be safely erased when compiling with other compilers.
#if defined(__clang__) && (!defined(SWIG))
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   __attribute__((x))
#else
#define THREAD_ANNOTATION_ATTRIBUTE__(x)   // no-op
#endif

#define CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(capability(x))

#define SCOPED_CAPABILITY \
  THREAD_ANNOTATION_ATTRIBUTE__(scoped_lockable)

#define GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))

#define PT_GUARDED_BY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(pt_guarded_by(x))

#define ACQUIRED_BEFORE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_before(__VA_ARGS__))

#define ACQUIRED_AFTER(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquired_after(__VA_ARGS__))

#define REQUIRES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_capability(__VA_ARGS__))

#define REQUIRES_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(requires_shared_capability(__VA_ARGS__))

#define ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_capability(__VA_ARGS__))

#define ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(acquire_shared_capability(__VA_ARGS__))

#define RELEASE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_capability(__VA_ARGS__))

#define RELEASE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(release_shared_capability(__VA_ARGS__))

#define TRY_ACQUIRE(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_capability(__VA_ARGS__))

#define TRY_ACQUIRE_SHARED(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(try_acquire_shared_capability(__VA_ARGS__))

#define EXCLUDES(...) \
  THREAD_ANNOTATION_ATTRIBUTE__(locks_excluded(__VA_ARGS__))

#define ASSERT_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_capability(x))

#define ASSERT_SHARED_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(assert_shared_capability(x))

#define RETURN_CAPABILITY(x) \
  THREAD_ANNOTATION_ATTRIBUTE__(lock_returned(x))

#define NO_THREAD_SAFETY_ANALYSIS \
  THREAD_ANNOTATION_ATTRIBUTE__(no_thread_safety_analysis)

// End of thread safety annotations }

#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail (int errnum,
                                  const char *file,
                                  unsigned int line,
                                  const char *function)
    noexcept __attribute__ ((__noreturn__));
__END_DECLS
#endif

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       if (__builtin_expect(errnum != 0, 0))    \
                         __assert_perror_fail (errnum, __FILE__, __LINE__, __func__);})

#else  // CHECK_PTHREAD_RETURN_VALUE

#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       assert(errnum == 0); (void) errnum;})

#endif // CHECK_PTHREAD_RETURN_VALUE

namespace muduo
{
    // Use as data member of a class, eg.
    //
    // class Foo
    // {
    //  public:
    //   int size() const;
    //
    //  private:
    //   mutable MutexLock mutex_;
    //   std::vector<int> data_ GUARDED_BY(mutex_);
    // };
    // 定义一个名为MutexLock的类，继承自非拷贝构造的noncopyable类，表示一个互斥锁类
    class CAPABILITY("mutex") MutexLock : noncopyable
    {
    public:
        // 构造函数，初始化holder_为0，并且初始化pthread_mutex_
        MutexLock()
            : holder_(0) // 初始化持有锁的线程ID为0，表示没有线程持有锁
        {
            MCHECK(pthread_mutex_init(&mutex_, NULL)); // 初始化互斥锁mutex_，如果失败则触发错误
        }

        // 析构函数，销毁mutex_并确保没有线程持有锁
        ~MutexLock()
        {
            assert(holder_ == 0); // 确保析构时没有线程持有锁，如果有则会触发断言
            MCHECK(pthread_mutex_destroy(&mutex_)); // 销毁互斥锁
        }

        // 判断当前线程是否持有该互斥锁
        bool isLockedByThisThread() const
        {
            return holder_ == CurrentThread::tid(); // 如果当前线程ID等于持有者ID，返回true
        }

        // 用于确保在锁定时进行断言，检查当前线程是否是锁的持有者
        void assertLocked() const ASSERT_CAPABILITY(this)
        {
            assert(isLockedByThisThread()); // 如果当前线程不是持有锁的线程，触发断言
        }

        // 内部使用的锁定函数
        void lock() ACQUIRE()
        {
            MCHECK(pthread_mutex_lock(&mutex_)); // 尝试加锁互斥锁，如果失败则触发错误
            assignHolder(); // 将当前线程ID赋给holder_，表示当前线程持有锁
        }

        // 内部使用的解锁函数
        void unlock() RELEASE()
        {
            unassignHolder(); // 清除当前线程的持有标记
            MCHECK(pthread_mutex_unlock(&mutex_)); // 解锁互斥锁，如果失败则触发错误
        }

        // 返回底层的pthread_mutex_t类型互斥锁指针（非const）
        pthread_mutex_t* getPthreadMutex() /* non-const */
        {
            return &mutex_; // 返回互斥锁的地址
        }

    private:
        // 声明友元类Condition，可以访问MutexLock的私有成员
        friend class Condition;

        // 一个用于管理holder_赋值的辅助类，确保在作用域内正确地管理holder_的状态
        class UnassignGuard : noncopyable
        {
        public:
            // 构造函数，传入MutexLock对象，初始化时解除持有者的记录
            explicit UnassignGuard(MutexLock& owner)
                : owner_(owner)
            {
                owner_.unassignHolder(); // 解除holder_标记
            }

            // 析构函数，在作用域结束时重新分配holder_
            ~UnassignGuard()
            {
                owner_.assignHolder(); // 恢复holder_标记为当前线程
            }

        private:
            MutexLock& owner_; // 引用MutexLock对象
        };

        // 清除holder_标记，表示没有线程持有锁
        void unassignHolder()
        {
            holder_ = 0; // 将holder_设置为0，表示没有线程持有锁
        }

        // 赋值当前线程ID给holder_，表示当前线程持有锁
        void assignHolder()
        {
            holder_ = CurrentThread::tid(); // 将当前线程ID赋给holder_
        }

        pthread_mutex_t mutex_; // 实际的互斥锁
        pid_t holder_; // 记录当前持有锁的线程ID
    };


    // Use as a stack variable, eg.
    // int Foo::size() const
    // {
    //   MutexLockGuard lock(mutex_);
    //   return data_.size();
    // }
    class SCOPED_CAPABILITY MutexLockGuard : noncopyable
    {
    public:
        explicit MutexLockGuard(MutexLock& mutex) ACQUIRE(mutex)
            : mutex_(mutex)
        {
            mutex_.lock();
        }

        ~MutexLockGuard() RELEASE()
        {
            mutex_.unlock();
        }

    private:
        MutexLock& mutex_;
    };
} // namespace muduo

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"

#endif  // MUDUO_BASE_MUTEX_H

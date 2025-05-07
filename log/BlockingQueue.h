// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef BLOCKINGQUEUE_H
#define BLOCKINGQUEUE_H

#include <mutex>
#include <condition_variable>
#include <deque>
#include <assert.h>

template <typename T>
class BlockingQueue  
{
public:
  BlockingQueue()
  {
    queue_();
  }

  void put(const T &x)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(x);
    notEmpty_.notify_one(); // wait morphing saves us
    // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
  }

  void put(T &&x)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(x));
    notEmpty_.notify_one();
  }

  T take()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    notEmpty_.wait(lock, [this]  {
      return !queue_.empty(); });

    assert(!queue_.empty());
    T front(std::move(queue_.front()));
    queue_.pop_front();
    return front;
  }

  size_t size() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

private:
  std::mutex mutex_;
  std::condition_variable notEmpty_ ;
  std::deque<T> queue_ ;
};

#endif // BLOCKINGQUEUE_H

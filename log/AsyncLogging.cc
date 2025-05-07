// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "AsyncLogging.h"
#include "LogFile.h"
#include "TimeStamp.h"

#include <stdio.h>


AsyncLogging::AsyncLogging(const string& basename,
                           off_t rollSize,
                           int flushInterval)
  : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_(),
    mutex_()
{
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
}

int append_cnt = 0;
void AsyncLogging::append(const char* logline, int len)
{
  std::cout << "append: " << logline;

  // return;
  // 1. 多线程加锁，线程安全
  std::lock_guard<std::mutex> lock(mutex_);;    // 多线程加锁
  // 2.判断是否写满buffer
  if (currentBuffer_->avail() > len)    // 判断buffer还有没有空间写入这条日志
  {
    // 2.1 buffer未满直接写入buffer
    currentBuffer_->append(logline, len); // 直接写入
  }
  else
  {
    // 2.2 当前buffer插入到buffer队列
    buffers_.push_back(std::move(currentBuffer_));    // buffers_是vector，把buffer入队列
    // printf("push_back append_cnt:%d, size:%d\n", ++append_cnt, buffers_.size());
    // 2.2.1 另个buffer是否为空，如果不为空则复用，如果为空则重新分配
    if (nextBuffer_)  // 用了双缓存
    {
      currentBuffer_ = std::move(nextBuffer_);    // 如果不为空则将buffer转移到currentBuffer_
    }
    else
    {
      // 重新分配buffer
      currentBuffer_.reset(new Buffer); // Rarely happens如果后端写入线程没有及时读取数据，那要再分配buffer
    }
    currentBuffer_->append(logline, len);    
    cond_.notify_one(); // 唤醒日志落盘线程
  }
}
int threadFunc_cnt = 0;
void AsyncLogging::threadFunc()
{
  assert(running_ == true);
  LogFile output(basename_, rollSize_, false);
  BufferPtr newBuffer1(new Buffer); // 是给currentBuffer_
  BufferPtr newBuffer2(new Buffer); // 是给nextBuffer_
  newBuffer1->bzero();  // 内容设置为'\0'
  newBuffer2->bzero();
  BufferVector buffersToWrite;    // 保存要写入的日志
  buffersToWrite.reserve(16);     // 队列初始化容量为16个buffer
  while (running_)
  {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());
  // MutexLockGuard lock(mutex_);  // 这个位置锁的粒度就大了，落盘的时候都会阻塞append
    { // 1. 多线程加锁，线程安全，注意锁的作用域
      std::unique_lock<std::mutex> lock(mutex_);
      // 2. 判断buffer队列是否为空，如果为空则休眠，如果不为空等待
      if (buffers_.empty())  // 没有数据可读取，休眠
      {
        // printf("waitForSeconds into\n");
        // 超时退出或者被唤醒 收到notify
        cond_.wait_for(lock, std::chrono::milliseconds(flushInterval_*1000), [this]  
				{
					return (!buffers_.empty()) | !running_; 
				});
        // printf("waitForSeconds leave\n");
      }
      // 3. 只要触发日志的落盘，不管当前的buffer是否写满都取出来，以后续写入磁盘
      buffers_.push_back(std::move(currentBuffer_));  // currentBuffer_被锁住  currentBuffer_被置空
      // printf("push_back threadFunc:%d, size:%d\n", ++threadFunc_cnt, buffers_.size());
      // 4.为当前buffer设置缓存空间，复用已经分配的空间
      currentBuffer_ = std::move(newBuffer1); // currentBuffer_ 需要内存空间
      // 5. 把前台日志队列buffers_的buffer转日到后台日志队列buffersToWrite，这样在日志落盘过程不影响
      //    前台日志队列buffers_.push_back的插入
      buffersToWrite.swap(buffers_);          // 用了双队列，把前端日志的队列所有buffer都转移到buffersToWrite队列
      if (!nextBuffer_)     // newBuffer2是给nextBuffer_
      {
        nextBuffer_ = std::move(newBuffer2);  // 如果为空则使用newBuffer2的缓存空间
      }
    } // 注意这里加锁的粒度，日志落盘的时候不需要加锁了，主要是双队列的功劳
    
    // 从这里是没有锁，数据落盘的时候不要加锁
    assert(!buffersToWrite.empty());

    // 6.判断日志是否有及时写入，如果不及时删掉一部分日志，并插入提示信息
    // fixme的操作 4M一个buffer *25 = 100M
    if (buffersToWrite.size() > 25)  // 这里缓存的数据太多了，比如4M为一个buffer空间，25个buffer就是100M了。
    {                               // 以一行日志100字节为例，那有100*1000*1000/100=100 0000 ,一百万的日志
      printf("Dropped\n");
      char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size()-2);    // 只保留2个buffer
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());   // 只保留2个buffer(默认4M)
    }
    // 7.循环写入所有buffer
    for (const auto& buffer : buffersToWrite)  // 遍历buffer
    {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffer->data(), buffer->length());    // 负责fwrite数据
    }
    // 8.刷新磁盘,刷到哪里去了能？
    output.flush();  //有待商榷  
    if (buffersToWrite.size() > 2)
    {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);   // 只保留2个buffer 
    }
    // 9. newBuffer1 和newBuffer2获取
    if (!newBuffer1)
    {
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());    // 复用buffer对象
      buffersToWrite.pop_back();
      newBuffer1->reset();    // 复位
    }

    if (!newBuffer2)
    {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());   // 复用buffer对象
      buffersToWrite.pop_back();
      newBuffer2->reset();   // 重置
    }

    buffersToWrite.clear(); 
    
  }
  output.flush();
}


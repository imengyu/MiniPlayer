#include "pch.h"
#include "CCAsyncTaskQueue.h"

CCAsyncTaskQueue::CCAsyncTaskQueue()
{
  workerQueue.alloc(32);
}

CCAsyncTaskQueue::~CCAsyncTaskQueue()
{
  Clear();
}

void CCAsyncTaskQueue::Clear()
{
  if (!workerQueue.empty()) {
    for (auto it = workerQueue.begin(); it; it = it->next)
      delete it->value;
    workerQueue.clear();
  }
}

int CCAsyncTaskQueue::Push(CCAsyncTask* task)
{
  workerQueueLock.lock();
  task->Id = workerQueueId++;
  if (workerQueueId > 65536) workerQueueId = 0;
  workerQueue.push(task);

  workerQueueLock.unlock();
  return task->Id;
}
CCAsyncTask* CCAsyncTaskQueue::Pop()
{
  workerQueueLock.lock();
  auto task = workerQueue.pop_front();
  workerQueueLock.unlock();
  return task;
}

#pragma once
#include <mutex>
#include "CCSimpleQueue.h"
#include "CCAsyncTask.h"

class CCAsyncTaskQueue
{
public:
	CCAsyncTaskQueue();
	~CCAsyncTaskQueue();

	void Clear();
	int Push(CCAsyncTask* task);
	CCAsyncTask* Pop();

private:
	std::mutex workerQueueLock;
	int workerQueueId = 0;
	CCSimpleQueue<CCAsyncTask> workerQueue;
};


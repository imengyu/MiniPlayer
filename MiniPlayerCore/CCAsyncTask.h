#pragma once
#include <mutex>
#include <string>

class CCAsyncTask
{
public:
	int Id;
	int Command;
	bool UserFree;
	bool ReturnStatus;
	int ReturnErrorCode;
	std::string ReturnErrorMessage;
	void* ReturnData;
};


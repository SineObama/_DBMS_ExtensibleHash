#pragma once

#include "PageManager.h"
#include "Cache.h"

#define P 128
#define MAXPID 10000000

// 按页访问单个数据文件的接口，缓存已封装在内（采用同时更新磁盘文件的方式）
class DataManager
{

public:

    DataManager(const char *);
    ~DataManager();

    Cache *getBlockAndLock();
    Cache *getBlockAndLock(index_t pid);
    Cache *writeBlockAndUnlock(Cache *, index_t pid);
    Cache *writeBlockAndUnlock(Cache *);
    Cache *lock(Cache *);
    Cache *unlock(Cache *);
    Cache *replace();

    std::fstream fs;
    int replace_count;

private:

    DataManager();

    Cache *cache;

};

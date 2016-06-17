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

    //char *getAndLock();
    // 返回缓存中的数据地址
    //char *readAndLock(index_t pid);
    // 按页id写入文件
    //void writeAndUnlock(char *, index_t pid, bool check = true);

    Cache *getBlockAndLock();
    Cache *getBlockAndLock(index_t pid);
    Cache *writeBlockAndUnlock(Cache *, index_t pid);
    Cache *writeBlockAndUnlock(Cache *);
    Cache *lock(Cache *);
    Cache *unlock(Cache *);
    Cache *replace();

    std::fstream fs;

private:

    DataManager();

    // 从磁盘读取页，置换掉缓存的一页，返回这个缓存页。缓存页将被清0以防读到无内容页的情况
    char *readAndReplace(index_t pid);

    Cache *cache;

    //int cur;  // 用于时钟页面算法，表示下一个缓存页的位置

};

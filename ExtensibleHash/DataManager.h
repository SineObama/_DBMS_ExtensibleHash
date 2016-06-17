#pragma once

#include "PageManager.h"
#include "Page.h"

#define P 128

// 按页访问单个数据文件的接口，缓存已封装在内（采用同时更新磁盘文件的方式）
class DataManager
{

public:

    DataManager(const char *);
    ~DataManager();

    Page *getAndLock();
    Page *getAndLock(index_t pid);
    Page *writeAndUnlock(Page *, index_t pid);
    Page *writeAndUnlock(Page *);
    Page *lock(Page *);
    Page *unlock(Page *);
    Page *replace();

    size_t getIOs();
    clock_t getIOtime();
    size_t getReplacements();

private:

    DataManager();

    std::fstream fs;

    Page *cache;

    size_t I, O, replaceCount;
    clock_t Itime, Otime;

};

#pragma once

#include "VLRPUtil.h"
#include "Page.h"

#define P 8

// 按页访问单个数据文件的接口，缓存已封装在内（采用同时更新磁盘文件的方式）
class RandomAccessMemory
{

public:

    // 必须先设置了数据文件才可以获取单例
    static void openfile(const char *);
    static RandomAccessMemory *getInstance();

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

    static bool opened;

    RandomAccessMemory();
    ~RandomAccessMemory();

    static std::fstream fs;

    static Page *cache;

    static size_t I, O, replaceCount;
    static clock_t Itime, Otime;

};

#pragma once

#include "PageManager.h"

#define P 128

// 按页访问单个数据文件的接口，缓存已封装在内（采用同时更新磁盘文件的方式）
class DataManager
{

public:

    DataManager(const char *);
    ~DataManager();

    // 返回缓存中的数据地址
    char *read(index_t pid);
    // 按页id写入文件
    void write(char *, index_t pid, bool check = true);
    // 按页id写入文件
    //int write(index_t pid);

    std::fstream fs;
private:

    DataManager();

    // 从磁盘读取页，置换掉缓存的一页，返回这个缓存页。缓存页将被清0以防读到无内容页的情况
    char *readAndReplace(index_t pid);


    struct Cache {
        char data[L];
        bool valid;
        bool bit;  // 时钟页面算法的检测位
        index_t pid;  // 对应磁盘上的页id
        Cache() { valid = bit = false; }
    } *cache;

    int cur;  // 用于时钟页面算法，表示下一个缓存页的位置

};

#pragma once

#include "VLRPUtil.h"

class Page {
    friend class RandomAccessMemory;
public:
    char data[L];
private:
    bool valid;
    bool bit;  // 时钟页面算法的检测位
    index_t pid;  // 对应磁盘上的页id
    size_t pin;  // 被使用的次数
    Page() { valid = bit = false; pin = 0; }
};

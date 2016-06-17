#pragma once

#define L 8192

typedef unsigned int index_t;

// 提供变长记录页的辅助类
class VLRPUtil
{

public:

    VLRPUtil(char *);
    ~VLRPUtil();

    // 返回值int(0)表示正常，(-1)表示出错
    int insert(const char *);
    int insert(const char *, size_t);
    int get(char *, size_t *, index_t) const;
    size_t getSpace() const;

private:

    char *_data;
    size_t *_n;
    index_t *_end;

    static const size_t size_l = sizeof(size_t);
    static const size_t index_l = sizeof(index_t);
    static const size_t item_l = size_l + index_l;

};

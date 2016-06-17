#pragma once

#include "Vector.h"
#include "myString.h"
#include "DataManager.h"

//#define MOST
#define BUFFER_SIZE 1000  // 保存单条记录

typedef int key_t;

/*	哈希索引文件格式：
    第0页相当于“文件头”，有2个size_t类型数字代表总页数和全局深度
    第1页开始顺序存储目录，目录每一项是2个数字：index_t类型的页id和size_t类型的局部深度*/
class ExtensibleHash
{

public:

    ExtensibleHash(const char *);
    ~ExtensibleHash();

    int insert(std::pair<key_t, char *>);
    Vector<myString> query(key_t);

    const char *check(int maxKey);

private:

    //index_t *index2pid(index_t);

    //const char *checkItem(const char *item, size_t gpages);

    DataManager dm;

    size_t pages, gdept;

    static const size_t index_l = sizeof(index_t);
    static const size_t key_l = sizeof(key_t);
    static const size_t size_l = sizeof(size_t);
    static const size_t item_l = index_l + size_l;
    static size_t N;  // 每一页能存放的目录项数。取2的整数次方
};

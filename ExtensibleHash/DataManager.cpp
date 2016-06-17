#include "stdafx.h"
#include "DataManager.h"

DataManager::DataManager(const char *filename)
{
    // 当文件不存在时创建一个空的
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::ofstream ofs(filename);
        ofs.close();
    }

    // bug修复：windows下不用二进制打开会把\n改成\r\n。但这里出现的好像是把0x0005替换成0x0e05。二进制打开就没问题了。
    fs.open(filename, std::ios::binary | std::ios::in | std::ios::out);
    cache = new Page[P];
    I = O = replaceCount = 0;
    Itime = Otime = 0;
}

DataManager::~DataManager()
{
    delete[] cache;
    fs.close();
}

Page *DataManager::getAndLock() {
    Page *c = replace();
    if (c == NULL)
        return NULL;
    memset(c->data, 0, L);
    return lock(c);
}

Page *DataManager::getAndLock(index_t pid) {
    // 尝试查找内存已缓存的页
    int i;
    for (i = 0; i < P; i++) {
        if (cache[i].pin == 0 && cache[i].valid == true && cache[i].pid == pid) {
            cache[i].bit = true;
            return lock(cache + i);
        }
    }
    // 使用时钟页面替换策略
    Page *c = replace();
    if (c == NULL)
        return NULL;

    c->bit = true;
    memset(c->data, 0, L);
    c->pid = pid;
    c->valid = true;

    clock_t begin = clock();
    fs.seekg(pid * L);
    fs.read(c->data, L);
    fs.clear();
    clock_t end = clock();
    I++;
    Itime += end - begin;
    return lock(c);

}

Page *DataManager::writeAndUnlock(Page *c, index_t pid) {
    if (c->pid != pid) {
        // 若找到即将写入的页id的对应内存，设为无效
        for (size_t i = 0; i < P; i++) {
            if (cache[i].valid && cache[i].pid == pid) {
                cache[i].valid = false;
                break;
            }
        }
        c->pid = pid;
        c->valid = true;
    }
    return writeAndUnlock(c);
}

// 写入磁盘
Page *DataManager::writeAndUnlock(Page *c) {
    size_t pos = c->pid * L;
    if (pos > 0x7fffffff) {
        throw std::exception("输出位置超过2G");
    }
    clock_t begin = clock();
    fs.seekp(pos);
    fs.write(c->data, L);
    fs.clear();
    clock_t end = clock();
    O++;
    Otime += end - begin;
    return unlock(c);
}

Page *DataManager::lock(Page *c) {
    if (c->pin > 0)
        return NULL;
    else {
        c->pin++;
        return c;
    }
}

Page *DataManager::unlock(Page *c) {
    if (c->pin > 0)
        c->pin--;
    return c;
}

Page *DataManager::replace() {
    replaceCount++;
    static size_t cur = 0;
    size_t last = cur;
    bool one = true, fail = false;
    while (1)
    {
        if (cache[cur].pin == 0) {
            if (!cache[cur].valid || cache[cur].bit == false)
                break;
            cache[cur].bit = false;
        }
        cur = (cur + 1) % P;
        if (cur == last) {
            if (one)
                one = false;
            else {
                fail = true;
                break;
            }
        }
    }
    if (fail)
        return NULL;
    Page *rtn = cache + cur;
    cur = (cur + 1) % P;
    return rtn;
}

size_t DataManager::getIOs() {
    return I + O;
}

clock_t DataManager::getIOtime() {
    return Itime + Otime;
}

size_t DataManager::getReplacements() {
    return replaceCount;
}

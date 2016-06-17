#include "stdafx.h"
#include "RandomAccessMemory.h"

bool RandomAccessMemory::opened = false;
std::fstream RandomAccessMemory::fs;
Page *RandomAccessMemory::cache = NULL;
size_t RandomAccessMemory::I = 0, RandomAccessMemory::O = 0, RandomAccessMemory::replaceCount = 0;
clock_t RandomAccessMemory::Itime = 0, RandomAccessMemory::Otime = 0;

void RandomAccessMemory::openfile(const char *filename) {
    if (opened)
        return;
    opened = true;
    // 当文件不存在时创建一个空的
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
        std::ofstream ofs(filename);
        ofs.close();
    }

    // bug修复：windows下不用二进制打开会把\n改成\r\n。但这里出现的好像是把0x0005替换成0x0e05。二进制打开就没问题了。
    fs.open(filename, std::ios::binary | std::ios::in | std::ios::out);
    cache = new Page[P];
}

RandomAccessMemory *RandomAccessMemory::getInstance() {
    if (!opened)
        return NULL;
    static RandomAccessMemory instance;
    return &instance;
}

RandomAccessMemory::RandomAccessMemory() {}

RandomAccessMemory::~RandomAccessMemory()
{
    delete[] cache;
    fs.close();
}

Page *RandomAccessMemory::getAndLock() {
    Page *c = replace();
    if (c == NULL)
        return NULL;
    memset(c->data, 0, L);
    return lock(c);
}

Page *RandomAccessMemory::getAndLock(index_t pid) {
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

Page *RandomAccessMemory::writeAndUnlock(Page *c, index_t pid) {
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
Page *RandomAccessMemory::writeAndUnlock(Page *c) {
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

Page *RandomAccessMemory::lock(Page *c) {
    if (c->pin > 0)
        return NULL;
    else {
        c->pin++;
        return c;
    }
}

Page *RandomAccessMemory::unlock(Page *c) {
    if (c->pin > 0)
        c->pin--;
    return c;
}

Page *RandomAccessMemory::replace() {
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

size_t RandomAccessMemory::getIOs() {
    return I + O;
}

clock_t RandomAccessMemory::getIOtime() {
    return Itime + Otime;
}

size_t RandomAccessMemory::getReplacements() {
    return replaceCount;
}

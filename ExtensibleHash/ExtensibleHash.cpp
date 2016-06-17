#include "stdafx.h"
#include "ExtensibleHash.h"

size_t ExtensibleHash::N = 0;

ExtensibleHash::ExtensibleHash(const char *filename)
{
    // 手动初始化N这个静态“常量”
    if (N == 0) {
        N = L / item_l;
        if (N == 0)
            throw std::exception("defined page-length is too small even for two number!");
        int max = 1 << 30;
        while ((N & max) == 0 && max != 0)
            max >>= 1;
        N = max;
    }

    dm = RandomAccessMemory::getInstance();

    Page *mainPage = dm->getAndLock(0);
    size_t *p = (size_t *)(mainPage->data);
    pages = *p;
    if (pages == 0) {  // 一般代表空文件
        // 写入第0页
        pages = *p = 3;
        gdept = *(p + 1) = 0;  // 好像等于白写
        dm->writeAndUnlock(mainPage);

        // 写入第1页的目录
        Page *gpage = dm->getAndLock(1);
        *(index_t *)(gpage->data) = 2;
        *(size_t *)(gpage->data + index_l) = 0;
        dm->writeAndUnlock(gpage);

        // 写入空白的第2页
        Page *lpage = dm->getAndLock(2);
        dm->writeAndUnlock(lpage);
    }
    else {
        gdept = *(p + 1);
        dm->unlock(mainPage);
    }
}

ExtensibleHash::~ExtensibleHash() {}

int ExtensibleHash::insert(std::pair<key_t, char *> pair) {
    int watch[L / sizeof(int)];  // debug
    // 生成数据记录
    key_t key = pair.first;// wrong after key = 1793
    char *val = pair.second;
    size_t recordLen = strlen(val);
    char *record = new char[key_l + recordLen + 1];
    *(key_t *)record = key;
    memcpy(record + key_l, val, recordLen + 1);

    // 以下变量命名中g前缀代表global，表示目录相关；l代表local，表示数据记录相关；m代表main，表示第0页
    while (true) {
        const size_t gsize = 1 << gdept;
        const index_t index = key % gsize;
        const index_t gpid = index / N + 1;
        Page *gpage = dm->getAndLock(gpid);
        const char *item = gpage->data + index % N * item_l;
        index_t lpid = *(index_t *)item;
        const index_t ldept = *(size_t *)(item + index_l);
        debug_memcpy(watch, gpage->data, L);
        dm->unlock(gpage);

        // 插入
        assert(lpid >= ((gsize / N) | 1));
        Page *lpage = dm->getAndLock(lpid);
        VLRPUtil pm(lpage->data);
        if (pm.insert(record, key_l + recordLen) == 0) {  // 插入成功
            debug_memcpy(watch, lpage->data, L);
            dm->writeAndUnlock(lpage);
            break;
        }
        dm->unlock(lpage);

        // 插入失败（桶满了）
#ifdef MOST
        // todo
        exit(1);
#else
        // 检查是否需要目录加倍
        if (ldept == gdept) {
            if (gsize >= N) {  // 须转移部分数据页，腾出位置放目录
                size_t copy_pages = gsize / N;  // 转移的页数
                index_t begin = copy_pages + 1, end = begin + copy_pages;  // 转移的范围

                // 转移数据页
                size_t off = pages - begin;
                for (index_t i = begin; i < end; i++) {
                    Page *p = dm->getAndLock(i);
                    dm->writeAndUnlock(p, i + off);
                }

                // 修改目录并加倍（复制）
                for (index_t i = 1; i < begin; i++) {
                    Page *p = dm->getAndLock(i);
                    debug_memcpy(watch, p->data, L);
                    char *cur = p->data;
                    for (index_t j = 0; j < N; j++, cur += item_l) {
                        index_t *index = (index_t *)cur;
                        if (*index >= begin && *index < end)
                            *index += off;
                    }
                    dm->writeAndUnlock(p);
                    dm->writeAndUnlock(p, i + copy_pages);
                }
                if (lpid >= begin && lpid < end)
                    lpid += off;

                // 修改第0页数据：增加总页数
                Page *mpage = dm->getAndLock(0);
                *(size_t *)(mpage->data) = (pages += copy_pages);
                dm->writeAndUnlock(mpage);
            }
            else {  // 在第1页内加倍目录
                Page *p = dm->getAndLock(1);
                memcpy(p->data + gsize * item_l, p->data, gsize * item_l);
                dm->writeAndUnlock(p);
            }

            // 修改第0页数据：全局深度+1
            Page *mpage = dm->getAndLock(0);
            *(size_t *)(mpage->data + size_l) = (++gdept);
            dm->writeAndUnlock(mpage);
        }

        // 分裂当前页：
        // todo 未处理同一键值的记录过多过长，一页存不下的问题。将会导致目录无限翻倍

        index_t newpid = pages;

        // 修改目录
        size_t ddept = gdept - ldept, num = 1 << ddept, lowBit = ldept == 0 ? 0 : index % (1 << ldept);
        for (size_t i = 0; i < num; i++) {
            index_t _index = (i << ldept) | lowBit;
            index_t gpid = _index / N + 1;
            Page *gpage = dm->getAndLock(gpid);
            char *item = gpage->data + _index % N * item_l;
            if (i % 2)
                *(index_t *)(item) = newpid;
            (*(size_t *)(item + index_l))++;
            debug_memcpy(watch, gpage->data, L);
            dm->writeAndUnlock(gpage);
        }

        // 获取空的缓存页，存放分裂后的2页
        Page *srcpage = dm->getAndLock(lpid);
        Page *newpage1 = dm->getAndLock();
        Page *newpage2 = dm->getAndLock();

        debug_memcpy(watch, srcpage->data, L);
        VLRPUtil src(srcpage->data), new1(newpage1->data), new2(newpage2->data);
        for (int i = 0;; i++) {
            char item[BUFFER_SIZE] = {};
            size_t length = sizeof(item);
            int state = src.get(item, &length, i);
            if (state == -1)
                break;
            else if (state == -2)
                exit(1);
            int key = *(int *)item;
            if (key & (1 << ldept))
                new2.insert(item, length);
            else
                new1.insert(item, length);
        }
        debug_memcpy(watch, newpage1->data, L);
        debug_memcpy(watch, newpage2->data, L);
        dm->unlock(srcpage);
        dm->writeAndUnlock(newpage1, lpid);
        dm->writeAndUnlock(newpage2, newpid);

        // 修改第0页数据：总页数+1
        Page *mpage = dm->getAndLock(0);
        *(size_t *)(mpage) = (++pages);
        dm->writeAndUnlock(mpage);
#endif // MOST
    }

    delete[] record;
    return 0;
}

size_t ExtensibleHash::getListPages() {
    size_t listPages = (1 << gdept) / N;
    return listPages > 0 ? listPages : 1;
}

size_t ExtensibleHash::getBuckets() {
    return pages - getListPages() - 1;
}

Vector<myString> ExtensibleHash::query(key_t key) {
    const size_t gsize = 1 << gdept;
    const index_t index = key % gsize;
    const index_t gpid = index / N + 1;
    Page *gpage = dm->getAndLock(gpid);
    const char *item = gpage->data + index % N * item_l;
    index_t lpid = *(index_t *)item;
    dm->unlock(gpage);

    Vector<myString> v;
    assert(lpid >= ((gsize / N) | 1));
    Page *lpage = dm->getAndLock(lpid);
    VLRPUtil pm(lpage->data);
    char record[BUFFER_SIZE];
    for (size_t i = 0;; i++) {
        size_t len = sizeof(record);
        if (pm.get(record, &len, i))
            break;
        if (*(key_t *)record != key)
            continue;
        v.insert(record + 4);
    }
    dm->unlock(lpage);
    size_t size = v.size();
    auto ary = v.get();
    if (size)
        for (size_t i = 0; i < size - 1; i++) {
            for (size_t j = i + 1; j < size; j++) {
                key_t key1, key2;
                sscanf_s(ary[i].c_str(), "%*d|%d", &key1);
                sscanf_s(ary[j].c_str(), "%*d|%d", &key2);
                if (key1 > key2) {
                    myString tem = ary[i];
                    ary[i] = ary[j];
                    ary[j] = tem;
                }
            }
        }
    return v;
}

const char *ExtensibleHash::check(int maxKey) {
    // 检查第0页
    Page *mpage = dm->getAndLock(0);
    size_t *mp = (size_t *)(mpage->data);
    size_t _pages = *mp, _gdept = *(mp + 1);
    dm->unlock(mpage);
    if (pages != _pages)
        throw std::exception("总页数同步错误");
    else if (gdept != _gdept)
        throw std::exception("全局深度同步错误");

    // 遍历目录页检查
    static char msg[100] = {};
    size_t gsize = 1 << gdept;
    // todo 没检查局部深度之间的统一性
    size_t gpages = gsize / N;
    size_t inner = N;
    if (gsize >= N) {
        gpages = 1;
        inner = gsize;
    }
    for (index_t i = 1; i < gpages + 1; i++) {
        Page *gpage = dm->getAndLock(i);
        char *cur = gpage->data;
        for (index_t j = 0; j < N; j++, cur += item_l) {

            index_t lpid = *(index_t *)(cur);
            size_t ldept = *(size_t *)(cur + index_l);
            if (lpid <= gpages) {
                sprintf_s(msg, "页id小于目录页：第%d页  第%d个", i, j);
                throw std::exception(msg);
            }
            if (lpid >= pages) {
                sprintf_s(msg, "页id大于总页数：第%d页  第%d个", i, j);
                throw std::exception(msg);
            }
            if (ldept > gdept) {
                sprintf_s(msg, "局部深度错误：第%d页  第%d个", i, j);
                throw std::exception(msg);
            }
            Page *lpage = dm->getAndLock(lpid);
            VLRPUtil pm(lpage->data);
            // todo 可能更深一步检查
            static char msg[100] = {};
            for (size_t k = 0; ; k++) {
                char s[BUFFER_SIZE] = {};
                size_t len = sizeof(s);
                if (pm.get(s, &len, k))
                    break;
                int key = *(int *)(s);
                if (key % gsize != (i - 1) * N + j) {
                    sprintf_s(msg, "第%d个记录键值%d错误，桶为%d。第%d页  第%d个", k, key, (k - 1) * N + j, i, j);
                    throw std::exception(msg);
                }
            }
            dm->unlock(lpage);
        }
        dm->unlock(gpage);
    }
    return NULL;
}

// 从目录获取桶的位置
//index_t *ExtensibleHash::index2pid(index_t index) {
//    index_t pid = index / N + 1;
//    char *page = dm->readAndLock(pid);
//    pid = *((index_t *)(page)+index % N);
//    return &pid;
//}

#include "stdafx.h"
#include "ExtensibleHash.h"

size_t ExtensibleHash::N = 0;

ExtensibleHash::ExtensibleHash(const char *filename) : dm(filename)
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

    Cache *mainPage = dm.getBlockAndLock(0);
    size_t *p = (size_t *)(mainPage->data);
    pages = *p;
    if (pages == 0) {  // 一般代表空文件
        // 写入第0页
        pages = *p = 3;
        gdept = *(p + 1) = 0;  // 好像等于白写
        dm.writeBlockAndUnlock(mainPage);

        // 写入第1页的目录
        Cache *gpage = dm.getBlockAndLock(1);
        *(index_t *)(gpage->data) = 2;
        *(size_t *)(gpage->data + index_l) = 0;
        dm.writeBlockAndUnlock(gpage);

        // 写入空白的第2页
        Cache *lpage = dm.getBlockAndLock(2);
        dm.writeBlockAndUnlock(lpage);
    }
    else {
        gdept = *(p + 1);
        dm.unlock(mainPage);
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

        Cache *gpage = dm.getBlockAndLock(gpid);
        char *item = gpage->data + index % N * item_l;
        index_t lpid = *(index_t *)item;
        const size_t ldept = *(size_t *)(item + index_l);

        memcpy(watch, gpage->data, L);


        dm.unlock(gpage);
        {
            if (lpid < ((gsize / N) | 1)) {
                return -1;
            }
            Cache *lpage = dm.getBlockAndLock(lpid);
            VLRPUtil pm(lpage->data);
            if (pm.insert(record, key_l + recordLen) == 0) {

                //memcpy(watch, lpage->data, L);

                dm.writeBlockAndUnlock(lpage);
                break;
            }
            else {
                dm.unlock(lpage);
            }
        }
        // 插入失败（桶满了）
#ifdef MOST
        // todo
        exit(1);
#else
        if (ldept == gdept) {  // 需要目录加倍
            if (gsize >= N) {  // 须转移部分数据页，腾出位置放目录
                size_t copy_pages = gsize / N;  // 转移的页数
                index_t begin = copy_pages + 1, end = begin + copy_pages;  // 转移的范围

                // 转移数据页
                size_t off = pages - begin;
                for (index_t i = begin; i < end; i++) {
                    Cache *p = dm.getBlockAndLock(i);
                    dm.writeBlockAndUnlock(p, i + off);
                }

                // 修改目录并加倍（复制）
                for (index_t i = 1; i < begin; i++) {
                    Cache *p = dm.getBlockAndLock(i);

                    memcpy(watch, p->data, L);

                    char *cur = p->data;
                    for (index_t j = 0; j < N; j++, cur += item_l) {
                        index_t *index = (index_t *)cur;
                        if (*index > MAXPID) {
                            return -1;
                        }
                        if (*index >= begin && *index < end)
                            *index += off;
                    }
                    dm.writeBlockAndUnlock(p);
                    dm.writeBlockAndUnlock(p, i + copy_pages);
                }
                // bug修复：lpid可能已无效
                if (lpid >= begin && lpid < end)
                    lpid += off;

                // 修改第0页数据：增加总页数
                Cache *mpage = dm.getBlockAndLock(0);
                *(size_t *)(mpage->data) = (pages += copy_pages);
                dm.writeBlockAndUnlock(mpage);
            }
            else {  // 在第1页内部目录加倍（复制）
                Cache *p = dm.getBlockAndLock(1);
                memcpy(p->data + gsize * item_l, p->data, gsize * item_l);
                dm.writeBlockAndUnlock(p);
            }

            // 修改第0页数据：全局深度+1
            Cache *mpage = dm.getBlockAndLock(0);
            *(size_t *)(mpage->data + size_l) = (++gdept);
            dm.writeBlockAndUnlock(mpage);
        }

        // 分裂当前页：

        index_t newpid = pages;

        // 修改目录
        size_t ddept = gdept - ldept, num = 1 << ddept, lowBit = ldept == 0 ? 0 : index % (1 << ldept);
        for (size_t i = 0; i < num; i++) {
            index_t _index = (i << ldept) | lowBit;
            index_t gpid = _index / N + 1;
            Cache *gpage = dm.getBlockAndLock(gpid);
            char *item = gpage->data + _index % N * item_l;
            if (i % 2)
                *(index_t *)(item) = newpid;
            (*(size_t *)(item + index_l))++;

            memcpy(watch, gpage->data, L);

            dm.writeBlockAndUnlock(gpage);
        }
        // bug修复：原本的方法存在逻辑错误：
        //// 增加较小目录项的本地深度
        //index_t index1 = index & ~(1 << ldept);
        //index_t gpid1 = index1 / N + 1;
        //Cache *gpage1 = dm.getBlockAndLock(gpid1);
        //char *item1 = gpage1->data + index1 % N * item_l;
        //(*(size_t *)(item1 + index_l))++;
        //dm.writeBlockAndUnlock(gpage1);
        //// 增加较大目录项的本地深度，并改变页id
        //index_t index2 = index | (1 << ldept);
        //index_t gpid2 = index2 / N + 1;
        //Cache *gpage2 = dm.getBlockAndLock(gpid2);
        //char *item2 = gpage2->data + index2 % N * item_l;
        //*(index_t *)(item2) = newpid;
        //(*(size_t *)(item2 + index_l))++;
        //dm.writeBlockAndUnlock(gpage2);

        // 获取空的缓存页，存放分裂后的2页
        Cache *lpage = dm.getBlockAndLock(lpid);
        Cache *newpage1 = dm.getBlockAndLock();
        Cache *newpage2 = dm.getBlockAndLock();

        memcpy(watch, lpage->data, L);

        VLRPUtil pm(lpage->data), pm1(newpage1->data), pm2(newpage2->data);
        for (int i = 0;; i++) {
            char item[BUFFER_SIZE] = {};
            size_t length = sizeof(item);
            int state = pm.get(item, &length, i);
            if (state == -1)
                break;
            else if (state == -2)
                exit(1);
            int key = *(int *)item;
            if (key & (1 << ldept))
                pm2.insert(item, length);
            else
                pm1.insert(item, length);
        }

        memcpy(watch, newpage1->data, L);
        memcpy(watch, newpage2->data, L);

        dm.unlock(lpage);
        dm.writeBlockAndUnlock(newpage1, lpid);
        dm.writeBlockAndUnlock(newpage2, newpid);

        // 修改第0页数据：总页数+1
        Cache *mpage = dm.getBlockAndLock(0);
        *(size_t *)(mpage) = (++pages);
        dm.writeBlockAndUnlock(mpage);
#endif // MOST
    }

    delete[] record;
    return 0;
}

Vector<myString> ExtensibleHash::query(key_t key) {
    return Vector<myString>();
}

const char *ExtensibleHash::check(int maxKey) {
    // 检查第0页
    Cache *mpage = dm.getBlockAndLock(0);
    size_t *mp = (size_t *)(mpage->data);
    size_t _pages = *mp, _gdept = *(mp + 1);
    dm.unlock(mpage);
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
        Cache *gpage = dm.getBlockAndLock(i);
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
            Cache *lpage = dm.getBlockAndLock(lpid);
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
            dm.unlock(lpage);

            //const char *error = checkItem(cur, gpages, (i * N + j) % gsize);
            //if (error) {
            //	sprintf(msg, "%s：第%d页  第%d个", error, i, j);
            //	return msg;
            //}
        }
        dm.unlock(gpage);
    }
    return NULL;
}

// 从目录获取桶的位置
//index_t *ExtensibleHash::index2pid(index_t index) {
//    index_t pid = index / N + 1;
//    char *page = dm.readAndLock(pid);
//    pid = *((index_t *)(page)+index % N);
//    return &pid;
//}
//
//const char *ExtensibleHash::checkItem(const char *cur, size_t gpages, int _key) {
//}

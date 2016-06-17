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
    cache = new Cache[P];
    //cur = 0;
}

DataManager::~DataManager()
{
    delete[] cache;
    fs.close();
}

//char *DataManager::getAndLock() {
//	return NULL;
//}

//char *DataManager::readAndLock(index_t pid) {
//	bool found = false;
//	int i;
//	for (i = 0; i < P; i++) {
//		if (cache[i].valid == true && cache[i].pid == pid) {
//			cache[i].bit = true;
//			cache[i].pin++;
//			found = true;
//			break;
//		}
//	}
//	// 这三行本来是用于调试的
//	int watch[L / sizeof(int)] = {};
//	fs.seekg(15 * L);
//	// 执行这1行就不会发生文件读取差异，直接导致的错误是VLRPUtil::insert返回-3
//	//fs.read((char *)watch, L);
//	return found ? cache[i].data : readAndReplace(pid);
//}

//void DataManager::writeAndUnlock(char *data, index_t pid, bool check) {
//	int watch[L / sizeof(int)] = {};
//	//fs.seekg(15 * L);
//	//fs.read((char *)watch, L);
//		// 查找对应的内存页设为新的页id，页id原本对应的内存页设为无效
//	for (int i = 0; i < P; i++) {
//		if (cache[i].pid == pid) {
//			cache[i].valid = false;
//		}
//		else if (cache[i].data == data) {
//			cache[i].pid = pid;
//			cache[i].pin--;
//		}
//	}
//	//memcpy(watch, data, L);
//	// 写入磁盘
//	fs.seekp(pid * L);
//	fs.write(data, L);
//	fs.clear();
//
//	//fs.seekg(15*L);
//	//fs.read((char *)watch, L);
//
//}

Cache *DataManager::getBlockAndLock() {
    Cache *c = replace();
    if (c == NULL)
        return NULL;
    memset(c->data, 0, L);
    return lock(c);
}

Cache *DataManager::getBlockAndLock(index_t pid) {
    if (pid > MAXPID) {
        return NULL;
    }
    // 尝试查找内存已缓存的页
    int i;
    for (i = 0; i < P; i++) {
        if (cache[i].pin == 0 && cache[i].valid == true && cache[i].pid == pid) {
            cache[i].bit = true;
            return lock(cache + i);
        }
    }
    // 使用时钟页面替换策略
    Cache *c = replace();
    if (c == NULL)
        return NULL;

    c->bit = true;
    memset(c->data, 0, L);
    c->pid = pid;
    c->valid = true;

    fs.seekg(pid * L);
    fs.read(c->data, L);

    //int watch[L / sizeof(int)] = {};
    //memcpy(watch, c->data, L);

    fs.clear();
    return lock(c);

}

Cache *DataManager::writeBlockAndUnlock(Cache *c, index_t pid) {
    if (pid > MAXPID) {
        return NULL;
    }
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
    return writeBlockAndUnlock(c);
}

// 写入磁盘
Cache *DataManager::writeBlockAndUnlock(Cache *c) {
    if (c->pid > MAXPID) {
        return NULL;
    }
    fs.seekp(c->pid * L);
    //int watch[L / sizeof(int)] = {};
    //memcpy(watch, c->data, L);
    fs.write(c->data, L);
    fs.clear();
    return unlock(c);
}

Cache *DataManager::lock(Cache *c) {
    if (c->pin > 0)
        return NULL;
    else {
        c->pin++;
        return c;
    }
}

Cache *DataManager::unlock(Cache *c) {
    if (c->pin > 0)
        c->pin--;
    return c;
}

Cache *DataManager::replace() {
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
    Cache *rtn = cache + cur;
    cur = (cur + 1) % P;
    return rtn;
}

char *DataManager::readAndReplace(index_t pid) {
    static size_t cur = 0;
    int last = cur;
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
    memset(cache[cur].data, 0, L);
    fs.seekg(pid * L);
    fs.read(cache[cur].data, L);
    fs.clear();
    //int watch[L / sizeof(int)] = {};
    //memcpy(watch, cache[cur].data, L);
    //fs.seekg(15 * L);
    //fs.read((char *)watch, L);
    cache[cur].valid = cache[cur].bit = true;
    cache[cur].pid = pid;
    cache[cur].pin++;
    char *rtn = cache[cur].data;
    cur = (cur + 1) % P;
    return rtn;
}

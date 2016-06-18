// ExtensibleHash.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ExtensibleHash.h"

#define MAXKEY 6000000
#define QUERY_TIMES 100000
//#define QUERY_FROM_FILE

int main(int argc, char **argv) {

    printf("这里是使用%d个内存页，从%s位拓展的哈希索引。\n", P,
#ifdef MOST
        "高"
#else
        "低"
#endif // MOST 
    );

    // 默认路径为当前路径
    char path[BUFFER_SIZE] = "./";
    if (argc > 1)  // 第一个参数默认为路径
        strcpy_s(path, argv[1]);

    const char *dataFilename = "lineitem.tbl",
        *indexFilename = "hashindex.out",
        *inputFilename = "testinput.in",
        *outputFilename = "testoutput.out";
    char fullpath[BUFFER_SIZE] = {};

    // 检查是否存在索引文件
    bool indexExist = true;
    sprintf_s(fullpath, "%s%s", path, indexFilename);
    std::ifstream indexFile(fullpath);
    if (!indexFile.is_open())
        indexExist = false;
    else
        indexFile.close();

    sprintf_s(fullpath, "%s%s", path, indexFilename);
    RandomAccessMemory::openfile(fullpath);
    RandomAccessMemory *RAM = RandomAccessMemory::getInstance();
    ExtensibleHash hash(fullpath);
    char line[BUFFER_SIZE] = {};

    if (!indexExist) {
        sprintf_s(fullpath, "%s%s", path, dataFilename);
        std::ifstream dataFile(fullpath);
        if (!dataFile.is_open()) {
            printf("打开数据文件 \"%s\" 失败\n", dataFilename);
            system("pause");
            return 0;
        }

        int start = clock();
        printf("正在创建索引文件。。。\n");

        while (!dataFile.eof()) {
            dataFile.getline(line, sizeof(line));
            std::stringstream ss(line);
            int orderKey;
            ss >> orderKey;
            int state = ss.rdstate();
            if (state)
                break;
            hash.insert(std::make_pair(orderKey, line));
        }
        dataFile.close();

        int end = clock();
        printf("建立索引耗时： %dms\n", end - start);
        printf("其中IO耗时： %dms，IO总数：%d\n", RAM->getIOtime(), RAM->getIOs());
    }
    else {
        printf("索引文件 \"%s\" 已存在。\n", indexFilename);
    }

    //hash.check(6100000);

    printf("哈希目录页数：%d\n", hash.getListPages());
    printf("哈希桶数：%d\n", hash.getBuckets());

    printf("查询测试开始。\n");
#ifdef QUERY_FROM_FILE
    sprintf_s(fullpath, "%s%s", path, inputFilename);
    std::ifstream inputFile(fullpath);
    if (!inputFile.is_open()) {
        printf("打开输入文件 \"%s\" 失败\n", inputFilename);
        system("pause");
        return 0;
    }
    sprintf_s(fullpath, "%s%s", path, outputFilename);
    std::ofstream outputFile(fullpath);
    size_t count = 0;
    clock_t lastIOtime = hash.dm.getIOtime();
    while (!inputFile.eof()) {
        inputFile.getline(line, sizeof(line));
        key_t key;
        sscanf_s(line, "%d", &key);
        if (key == -1)
            break;
        count++;
        clock_t begin = clock();
        auto vector = hash.query(key);
        clock_t end = clock();
        clock_t curIOtime = hash.dm.getIOtime();
        printf("查询%d完成。耗时%dms，其中IO耗时%dms\n", count, end - begin, curIOtime - lastIOtime);
        lastIOtime = curIOtime;
        auto string = vector.get();
        size_t len = vector.size();
        for (size_t i = 0; i < len; i++) {
            outputFile << string[i].c_str() << "\n";
        }
        outputFile << "-1\n";
    }
    inputFile.close();
    outputFile.close();
#else
    sprintf_s(fullpath, "%s%s", path, outputFilename);
    std::ofstream outputFile(fullpath);
    size_t count = 0;
    clock_t totaltime = 0, IOtime = 0;
    clock_t lastIOtime = RAM->getIOtime();
    srand(time(NULL));
    while (count <= QUERY_TIMES) {
        count++;
        key_t key = ((rand() << 15) + rand()) % MAXKEY;
        clock_t begin = clock();
        auto vector = hash.query(key);
        clock_t end = clock();
        totaltime += end - begin;
        clock_t curIOtime = RAM->getIOtime();
        IOtime += curIOtime - lastIOtime;
        lastIOtime = curIOtime;
        auto string = vector.get();
        size_t len = vector.size();
        for (size_t i = 0; i < len; i++) {
            outputFile << string[i].c_str() << "\n";
        }
        outputFile << "-1\n";
    }
    outputFile.close();
    printf("完成%d个随机查询耗时%dms，其中IO耗时%dms\n", QUERY_TIMES, totaltime, IOtime);
#endif // QUERY_FROM_FILE
    printf("查询测试结束。\n");

    printf("程序正常结束\n");
    system("pause");
    return 0;
}

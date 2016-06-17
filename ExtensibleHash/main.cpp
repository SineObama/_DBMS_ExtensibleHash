// ExtensibleHash.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ExtensibleHash.h"

int main(int argc, char **argv) {

    // 默认路径为当前路径
    char path[BUFFER_SIZE] = "./";
    if (argc > 1)  // 第一个参数默认为路径
        strcpy_s(path, argv[1]);

    const char *dataFilename = "lineitem6.tbl",
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
        printf("其中IO耗时： %dms\n", hash.dm.getIOtime());
    }
    else {
        printf("索引文件 \"%s\" 已存在。\n", indexFilename);
    }

    //hash.check(6100000);

    printf("哈希目录页数：%d\n", hash.getListPages());
    printf("哈希桶数：%d\n", hash.getBuckets());

    printf("查询测试开始。\n");

    sprintf_s(fullpath, "%s%s", path, inputFilename);
    std::ifstream inputFile(fullpath);
    if (!inputFile.is_open()) {
        printf("打开输入文件 \"%s\" 失败\n", inputFilename);
        system("pause");
        return 0;
    }
    sprintf_s(fullpath, "%s%s", path, outputFilename);
    std::ofstream outputFile(fullpath);
    while (!inputFile.eof()) {
        inputFile.getline(line, sizeof(line));
        key_t key;
        sscanf_s(line, "%d", &key);
        if (key == -1)
            break;
        auto vector = hash.query(key);
        auto string = vector.get();
        size_t len = vector.size();
        for (size_t i = 0; i < len; i++) {
            outputFile << string[i].c_str() << "\n";
        }
        outputFile << "-1\n";
    }
    inputFile.close();
    outputFile.close();


    printf("查询测试结束。\n");

    printf("程序正常结束\n");
    system("pause");
    return 0;
}

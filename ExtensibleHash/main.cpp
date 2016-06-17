// ExtensibleHash.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ExtensibleHash.h"

int main(int argc, char **argv) {

    char path[BUFFER_SIZE] = "./";
    if (argc > 1) {
        strcpy_s(path, argv[1]);
    }

    const char *dataFilename = "lineitem5.tbl",
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

    if (!indexExist) {

        sprintf_s(fullpath, "%s%s", path, dataFilename);
        std::ifstream dataFile(fullpath);
        if (!dataFile.is_open()) {
            printf("open .tbl fail\n");
            system("pause");
            return 0;
        }
        char line[BUFFER_SIZE] = {};

        int start = clock();

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
        printf("set up: %dms\n", end - start);
    }
    else {
        printf("索引文件已存在。\n");
    }

    //hash.check(700000);

    // todo: query

    printf("程序正常结束\n");
    system("pause");
    return 0;
}

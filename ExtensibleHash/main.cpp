// ExtensibleHash.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ExtensibleHash.h"

int main() {
    if (0) {
        if (0) {
            std::ofstream hash("test.txt");
            hash.close();
            ExtensibleHash test("test.txt");
        }
        else {
            printf("%d\n", (1 << 31) >> 1);
            //std::stringstream ss;
            //ss << "123\n12345\n";
            //char s[2];
            //ss.getline(s, sizeof(s));
            //printf("%s\n", s);
            //int state = ss.rdstate();
            //ss.getline(s, sizeof(s));
            //printf("%s\n", s);
            //std::fstream fs("test.txt");

            //fs.seekp(10);
            //char data[10] = { '\001', '\0', '\0', '\0' };
            //fs.write(data, 10);
            //int state2 = fs.rdstate();
            //fs.clear();

            //fs.seekg(10);
            //char re[10] = {};
            //fs.read(re, 10);
            //printf("%d\n", *(int *)re);
            //int state = fs.rdstate();
            //fs.clear();

            //fs.close();
        }
        system("pause");
        return 0;
    }

    int start = clock();

    std::ifstream tbl("lineitem.tbl");
    if (!tbl.is_open()) {
        printf("open .tbl fail\n");
        system("pause");
        return 0;
    }

    const char *path = "hashindex.out";
    // 清空索引文件
    std::ofstream hashFile(path);
    hashFile.close();

    ExtensibleHash hash(path);
    char line[BUFFER_SIZE] = {};
    while (!tbl.eof()) {
        tbl.getline(line, sizeof(line));
        std::stringstream ss(line);
        int orderKey;
        ss >> orderKey;
        int state = ss.rdstate();
        if (state)
            break;
        hash.insert(std::make_pair(orderKey, line));
    }
    tbl.close();

    //hash.check(10000);

    int end = clock();
    printf("set up: %dms\n", end - start);

    // todo: query

    printf("程序正常结束\n");
    system("pause");
    return 0;
}

#include "stdafx.h"
#include "PageManager.h"

VLRPUtil::VLRPUtil(char *s)
{
    _data = s;
    _n = (size_t *)(this->_data + L - size_l);
    _end = (index_t *)(this->_data + L - item_l);
    if (*_n > L / item_l)
        throw std::exception("页内槽数简直太大！");
    if (*_end > L)
        throw std::exception("页内尾地址溢出");
}

VLRPUtil::~VLRPUtil() {}

int VLRPUtil::insert(const char *s) {
    return insert(s, strlen(s));
}

int VLRPUtil::insert(const char *s, size_t size) {
    if (size + item_l > getSpace())
        return -1;
    if (size == 0)
        return -2;
    memcpy(_data + *_end, s, size);
    (*_n)++;
    size_t off1 = *_n * item_l + size_l;
    if (off1 <= 0 || off1 > L) {
        return -4;
    }
    *(size_t *)(_data + L - off1) = size;  // 写入新槽的长度
    size_t off2 = (*_n + 1) * item_l;
    if (off2 <= 0 || off2 > L) {
        return -4;
    }
    *(index_t *)(_data + L - off2) = *_end;  // 写入新槽的位置
    *_end += size;
    return 0;
}

int VLRPUtil::get(char *s, size_t *length, index_t slot) const {
    if (slot >= *_n)
        return -1;
    size_t size = *(size_t *)(this->_data + L - (slot + 1) * item_l - size_l);
    if (size >= *length)
        return -2;
    *length = size;
    index_t index = *(index_t *)(this->_data + L - (slot + 2) * item_l);
    memcpy(s, this->_data + index, size);
    s[size] = '\0';
    return 0;
}

size_t VLRPUtil::getSpace() const {
    return L - (*_n + 1) * item_l - *_end;
}

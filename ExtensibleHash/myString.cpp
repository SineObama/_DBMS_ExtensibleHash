#include "stdafx.h"
#include "myString.h"

myString::myString() {
    _size = 0;
    _s = NULL;
}

myString::myString(const char *s)
{
    _size = strlen(s);
    _s = new char[_size + 1];
    memcpy(_s, s, _size + 1);
}

myString::myString(const myString &o) {
    _size = o._size;
    _s = new char[_size + 1];
    memcpy(_s, o._s, _size + 1);
}

myString::myString(const myString &a, const myString &b) {
    _size = a._size + b._size;
    _s = new char[_size + 1];
    memcpy(_s, a._s, a._size);
    memcpy(_s + a._size, b._s, b._size + 1);
}

myString::myString(const char *s, size_t size)
{
    _size = size;
    _s = new char[_size + 1];
    memcpy(_s, s, _size);
    _s[_size] = '\0';
}

myString::~myString()
{
    delete[] _s;
}

myString myString::operator=(const myString &o) {
    if (&o == this)
        return *this;
    _size = o._size;
        delete[] _s;
    _s = new char[_size + 1];
    memcpy(_s, o._s, _size + 1);
    return *this;
}

const char *myString::c_str() {
    return _s;
}

myString operator+(const myString &a, const myString &b) {
    return myString(a, b);
}

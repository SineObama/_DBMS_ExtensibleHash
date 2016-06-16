#pragma once

class myString
{

public:

    myString();
    myString(const char *s);
    myString(const myString &);
    myString(const myString &, const myString &);
    myString(const char *, size_t);
    ~myString();

    myString operator=(const myString &);

    const char *c_str();

private:

    size_t _size;
    char *_s;

};

myString operator+(const myString &, const myString &);

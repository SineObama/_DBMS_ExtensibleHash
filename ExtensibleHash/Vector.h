#pragma once

template<class T>
class Vector
{

public:

    Vector();
    Vector(const Vector<T> &);
    ~Vector();

    int insert(T);
    size_t size();
    T *get();

private:

    size_t _size, _capacity;
    T *array;

};

template<class T>
Vector<T>::Vector()
{
    _size = 0;
    _capacity = 2;
    array = new T[_capacity];
}

template<class T>
Vector<T>::Vector(const Vector<T> &o) {
    _size = o._size;
    _capacity = o._capacity;
    array = new T[_capacity];
    for (int i = 0; i < _size; i++) {
        array[i] = o.array[i];
    }
}

template<class T>
Vector<T>::~Vector()
{
    delete[] array;
}

template<class T>
int Vector<T>::insert(T element) {
    if (_size >= _capacity) {
        _capacity *= 2;
        T *tem = new T[_capacity];
        for (size_t i = 0; i < _size; i++) {
            tem[i] = array[i];
        }
        delete[] array;
        array = tem;
    }
    array[_size++] = element;
    return 0;
}

template<class T>
size_t Vector<T>::size() {
    return _size;
}

template<class T>
T *Vector<T>::get() {
    return array;
}

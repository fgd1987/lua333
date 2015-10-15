
#ifndef _ARRAY_H_
#define _ARRAY_H_

#include "object.h"

class Array : public Object {

public:
    static Array* create() {
        return new Array();
    }
    Array();
    Array(int size);
    virtual ~Array();
    void set(int index, Object *elem);
    void add(Object *elem);
    void remove(int index);
    void remove(Object *object);
    void sort();
    void clear();
    void swap(int index1, int index2);
    void trunc(int count);
private:
    void grow(int size = 0);
private:
    Object **array_;
    int count_;
};


#endif

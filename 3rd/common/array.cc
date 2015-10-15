
#include "array.h"
#include <malloc.h>
#include "log.h"

#define INIT_COUNT 20

Array::Array() {
    array_ = NULL;
    count_ = 0;
}

Array::Array(int size) {
    grow(size);
}

Array::~Array() {
    clear();
    if (array_ != NULL) {
        free(array_);
        array_ = NULL;
        count_ = 0;
    }
}

void Array::clear() {
    for(int i = 0; i < count_; i++) {
        Object *object = array_[i];
        if (object != NULL) {
            object->release();
            array_[i] = NULL;
        }
    }
    count_ = 0;
}

void Array::add(Object* object) {
    set(count_, object);
}

void Array::set(int index, Object *object) {
    if (index < 0) {
        index = count_ + index;
    }
    if (index < 0) {
        LOG_LOG("index(%d) invalid", index);
        return;
    }
    grow();
    if (index >= count_) {
        return;
    }
    Object *last_object = array_[index];
    if (last_object != NULL) {
        last_object->release();
    }
    array_[index] = object;
    object->retain();
    count_++;
}

void Array::grow(int size) {
    if (array_ == NULL) {
        int new_count = size != 0 ? size : INIT_COUNT;
        array_ = (Object **)malloc(sizeof(Object *) * new_count);
        if (array_ == NULL) {
            LOG_ERROR("malloc fail");
            return;
        }
        count_ = new_count;
        memset(array_, 0, count_ * sizeof(Object *));
    } else {
        int new_count = size != 0 ? size : count_ << 1;
        if (new_count <= count_) {
            LOG_ERROR("newcount(%d) count(%d)", new_count, count_);
            return;
        }
        Object **new_array = (Object **)realloc(array_, sizeof(Object *) * new_count);
        if (new_array == NULL) {
            LOG_ERROR("realloc fail");
            return;
        }
        memset(new_array + count_, 0, (new_count - count_) * sizeof(Object *));
        count_ = new_count;
        array_ = new_array;
    }
}


void Array::remove(int index) {
    if (index < 0) {
        index = count_ + index;
    }
    if (index < 0) {
        LOG_LOG("index(%d) invalid", index);
        return;
    }
    if (index >= count_) {
        LOG_LOG("index(%d) invalid", index);
        return;
    }
    Object *last_object = array_[index];
    if (last_object) {
        last_object->release();
        count_--;
        array_[index] = NULL;
    }
}

void Array::remove(Object *object) {
    int index = -1;
    for (int i = 0; i < count_; i++) {
        if (array_[i] == object) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        remove(index);
    }
}

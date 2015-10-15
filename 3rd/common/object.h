
#ifndef _OBJECT_H_
#define _OBJECT_H_
#include "log.h"

class Object {

public:
    static Object* create() {
        return new Object();
    }

    Object() : ref_count_(1) {
        LOG_LOG("Object()");
    }

    virtual ~Object() {
        LOG_LOG("~Object()");
    }

    void retain() {
        ref_count_++;
    }

    void release() {
        ref_count_--;
        LOG_LOG("ref_count_(%d)", ref_count_);
        if (ref_count_ == 0) {
            LOG_LOG("AAA");
            delete this;
        }
    }

    void auto_release() {
    }

private:
    int ref_count_;
};

#endif

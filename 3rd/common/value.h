#ifndef __VALUE_H__
#define __VALUE_H__
#include "object.h"
#include "cstr.h"

typedef unsigned char byte;
typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

#define VALUE_OBJECT 1
#define VALUE_STRING 2
#define VALUE_INT8 3
#define VALUE_UINT8 4
#define VALUE_INT16 5
#define VALUE_UINT16 6
#define VALUE_INT32 7
#define VALUE_UINT32 8
#define VALUE_INT64 9
#define VALUE_UINT64 10

struct Value {
    byte type;
    union {
        Object *object;
        int8 value_int8;
        uint8 value_uint8;
        int16 value_int16;
        uint16 value_uint16;
        int32 value_int32;
        uint32 value_uint32;
        int64 value_int64;
        uint64 value_uint64;
    };
};

#endif

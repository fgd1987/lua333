#ifndef _CSTRING_H_
#define _CSTRING_H_

#include "object.h"
#include <string.h>
#include <stdlib.h>

/*
 *
 *
 */

class String : public Object
{
public:
    virtual ~String();
    String();
    String(const char *str);
    String(const String &other);
    inline operator char *() {return str_;}
    const char *str() {return str_;}
private:
    char* str_;
    int len_;
};

#endif

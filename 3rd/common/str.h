#ifndef _STRING_H_
#define _STRING_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * 
 *  String *str = String::create("hello);
 *  str->release();
 *
 */

class String 
{
public:
    static String* create(const char *str);
public:
    virtual ~String();
    String(const char *str);
    String(const String &other);
    inline operator char *() {return _str;}
    const char *str() {return _str;}
private:
    char* _str;
    int _len;
};

#endif

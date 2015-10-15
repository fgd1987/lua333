#include "str.h"

String* String::create(const char *str)
{
    String *self = new String(str);
    return self;
}

String::String(const String& other) 
{
    if(other._str != NULL) 
    {
       int len = strlen(other._str);
       _str = (char *)malloc(len + 1);
       strcpy(_str, other._str);
       _len = len;
    } else {
        _str = NULL;
        _len = 0;
    }
}

String::String(const char* str)
{
   int len = strlen(str);
   _str = (char *)malloc(len + 1);
   strcpy(_str, str);
   _len = len;
}

String::~String( ){
    if (_str != NULL) {
        free(_str);
        _str = NULL;
    }
    _len = 0;
}


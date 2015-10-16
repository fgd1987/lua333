#include "cstr.h"

String::String() {
    str_ = NULL;
    len_ = 0;
}

String::String(const String& other) 
{
    if(other.str_ != NULL) 
    {
       int len = strlen(other.str_);
       str_ = (char *)malloc(len + 1);
       strcpy(str_, other.str_);
       len_ = len;
    } else {
        str_ = NULL;
        len_ = 0;
    }
}

String::String(const char* str)
{
   int len = strlen(str);
   str_ = (char *)malloc(len + 1);
   strcpy(str_, str);
   len_ = len;
}

String::~String( ){
    if (str_ != NULL) {
        free(str_);
        str_ = NULL;
    }
    len_ = 0;
}


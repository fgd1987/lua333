#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include "utils.h"
#ifndef __APPLE__ 
#include <openssl/md5.h>
#else
#include "md5.h"
#endif
using namespace std;

string dictionary[]={"a","b","c","d","e","f","g",
                     "h","i","j","k","l","m","n",
                     "o","p","q","r","s","t","u",
                     "v","w","x","y","z","0","1",
                     "2","3","4","5","6","7","8",
                     "9","A","B","C","D","E","F",
                     "G","H","I","J","K","L","M",
                     "N","O","P","Q","R","S","T",
                     "U","V","W","X","Y","Z"};


string* shorturl(const char* url, int urllen)
{
    unsigned char md[16];
    char temp[33]={'\0'};
    string md5string;
    MD5((const unsigned char*)url, urllen, md);
    for(int i=0;i<16;i++)
    {   
        sprintf(temp,"%02x",md[i]);
        md5string+=(string)temp;
    }
    string *resurl=new string[4];
    for(int i=0;i<4;i++)
    {   
        string tempStr="";
        string substring =md5string.substr(8*i,8);
        long hexint = 0x3FFFFFFF & strtol(substring.c_str(),NULL,16);
        for(int j=0;j<6;j++)
        {
            long index = 0x0000003D & hexint;
            tempStr+=dictionary[index];
            hexint = hexint >>5;
        }
        resurl[i]=tempStr;
    }
    return resurl;//返回4个可用的shorturl
}


#ifndef _ACTORDB_H_
#define _ACTORDB_H_
//此文件自动生成，不要手动修改
//#include "common.h"
#include <string>
#include <vector>
#include <map>
#include <array>
using namespace std;

class Task {
public:
   Task();
   ~Task();
   int packlen();
   int pack(char *buf, int len);
   int unpack(const char *buf, int len);
public:
   int32 uid;
   int32 taskid;
};

class User {
public:
   User();
   ~User();
   int packlen();
   int pack(char *buf, int len);
   int unpack(const char *buf, int len);
public:
   string uname;
   vector<Task*> task_array;
   int32 uid;
   Task* task;
};
#endif

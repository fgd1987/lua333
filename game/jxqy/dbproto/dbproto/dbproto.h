#include "achieve.pb.h"
#include "friend.pb.h"
#include "player.pb.h"
#include "task.pb.h"
#include "user.pb.h"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor_database.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/compiler/importer.h>
void dbproto_FriendData_del_friends(dbproto::FriendData* self, lua_State* L, int index);
void dbproto_Achieve_del_value(dbproto::Achieve* self, lua_State* L, int index);
void dbproto_AchieveData_del_achieves(dbproto::AchieveData* self, lua_State* L, int index);
void dbproto_TaskData_del_tasks(dbproto::TaskData* self, lua_State* L, int index);
void dbproto_Task_del_value(dbproto::Task* self, lua_State* L, int index);

int dbproto_FriendData_tostring(dbproto::FriendData* self, lua_State* L);
int dbproto_FriendData_parse_from_string(dbproto::FriendData* self, lua_State* L);
int dbproto_UserData_tostring(dbproto::UserData* self, lua_State* L);
int dbproto_UserData_parse_from_string(dbproto::UserData* self, lua_State* L);
int dbproto_PlayerData_tostring(dbproto::PlayerData* self, lua_State* L);
int dbproto_PlayerData_parse_from_string(dbproto::PlayerData* self, lua_State* L);
int dbproto_Achieve_tostring(dbproto::Achieve* self, lua_State* L);
int dbproto_Achieve_parse_from_string(dbproto::Achieve* self, lua_State* L);
int dbproto_AchieveData_tostring(dbproto::AchieveData* self, lua_State* L);
int dbproto_AchieveData_parse_from_string(dbproto::AchieveData* self, lua_State* L);
int dbproto_TaskData_tostring(dbproto::TaskData* self, lua_State* L);
int dbproto_TaskData_parse_from_string(dbproto::TaskData* self, lua_State* L);
int dbproto_Friend_tostring(dbproto::Friend* self, lua_State* L);
int dbproto_Friend_parse_from_string(dbproto::Friend* self, lua_State* L);
int dbproto_Task_tostring(dbproto::Task* self, lua_State* L);
int dbproto_Task_parse_from_string(dbproto::Task* self, lua_State* L);
int dbproto_UserData_Hello_tostring(dbproto::UserData::Hello* self, lua_State* L);
int dbproto_UserData_Hello_parse_from_string(dbproto::UserData::Hello* self, lua_State* L);

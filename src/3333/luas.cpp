 
#include "luas.h"

lua_State *L;

lua_State *luas_open(){
    return L;
}
void luas_set_state(lua_State *_L){
    L = _L;
}

lua_State *luas_state(){
    return L;
}
int luas_init()
{
    L = luaL_newstate();

    luaopen_base(L);
    luaopen_table(L);
	luaL_openlibs(L); /* opens the basic library */
	luaopen_string(L);
	luaopen_math(L);
    return 1;
}

int luas_close()
{
    lua_close(L);

    L = NULL;
    return 1;
}

int luas_reg_lib(const char *libname, luaL_Reg *reglist)
{
	//lua_newtable(L);
	//lua_setglobal(L, libname);
	//lua_getglobal(L, libname);
	//luaL_setfuncs(L, (luaL_Reg*)reglist, 0);
	luaL_register(L, libname, (luaL_Reg*)reglist);

	return 0;
}

int luas_dofile(const char *filepath)
{
    int rt = luaL_dofile(L, filepath);
    if(rt != 0)
    {
        LOG_ERROR("lua file:%s error:%s", filepath, lua_tostring(L, -1));
    }
    return rt;
}



int luas_callfunc(const char *func, const char *format, ...)
{
    va_list   args;
    va_start(args, format);

    int it = luas_vcallfunc(func, format, args);
    va_end(args);
    return it;
}

/*
 * mod.login.Main.hello_world
 */
int luas_pushluafunction(const char *func){
    char *start = (char *)func;
    char *class_name = start;
    char *pfunc = start;
    while(*pfunc != 0){
        if(*pfunc == '.' && class_name == start){
            *pfunc = 0;
            lua_getglobal(L, class_name);
            *pfunc = '.';
            if(lua_isnil(L, -1)){
                return 0;
            }
            class_name = pfunc + 1;
        }else if(*pfunc == '.'){
            *pfunc = 0;
            lua_pushstring(L, class_name);
            lua_gettable(L, -2);
            *pfunc = '.';
            if(lua_isnil(L, -1)){
                return 0;
            }
    	    lua_remove(L, -2);//弹出table
            class_name = pfunc + 1;
        }
        pfunc++;
    }
    if(class_name == start){
        lua_getglobal(L, class_name);
        if(lua_isnil(L, -1)){
            return 0;
        }
    }else{
        lua_pushstring(L, class_name);
        lua_gettable(L, -2);
        if(lua_isnil(L, -1)){
            return 0;
        }
        lua_remove(L, -2);//弹出table
    }
    return 1;     

}
int luas_vcallfunc(const char *func, const char *format, va_list args)
{
    int i, arg_num = 0;
    int top = lua_gettop(L);

    luas_pushluafunction(func);
    int format_len = strlen(format);
    for(i = 0; i < format_len; i++)
    {
        char c = format[i];
        if(c != '%')continue;

        c = format[i+1];
        if(c == 'd')
        {
            int val = va_arg(args,int);
            lua_pushnumber(L, val);
            arg_num++;
        }else if(c == 's')
        {
            char *val = va_arg(args, char*);
            lua_pushstring(L, val);
            arg_num++;
        }
    }
    if (lua_pcall(L, arg_num, 1, 0) != 0)
	{
        LOG_ERROR( "error running function %s: %s\n", func, lua_tostring(L, -1));
        lua_pop(L, lua_gettop(L) - top);
        return 0;
	}else{
        lua_pop(L, lua_gettop(L) - top);
        return 1;
    }
}

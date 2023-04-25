#define TEST_NO_LUA_SETUP
#include "test.h"
#include <stdlib.h>
#include <string.h>

static const char* support_functions =
"function Assert(cond, func)\n"
"    if cond == false then\n"
"        if func ~= nil then\n"
"            func()\n"
"        end\n"
"        assert(cond)\n"
"    end\n"
"end\n"
"\n"
"function CompareValue(v1, v2)\n"
"    if type(v1) ~= type(v2) then\n"
"        return false\n"
"    end\n"
"    if type(v1) ~= \"table\" then\n"
"        return v1 == v2\n"
"    end\n"
"    for k,v in pairs(v1) do\n"
"        if v2[k] ~= v then\n"
"            return false\n"
"        end\n"
"    end\n"
"    for k,v in pairs(v2) do\n"
"        if v1[k] ~= v then\n"
"            return false\n"
"        end\n"
"    end\n"
"    return true\n"
"end\n"
"\n";

test_ctx_t G = { NULL };

static int _msg_handler(lua_State* L)
{
    const char* msg = lua_tostring(L, 1);
    if (msg == NULL)
    {  /* is error object not a string? */
        if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
            lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
        {
            return 1;  /* that is the message */
        }
        else
        {
            msg = lua_pushfstring(L, "(error object is a %s value)",
                luaL_typename(L, 1));
        }
    }
    luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
    return 1;  /* return the traceback */
}

static int test_dostring_ex(lua_State* L, const char* name, const char* script)
{
    int ret = LUA_OK;
    int sp = lua_gettop(L);
    lua_pushcfunction(L, _msg_handler); // sp + 1

    size_t script_sz = strlen(script);
    if ((ret = luaL_loadbuffer(L, script, script_sz, name)) != LUA_OK)
    {
        return ret;
    }

    if ((ret = lua_pcall(L, 0, LUA_MULTRET, sp + 1)) != LUA_OK)
    {
        return ret;
    }

    return ret;
}

static const char* _filename(const char* file)
{
    const char* pos = file;

    if (file == NULL)
    {
        return NULL;
    }

    for (; *file; ++file)
    {
        if (*file == '\\' || *file == '/')
        {
            pos = file + 1;
        }
    }
    return pos;
}

int test_dostring(lua_State* L, const char* script)
{
    return test_dostring_ex(L, script, script);
}

void test_lua_setup(void)
{
    G.L = luaL_newstate();
    ASSERT_NE_PTR(G.L, NULL);

    luaL_openlibs(G.L);

    ASSERT_EQ_INT(luaopen_cjson(G.L), 1);
    lua_setglobal(G.L, "cjson");

    ASSERT_EQ_INT(test_dostring_ex(G.L, _filename(__FILE__), support_functions), LUA_OK,
        "%s", lua_tostring(G.L, -1));
}

void test_lua_teardown(void)
{
    lua_close(G.L);
    G.L = NULL;
}

int main(int argc, char* argv[])
{
    return cutest_run_tests(argc, argv, stdout, NULL);
}

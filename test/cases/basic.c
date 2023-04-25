#include "test.h"

TEST_F(lua, cjson_table)
{
    static const char* script =
"local t = {\n"
"    a = 1,\n"
"    b = \"hello\","
"}\n"
"t[0] = 3\n"
"local ret = cjson.serialize(t)\n"
"local pat = cjson.deserialize(ret)\n"
"Assert(CompareValue(t, pat) == true, function()\n"
"    io.write(\"ret:\" .. ret .. \"\\n\")\n"
"end)\n";

    ASSERT_EQ_INT(test_dostring(G.L, script), LUA_OK,
        "%s", lua_tostring(G.L, -1));
}

TEST_F(lua, cjson_double)
{
    static const char* script =
"local dat = cjson.serialize(1.2)\n"
"local pat = \"{\\\"v\\\":1.2}\"\n"
"Assert(cjson.compare(dat, pat), function()\n"
"    io.write(\"dat:\" .. dat .. \"\\n\")\n"
"    io.write(\"pat:\" .. pat .. \"\\n\")\n"
"end)\n";

    ASSERT_EQ_INT(test_dostring(G.L, script), LUA_OK,
        "%s", lua_tostring(G.L, -1));
}

TEST_F(lua, tojson_userdata_no_metamethod)
{
    static const char* script =
"local ret, dat = pcall(cjson.tojson, test_v)\n"
"assert(ret == false)\n";

    int* v = (int*)lua_newuserdata(G.L, sizeof(int));
    *v = 99;

    static luaL_Reg s_meta[] = {
        { NULL, NULL },
    };
    if (luaL_newmetatable(G.L, "__test_type") != 0)
    {
        luaL_setfuncs(G.L, s_meta, 0);
    }
    lua_setmetatable(G.L, -2);
    lua_setglobal(G.L, "test_v");

    ASSERT_EQ_INT(test_dostring(G.L, script), LUA_OK,
        "%s", lua_tostring(G.L, -1));
}

static int _test_basic_userdata_tostring(lua_State* L)
{
    int* v = lua_touserdata(L, 1);
    lua_pushfstring(L, "%d", *v);
    return 1;
}

TEST_F(lua, tojson_userdata)
{
    static const char* script =
"local dat = cjson.serialize(test_v)\n"
"local pat = \"{\\\"v\\\":{\\\"t\\\":\\\"__test_type\\\",\\\"p\\\":\\\"99\\\"}}\"\n"
"Assert(cjson.compare(dat, pat), function()\n"
"    io.write(\"dat:\" .. dat .. \"\\n\")\n"
"    io.write(\"pat:\" .. pat .. \"\\n\")\n"
"end)\n";

    int* v = (int*)lua_newuserdata(G.L, sizeof(int));
    *v = 99;

    static luaL_Reg s_meta[] = {
        { "__tostring", _test_basic_userdata_tostring },
        { NULL,         NULL },
    };
    if (luaL_newmetatable(G.L, "__test_type") != 0)
    {
        luaL_setfuncs(G.L, s_meta, 0);
    }
    lua_setmetatable(G.L, -2);
    lua_setglobal(G.L, "test_v");

    ASSERT_EQ_INT(test_dostring(G.L, script), LUA_OK,
        "%s", lua_tostring(G.L, -1));
}

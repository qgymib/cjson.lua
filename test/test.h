#ifndef __TEST_H__
#define __TEST_H__

#include "cutest.h"
#include "cjson.lua.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

typedef struct test_ctx
{
    /**
     * @brief Lua VM.
     */
    lua_State* L;
} test_ctx_t;

/**
 * @brief Global test runtime.
 */
extern test_ctx_t G;

/**
 * @brief Setup Lua test environment.
 */
void test_lua_setup(void);

/**
 * @brief Teardown Lua test environment.
 */
void test_lua_teardown(void);

/**
 * @brief Like luaL_dostring(), but with traceback.
 */
int test_dostring(lua_State* L, const char* script);

#if !defined(TEST_NO_LUA_SETUP)

TEST_FIXTURE_SETUP(lua)
{
    test_lua_setup();
}

TEST_FIXTURE_TEARDOWN(lua)
{
    test_lua_teardown();
}

#endif

#ifdef __cplusplus
}
#endif
#endif

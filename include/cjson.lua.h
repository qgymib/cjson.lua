#ifndef __CJSON_LUA_H__
#define __CJSON_LUA_H__
#ifdef __cplusplus
extern "C" {
#endif

struct lua_State;

/**
 * @brief Load cJSON package.
 * @param[in] L     Lua Stack.
 * @return          Always 1.
 */
int luaopen_cjson(struct lua_State* L);

/**
 * @brief Convert Lua value into json string.
 * @param[in] L     Lua stack.
 * @param[in] idx   Value index.
 * @return          Always 1.
 */
int lua_cjson_serialize(struct lua_State* L, int idx);

/**
 * @brief Convert json into Lua value.
 * @param[in] L     Lua stack.
 * @param[in] idx   Value index.
 * @return          Always 1.
 */
int lua_cjson_deserialize(struct lua_State* L, int idx);

#ifdef __cplusplus
}
#endif
#endif

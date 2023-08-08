#include "cjson.lua.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "cjson/cJSON.h"

static int _lua_cjson_serialize(lua_State* L, int idx, cJSON* obj, const char* k);
static int _lua_cjson_deserialize(lua_State* L, cJSON* v);
static int _cjson_from_json_array(lua_State* L, int idx, cJSON* obj);

typedef struct lua_cjson
{
    cJSON* obj;
} lua_cjson_t;

static int _cjson_gc(lua_State* L)
{
    lua_cjson_t* self = lua_touserdata(L, 1);

    if (self->obj != NULL)
    {
        cJSON_Delete(self->obj);
        self->obj = NULL;
    }

    return 0;
}

static lua_cjson_t* _cjson_make_json(lua_State* L)
{
    lua_cjson_t* obj = lua_newuserdata(L, sizeof(lua_cjson_t));
    obj->obj = NULL;

    static const luaL_Reg s_meta[] = {
        { "__gc",   _cjson_gc },
        { NULL,     NULL },
    };
    if (luaL_newmetatable(L, "__cjson") != 0)
    {
        luaL_setfuncs(L, s_meta, 0);
    }
    lua_setmetatable(L, -2);

    obj->obj = cJSON_CreateObject();

    return obj;
}

static int _cjson_serialize(lua_State* L)
{
    return lua_cjson_serialize(L, 1);
}

static int _cjson_deserialize(lua_State* L)
{
    return lua_cjson_deserialize(L, 1);
}

static int _cjson_tostring_table(lua_State* L, int idx, cJSON* obj, const char* k)
{
    int sp = lua_gettop(L);

    cJSON* arr = cJSON_CreateArray();
    cJSON_AddItemToObject(obj, k, arr);

    lua_pushnil(L); // sp + 1
    while (lua_next(L, idx) != 0)
    {// key at sp+1, value at sp+2
        cJSON* ele = cJSON_CreateObject();
        cJSON_AddItemToArray(arr, ele);

        _lua_cjson_serialize(L, sp + 1, ele, "k");
        _lua_cjson_serialize(L, sp + 2, ele, "v");

        lua_pop(L, 1);
    }

    return 0;
}

static int _cjson_tostring_native(lua_State* L, int idx, cJSON* obj, const char* k)
{
    int type;

    switch (type = lua_type(L, idx))
    {
    case LUA_TNIL:
        cJSON_AddNullToObject(obj, k);
        return 0;

    case LUA_TBOOLEAN:
        cJSON_AddBoolToObject(obj, k, lua_toboolean(L, idx));
        return 0;

    case LUA_TNUMBER:
        cJSON_AddNumberToObject(obj, k, lua_tonumber(L, idx));
        return 0;

    case LUA_TSTRING:
        cJSON_AddStringToObject(obj, k, lua_tostring(L, idx));
        return 0;

    case LUA_TTABLE:
        return _cjson_tostring_table(L, idx, obj, k);

    case LUA_TUSERDATA:
    case LUA_TLIGHTUSERDATA:
        if (lua_touserdata(L, idx) == NULL)
        {
            cJSON_AddNullToObject(obj, k);
            return 0;
        }
        lua_pushfstring(L, "userdata does not contains `__tostring` metamethod.");
        return -1;

    case LUA_TFUNCTION:
    case LUA_TTHREAD:
    default:
        lua_pushfstring(L, "type %s does not support serializable.", lua_typename(L, idx));
        return -1;
    }
}

static int _lua_cjson_serialize(lua_State* L, int idx, cJSON* obj, const char* k)
{
    int ret;
    int sp = lua_gettop(L);

    if ((ret = luaL_getmetafield(L, idx, "__name")) == LUA_TSTRING) // sp + 1
    {
        if ((ret = luaL_getmetafield(L, idx, "__tostring")) != LUA_TFUNCTION) // sp + 2
        {
            lua_pushfstring(L, "type `%s` does not have `__tostring` meta method.", lua_tostring(L, sp + 1));
            return -1;
        }

        lua_pushvalue(L, idx); // sp + 3
        lua_call(L, 1, 1); // sp + 2

        cJSON* v = cJSON_CreateObject();
        cJSON_AddStringToObject(v, "t", lua_tostring(L, sp + 1));
        cJSON_AddStringToObject(v, "p", lua_tostring(L, sp + 2));
        cJSON_AddItemToObject(obj, k, v);

        lua_settop(L, sp);
        return 0;
    }
    lua_settop(L, sp);

    return _cjson_tostring_native(L, idx, obj, k);
}

static int _lua_cjson_fromjson_table(lua_State* L, cJSON* obj)
{
    cJSON* ele;
    lua_newtable(L);

    cJSON_ArrayForEach(ele, obj)
    {
        cJSON* k = cJSON_GetObjectItem(ele, "k");
        if (_lua_cjson_deserialize(L, k) != 0)
        {
            return -1;
        }

        cJSON* v = cJSON_GetObjectItem(ele, "v");
        if (_lua_cjson_deserialize(L, v) != 0)
        {
            return -1;
        }

        lua_settable(L, -3);
    }

    return 0;
}

static int _lua_cjson_fromjson_userdata(lua_State* L, cJSON* obj)
{
    int sp = lua_gettop(L);
    cJSON* t = cJSON_GetObjectItem(obj, "t");
    cJSON* p = cJSON_GetObjectItem(obj, "p");

    if (luaL_getmetatable(L, cJSON_GetStringValue(t)) != LUA_TTABLE) // sp + 1
    {
        lua_pushfstring(L, "no metatable named `%s`", cJSON_GetStringValue(t));
        return -1;
    }
    if (lua_getfield(L, sp + 1, "__fromstring") != LUA_TFUNCTION) // sp + 2
    {
        lua_pushfstring(L, "no metamethod `__fromstring` in metatable `%s`", cJSON_GetStringValue(t));
        return -1;
    }

    lua_pushstring(L, cJSON_GetStringValue(p)); // sp + 3
    lua_call(L, 1, 1); // sp + 2

    lua_remove(L, sp + 1); // sp + 1
    return 0;
}

static int _lua_cjson_deserialize(lua_State* L, cJSON* v)
{
    switch (v->type)
    {
    case cJSON_False:
        lua_pushboolean(L, 0);
        break;

    case cJSON_True:
        lua_pushboolean(L, 1);
        break;

    case cJSON_NULL:
        lua_pushnil(L);
        break;

    case cJSON_Number:
        lua_pushnumber(L, cJSON_GetNumberValue(v));
        break;

    case cJSON_String:
        lua_pushstring(L, cJSON_GetStringValue(v));
        break;

    case cJSON_Array:
        return _lua_cjson_fromjson_table(L, v);

    case cJSON_Object:
        return _lua_cjson_fromjson_userdata(L, v);

    default:
        lua_pushfstring(L, "unknown JSON type %d", v->type);
        return -1;
    }

    return 0;
}

static int _cjson_compare(lua_State* L)
{
    cJSON* v1_json = NULL;
    cJSON* v2_json = NULL;
    const char* err_pos = NULL;

    size_t v1_sz = 0;
    const char* v1 = luaL_checklstring(L, 1, &v1_sz);

    size_t v2_sz = 0;
    const char* v2 = luaL_checklstring(L, 2, &v2_sz);

    if ((v1_json = cJSON_ParseWithLengthOpts(v1, v1_sz, &err_pos, 0)) == NULL)
    {
        lua_pushfstring(L, "parser argument #1 as json failed at `%s`", err_pos);
        goto error;
    }

    if ((v2_json = cJSON_ParseWithLengthOpts(v2, v2_sz, &err_pos, 0)) == NULL)
    {
		lua_pushfstring(L, "parser argument #2 as json failed at `%s`", err_pos);
		goto error;
    }

    int ret = cJSON_Compare(v1_json, v2_json, 1);
    cJSON_Delete(v1_json);
    cJSON_Delete(v2_json);

    lua_pushboolean(L, ret);
    return 1;

error:
    if (v1_json != NULL)
    {
        cJSON_Delete(v1_json);
        v1_json = NULL;
    }
    if (v2_json != NULL)
    {
        cJSON_Delete(v2_json);
        v2_json = NULL;
    }
    return lua_error(L);
}

static int _cjson_from_json_object(lua_State* L, int idx, cJSON* obj)
{
    cJSON* ele;
    cJSON_ArrayForEach(ele, obj)
    {
        lua_pushstring(L, ele->string);

		if (cJSON_IsBool(ele))
		{
			lua_pushboolean(L, ele->valueint);
		}
		else if (cJSON_IsNull(ele))
		{
			lua_pushlightuserdata(L, NULL);
		}
		else if (cJSON_IsNumber(ele))
		{
			lua_pushnumber(L, ele->valuedouble);
		}
		else if (cJSON_IsString(ele))
		{
			lua_pushstring(L, ele->valuestring);
		}
		else if (cJSON_IsArray(ele))
		{
			int sp = lua_gettop(L);
			lua_newtable(L);
			_cjson_from_json_array(L, sp + 1, ele);
		}
		else if (cJSON_IsObject(ele) || cJSON_IsRaw(ele))
		{
			int sp = lua_gettop(L);
			lua_newtable(L);
			_cjson_from_json_object(L, sp + 1, ele);
		}
		else
		{
			return luaL_error(L, "invalid json object type %d.", ele->type);
		}

		lua_settable(L, idx);
    }

    return 0;
}

static int _cjson_from_json_array(lua_State* L, int idx, cJSON* obj)
{
	cJSON* ele;
    size_t pos = 1;
	cJSON_ArrayForEach(ele, obj)
	{
        lua_pushinteger(L, pos);

        if (cJSON_IsBool(ele))
        {
            lua_pushboolean(L, ele->valueint);
        }
        else if (cJSON_IsNull(ele))
        {
            lua_pushlightuserdata(L, NULL);
        }
        else if (cJSON_IsNumber(ele))
        {
            lua_pushnumber(L, ele->valuedouble);
        }
        else if (cJSON_IsString(ele))
        {
            lua_pushstring(L, ele->valuestring);
        }
        else if (cJSON_IsArray(ele))
        {
            int sp = lua_gettop(L);
            lua_newtable(L);
            _cjson_from_json_array(L, sp + 1, ele);
        }
        else if (cJSON_IsObject(ele) || cJSON_IsRaw(ele))
        {
			int sp = lua_gettop(L);
			lua_newtable(L);
            _cjson_from_json_object(L, sp + 1, ele);
        }
        else
        {
            return luaL_error(L, "invalid json object type %d.", ele->type);
        }

        lua_settable(L, idx);
        pos++;
	}

    return 0;
}

static int _cjson_from_json(lua_State* L)
{
    int sp = lua_gettop(L);

    size_t data_sz = 0;
    const char* data = luaL_checklstring(L, 1, &data_sz);

    cJSON* object = cJSON_ParseWithLength(data, data_sz);
    if (object == NULL)
    {
        return 0;
    }

    lua_newtable(L); //sp+1
    if (cJSON_IsObject(object))
    {
        _cjson_from_json_object(L, sp + 1, object);
    }
    else if (cJSON_IsArray(object))
    {
        _cjson_from_json_array(L, sp + 1, object);
    }
    else
    {
        return luaL_error(L, "invalid json object type %d.", object->type);
    }

    cJSON_Delete(object);

    return 1;
}

int luaopen_cjson(lua_State* L)
{
    static const luaL_Reg s_apis[] = {
        { "from_json",      _cjson_from_json },
        { "serialize",      _cjson_serialize },
        { "deserialize",    _cjson_deserialize },
        { "compare",        _cjson_compare },
        { NULL,             NULL },
    };
    luaL_newlib(L, s_apis);

    lua_pushlightuserdata(L, NULL);
    lua_setfield(L, -2, "NULL");

    return 1;
}

int lua_cjson_serialize(lua_State* L, int idx)
{
    lua_cjson_t* obj = _cjson_make_json(L); // sp + 1

    if (_lua_cjson_serialize(L, idx, obj->obj, "v") != 0)
    {
        return lua_error(L);
    }

    char* str = cJSON_PrintUnformatted(obj->obj);
    lua_pop(L, 1); // sp

    lua_pushstring(L, str);
    cJSON_free(str);

    return 1;
}

int lua_cjson_deserialize(lua_State* L, int idx)
{
    size_t data_sz = 0;
    const char* data = luaL_checklstring(L, idx, &data_sz);

    int sp = lua_gettop(L);

    lua_cjson_t* obj = _cjson_make_json(L); // sp + 1
    cJSON_Delete(obj->obj); obj->obj = NULL;

    char* return_parse_end = NULL;
    obj->obj = cJSON_ParseWithLengthOpts(data, data_sz, &return_parse_end, 0);

    if (obj->obj == NULL)
    {
        lua_pop(L, 1); // sp
        lua_pushfstring(L, "JSON parser failed at `%s`", return_parse_end); // sp + 1
        return lua_error(L);
    }

    cJSON* v = cJSON_GetObjectItem(obj->obj, "v");
    int ret = _lua_cjson_deserialize(L, v); // sp + 2
    lua_remove(L, sp + 1);

    if (ret != 0)
    {
        return lua_error(L);
    }

    return 1;
}

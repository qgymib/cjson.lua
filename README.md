# cjson.lua

A Lua wrapper for [cJSON](https://github.com/DaveGamble/cJSON).

## API

### cjson.serialize

```lua
string cjson.serialize(object)
```

Serialize a Lua value into JSON string, which can be deserialized by `cjson.deserialize()` later.

Convert rule in order:
1. A Lua value will always have a key `v` as it's top level key.
   ```
   { "v": "example" }
   ```
2. If a value have metatable field `__name`, and metamethod `__tostring`, the
   value of `__name` and return value of `__tostring` will be the json value.
   ```
   { "v": { "t": "typename", "p": "payload" } }
   ```
2. If it is a Lua native type (nil, boolean, number, string), the value is
   the equal JSON value:
   ```
   { "v": null }
   { "v": true }
   { "v": 1.2 }
   { "v": "hello world" }
   ```
3. If it is a userdata (at this stage, it does not have metatable), and it
   is equal to C value `NULL`, treat it as JSON null.
   ```
   { "v": null }
   ```
4. If it is a table, expand it as JSON array. The content in array is a
   series of JSON object that have `k` as table key and `v` as table value.
   The value of `k` and `v` follows the same convert rule.
   ```
   { "v": [ { "k": 0, "v": 3 }, { "k": "b", "v": { "t": "typename", "p": "payload"} } ] }
   ```
5. For other conditions raise an error.

### cjson.deserialize

```lua
object cjson.deserialize(string)
```

Deserialize a JSON string into Lua value.

Convert rule in order:

1. A Lua value will always have a key `v` as it's top level key.
2. If the JSON value is `null`, `boolean`, `number` or `string`, convert it into equal Lua value.
3. If the JSON value is array, treat it as Lua table.
4. If the JSON value is object, treat it as userdata. The `t` specify the type name of userdata.
   A metamethod `__fromstring` will be called with `payload` as it's argument. The return value
   will be treat it as Lua value.

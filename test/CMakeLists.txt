add_executable(cjson.lua.test
    "cases/basic.c"
    "main.c")

target_include_directories(cjson.lua.test
    PRIVATE
        ${LUA_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(cjson.lua.test
    PRIVATE
        lcjson
        cutest)

setup_target_wall(cjson.lua.test)

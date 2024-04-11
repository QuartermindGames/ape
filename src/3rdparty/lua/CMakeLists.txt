add_library(lua STATIC
        lapi.c
        lauxlib.c
        lbaselib.c
        lcode.c
        lcorolib.c
        lctype.c
        ldblib.c
        ldebug.c
        ldo.c
        ldump.c
        lfunc.c
        lgc.c
        linit.c
        liolib.c
        llex.c
        lmathlib.c
        lmem.c
        loadlib.c
        lobject.c
        lopcodes.c
        loslib.c
        lparser.c
        lstate.c
        lstring.c
        lstrlib.c
        ltable.c
        ltablib.c
        ltm.c
        lua.c
        luac.c
        lundump.c
        lutf8lib.c
        lvm.c
        lzio.c
)

target_include_directories(lua PUBLIC include)
target_compile_definitions(lua PRIVATE LUA_USE_LINUX)

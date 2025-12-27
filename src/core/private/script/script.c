// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Scripting interface implementation.
// Author:  Mark E. Sowden

#include "3rdparty/lua/include/lua.h"
#include "3rdparty/lua/include/lauxlib.h"
#include "3rdparty/lua/include/lualib.h"

#include "qmos/public/qm_os_memory.h"

#include "ape_private.h"

#include "script.h"

#include "3rdparty/lua/ldebug.h"

static lua_State *scriptLuaState;

static void *script_alloc( void *, void *ptr, size_t, const size_t nsize )
{
	if ( nsize == 0 )
	{
		qm_os_memory_free( ptr );
		return nullptr;
	}

	return qm_os_memory_realloc( ptr, nsize );
}

static void strings_to_buf( lua_State *state, luaL_Buffer *dst )
{
	luaL_buffinit( state, dst );

	int n = lua_gettop( state );
	for ( int i = 1; i <= n; ++i )
	{
		if ( !lua_isstring( state, i ) && !lua_isnumber( state, i ) )
		{
			continue;
		}

		size_t      len;
		const char *str = luaL_tolstring( state, i, &len );

		luaL_addlstring( dst, str, len );

		lua_pop( state, 1 );
	}
}

static int core_print( void *ptr )
{
	lua_State *state = ptr;

	luaL_Buffer buffer;
	strings_to_buf( state, &buffer );

	luaL_pushresult( &buffer );

	const char *output = lua_tostring( state, -1 );
	if ( output != nullptr )
	{
		ape_console_print_( "%s\n", output );
	}

	lua_pop( state, 1 );

	return 0;
}

static int core_warning( void *ptr )
{
	lua_State *state = ptr;

	luaL_Buffer buffer;
	strings_to_buf( state, &buffer );

	luaL_pushresult( &buffer );

	const char *output = lua_tostring( state, -1 );
	if ( output != nullptr )
	{
		ape_console_warning_( "%s\n", output );
	}

	lua_pop( state, 1 );

	return 0;
}

static int core_error( void *ptr )
{
	lua_State *state = ptr;

	bool shouldDie = lua_toboolean( state, 1 );

	luaL_Buffer buffer;
	strings_to_buf( state, &buffer );

	luaL_pushresult( &buffer );

	const char *output = lua_tostring( state, -1 );
	if ( output != nullptr )
	{
		ape_console_error_( shouldDie, "%s\n", output );
	}

	lua_pop( state, 1 );

	return 0;
}

static const ApeScriptLuaExport scriptCoreExports[] = {
        APE_SCRIPT_REGISTER_FUNC( "print", core_print ),
        APE_SCRIPT_REGISTER_FUNC( "warning", core_warning ),
        APE_SCRIPT_REGISTER_FUNC( "error", core_error ),

        APE_SCRIPT_REGISTER_CONST_INT( "VERSION_MAJOR", VERSION_MAJOR ),
        APE_SCRIPT_REGISTER_CONST_INT( "VERSION_MINOR", VERSION_MINOR ),
        APE_SCRIPT_REGISTER_CONST_INT( "VERSION_PATCH", VERSION_PATCH ),
        APE_SCRIPT_REGISTER_CONST_STRING( "VERSION_CODENAME", VERSION_CODENAME ),
};

static const ApeScriptLuaInterface scriptCoreInterface = {
        .name       = "core",
        .exports    = scriptCoreExports,
        .numExports = QM_OS_ARRAY_ELEMENTS( scriptCoreExports ),
};

void ape_script_manager_register_interface( const ApeScriptLuaInterface *interface )
{
	lua_newtable( scriptLuaState );
	for ( unsigned int i = 0; i < interface->numExports; ++i )
	{
		switch ( interface->exports[ i ].type )
		{
			default:
				ape_console_warning_( "Unknown script export type (%u)!\n", interface->exports[ i ].type );
				break;
			case APE_SCRIPT_LUA_EXPORT_TYPE_FUNC:
				lua_pushcfunction( scriptLuaState, ( int ( * )( lua_State * ) ) interface->exports[ i ].func );
				lua_setfield( scriptLuaState, -2, interface->exports[ i ].name );
				break;
			case APE_SCRIPT_LUA_EXPORT_TYPE_CONST:
			{
				if ( interface->exports[ i ].constant.type == APE_SCRIPT_LUA_DATA_TYPE_INT )
				{
					lua_pushinteger( scriptLuaState, interface->exports[ i ].constant.data.vi );
				}
				else if ( interface->exports[ i ].constant.type == APE_SCRIPT_LUA_DATA_TYPE_FLOAT )
				{
					lua_pushnumber( scriptLuaState, interface->exports[ i ].constant.data.vf );
				}
				else if ( interface->exports[ i ].constant.type == APE_SCRIPT_LUA_DATA_TYPE_STRING )
				{
					lua_pushstring( scriptLuaState, interface->exports[ i ].constant.data.vs );
				}
				lua_setfield( scriptLuaState, -2, interface->exports[ i ].name );
				break;
			}
		}
	}

	lua_setglobal( scriptLuaState, interface->name );
}

void ape_script_manager_initialize_()
{
	assert( scriptLuaState == nullptr );

	scriptLuaState = lua_newstate( script_alloc, nullptr );
	if ( scriptLuaState == nullptr )
	{
		ape_console_error_( true, "Failed to initialize script interface!\n" );
	}

	lua_newtable( scriptLuaState );

	ape_script_manager_register_interface( &scriptCoreInterface );

	//ape_script_manager_do_file( "scripts/script_test.lua" );
}

void ape_script_manager_shutdown_()
{
	if ( scriptLuaState != nullptr )
	{
		lua_close( scriptLuaState );
		scriptLuaState = nullptr;
	}
}

bool ape_script_manager_do_string( const char *buf )
{
	if ( !luaL_dostring( scriptLuaState, buf ) )
	{
		ape_console_warning_( "Lua error: %s\n", lua_tostring( scriptLuaState, -1 ) );
		return false;
	}

	return true;
}

bool ape_script_manager_do_file( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == nullptr )
	{
		ape_console_warning_( "Failed to open file (%s): %s\n", path, PlGetError() );
		return false;
	}

	size_t size = PlGetFileSize( file );
	if ( size == 0 || size >= PlMegabytesToBytes( 64 ) )
	{
		ape_console_warning_( "Invalid size for file (%s) (%u)!\n", path, size );
		return false;
	}

	bool status = false;

	char *buf = QM_OS_MEMORY_NEW_( char, size + 1 );
	PlReadFile( file, buf, sizeof( char ), size );
	PlCloseFile( file );

	if ( !luaL_dostring( scriptLuaState, buf ) )
	{
		status = true;
	}
	else
	{
		ape_console_warning_( "Lua error (%s): %s\n", path, lua_tostring( scriptLuaState, -1 ) );
	}

	qm_os_memory_free( buf );

	return status;
}

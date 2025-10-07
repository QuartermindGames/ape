// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef enum ApeScriptLuaExportType
{
	APE_SCRIPT_LUA_EXPORT_TYPE_FUNC,
	APE_SCRIPT_LUA_EXPORT_TYPE_VAR,
	APE_SCRIPT_LUA_EXPORT_TYPE_CONST,
} ApeScriptLuaExportType;

typedef enum ApeScriptLuaDataType
{
	APE_SCRIPT_LUA_DATA_TYPE_INT,
	APE_SCRIPT_LUA_DATA_TYPE_FLOAT,
	APE_SCRIPT_LUA_DATA_TYPE_STRING,
} ApeScriptLuaDataType;

typedef union ApeScriptLuaData
{
	int         vi;
	float       vf;
	bool        vb;
	const char *vs;
} ApeScriptLuaData;

typedef struct ApeScriptLuaExport
{
	const char            *name;
	ApeScriptLuaExportType type;

	union
	{
		int ( *func )( void *state );
		struct
		{
			void                *ptr;
			ApeScriptLuaDataType type;
		} var;
		struct
		{
			ApeScriptLuaData     data;
			ApeScriptLuaDataType type;
		} constant;
	};
} ApeScriptLuaExport;

#define APE_SCRIPT_REGISTER_FUNC( NAME, METHOD )                          \
	{                                                                     \
		( NAME ), APE_SCRIPT_LUA_EXPORT_TYPE_FUNC, { .func = ( METHOD ) } \
	}
#define APE_SCRIPT_REGISTER_CONST_INT( NAME, VALUE )              \
	{                                                             \
		( NAME ), APE_SCRIPT_LUA_EXPORT_TYPE_CONST,               \
		{                                                         \
			.constant = {.data.vi = ( VALUE ),                    \
				         .type    = APE_SCRIPT_LUA_DATA_TYPE_INT, \
			}                                                     \
		}                                                         \
	}
#define APE_SCRIPT_REGISTER_CONST_STRING( NAME, VALUE )              \
	{                                                                \
		( NAME ), APE_SCRIPT_LUA_EXPORT_TYPE_CONST,                  \
		{                                                            \
			.constant = {.data.vs = ( VALUE ),                       \
				         .type    = APE_SCRIPT_LUA_DATA_TYPE_STRING, \
			}                                                        \
		}                                                            \
	}

typedef struct ApeScriptLuaInterface
{
	const char               *name;
	const ApeScriptLuaExport *exports;
	unsigned int              numExports;
} ApeScriptLuaInterface;

void ape_script_manager_register_interface( const ApeScriptLuaInterface *interface );

bool ape_script_manager_do_string( const char *buf );
bool ape_script_manager_do_file( const char *path );

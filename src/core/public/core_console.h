// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "qmmath/public/qm_math_colour.h"

PL_EXTERN_C

void ape_console_parse( const char *string );

/////////////////////////////////////////////////////////////////////////////////////
// Console Variables
/////////////////////////////////////////////////////////////////////////////////////

static constexpr unsigned int APE_CONSOLE_VAR_MAX_STRING = 512;
typedef char                  ApeConsoleVarString[ APE_CONSOLE_VAR_MAX_STRING ];

typedef struct ApeConsoleVar ApeConsoleVar;

typedef void ( *ApeConsoleCallback )( ApeConsoleVar *var );

enum
{
	QM_OS_BIT_FLAG( APE_CONSOLE_VAR_FLAG_ARCHIVE, 0 ),
	QM_OS_BIT_FLAG( APE_CONSOLE_VAR_FLAG_CHEAT, 1 ),
};

/* todo: make this structure private */
typedef struct ApeConsoleVar
{
	char *name;
	char *description;

	PLVariableType type;

	ApeConsoleCallback CallbackFunction;

	/////////////////////////////

	union
	{
		float       f_value;
		int         i_value;
		const char *s_value;
		bool        b_value;
	};
	char  value[ APE_CONSOLE_VAR_MAX_STRING ];
	char  default_value[ APE_CONSOLE_VAR_MAX_STRING ];
	void *ptrValue;

	unsigned int flags;
} ApeConsoleVar;

void ape_console_var_register( const char *name, const char *desc, const char *value, PLVariableType type, void *ptr, ApeConsoleCallback callback, unsigned int flags );

void PlGetConsoleVariables( ApeConsoleVar ***vars, size_t *num_vars );

ApeConsoleVar *PlGetConsoleVariable( const char *name );

const char *ape_console_var_get( const char *name );
const char *ape_console_var_get_default( const char *name );

void ape_console_var_set_( ApeConsoleVar *var, const char *value );
void ape_console_var_set( const char *name, const char *value );

unsigned int ape_console_var_match( const char *name, const char **dstOptions, unsigned int dstSize );

/* fetch and cache console var for trivial lookup */
#define PL_GET_CVAR( NAME, STORE )                    \
	static ApeConsoleVar *( STORE ) = NULL;           \
	if ( ( STORE ) == NULL )                          \
	{                                                 \
		( STORE ) = PlGetConsoleVariable( ( NAME ) ); \
		assert( ( STORE ) != NULL );                  \
	}

/////////////////////////////////////////////////////////////////////////////////////
// Console Commands
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeConsoleCmd ApeConsoleCmd;

void ape_console_cmd_register( const char *name, const char *description, int args, void ( *CallbackFunction )( unsigned int argc, const char *const *argv ) );

unsigned int ape_console_cmd_match( const char *name, const char **dstOptions, unsigned int dstSize );

PL_EXTERN_C_END

// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Console Variable management.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_string.h"
#include "plcore/pl_hashtable.h"
#include "aux/public/aux_project.h"

#include "ape_private.h"

static ApeConsoleVar **variables;
static size_t          numVariables;
static size_t          maxVariables = 512;
static PLHashTable    *variableHashes;

void ape_console_var_initialize_()
{
	variables = APE_MEMORY_NEW_C( ApeConsoleVar *, maxVariables );
}

void ape_console_var_shutdown_()
{
	PlDestroyHashTable( variableHashes );
	variableHashes = nullptr;

	if ( variables )
	{
		for ( ApeConsoleVar **var = variables; var < variables + numVariables; ++var )
		{
			// todo, should we return here; assume it's the end?
			if ( *var == nullptr )
			{
				continue;
			}

			qm_os_memory_free( ( *var )->name );
			qm_os_memory_free( ( *var )->description );
			qm_os_memory_free( *var );
		}

		qm_os_memory_free( variables );
		variables = nullptr;
	}
}

static void console_var_print_details( const ApeConsoleVar *var )
{
	ape_console_print_( " %-25s : %-5s / %-15s : %-20s\n",
	                    var->name,
	                    var->value,
	                    var->default_value,
	                    var->description != nullptr ? var->description : "None" );
}

bool ape_console_var_parse_( const char *name, unsigned int argc, const char *const *argv )
{
	ApeConsoleVar *var = PlGetConsoleVariable( name );
	if ( var == nullptr )
	{
		return false;
	}

	if ( argc == 0 )
	{
		console_var_print_details( var );
		return true;
	}

	ape_console_var_set_( var, argv[ 1 ] );

	return true;
}

void ape_console_var_find_( const char *term )
{
	ape_console_print_( "Variables that match the term \"%s\"\n", term );
	for ( ApeConsoleVar **var = variables; var < variables + numVariables; ++var )
	{
		if ( pl_strcasestr( ( *var )->name, term ) == nullptr && ( *var )->description != nullptr && pl_strcasestr( ( *var )->description, term ) == nullptr )
		{
			continue;
		}

		console_var_print_details( *var );
	}
}

bool ape_console_var_help_( const char *name )
{
	ApeConsoleVar *var = PlGetConsoleVariable( name );
	if ( var == nullptr )
	{
		return false;
	}

	console_var_print_details( var );
	return true;
}

ApeConsoleVar *ape_console_var_register_( const char *name, const char *description, const char *defaultValue, PLVariableType type, void *ptrValue, ApeConsoleCallback CallbackFunction, bool archive )
{
	assert( variables );

	if ( PlGetConsoleVariable( name ) != nullptr )
	{
		ape_console_warning_( "Variable with name (%s) has already been registered!\n", name );
		return nullptr;
	}

	// Deal with resizing the array dynamically...
	if ( 1 + numVariables > maxVariables )
	{
		variables = ( ApeConsoleVar ** ) qm_os_memory_realloc( variables, ( maxVariables += 128 ) * sizeof( ApeConsoleVar ) );
	}

	if ( variableHashes == nullptr )
	{
		variableHashes = PlCreateHashTable();
	}

	ApeConsoleVar *out = nullptr;
	if ( numVariables < maxVariables )
	{
		variables[ numVariables ] = ( ApeConsoleVar * ) QM_OS_MEMORY_MALLOC_( sizeof( ApeConsoleVar ) );
		out                       = variables[ numVariables ];

		out->type = type;
		if ( ptrValue != nullptr )
		{
			out->ptrValue = ptrValue;
		}

		if ( archive )
		{
			out->flags |= APE_CONSOLE_VAR_FLAG_ARCHIVE;
		}

		size_t s  = strlen( name ) + 1;
		out->name = QM_OS_MEMORY_NEW_( char, s );
		strncpy( out->name, name, s );
		qm_os_string_to_lower( out->name, s );

		PlInsertHashTableNode( variableHashes, out->name, s, out );

		// restore name back to case variant
		strncpy( out->name, name, s );

		if ( description != nullptr )
		{
			s                = strlen( description ) + 1;
			out->description = QM_OS_MEMORY_NEW_( char, s );
			strncpy( out->description, description, s );
		}

		qm_os_string_copy( out->default_value, defaultValue, sizeof( out->default_value ) );
		ape_console_var_set_( out, out->default_value );

		// Ensure the callback is only called afterwards
		if ( CallbackFunction != nullptr )
		{
			out->CallbackFunction = CallbackFunction;
		}

		numVariables++;
	}

	return out;
}

void PlGetConsoleVariables( ApeConsoleVar ***vars, size_t *num_vars )
{
	*vars     = variables;
	*num_vars = numVariables;
}

ApeConsoleVar *PlGetConsoleVariable( const char *name )
{
	if ( variableHashes == nullptr )
	{
		PlReportErrorF( PL_RESULT_MEMORY_EOA, "no console variables registered" );
		return nullptr;
	}

	// convert it so it's case insensitive here...
	size_t s   = strlen( name ) + 1;
	char  *tmp = QM_OS_MEMORY_NEW_( char, s );
	for ( unsigned int i = 0; i < s; ++i )
	{
		tmp[ i ] = ( char ) tolower( name[ i ] );
	}

	ApeConsoleVar *var = PlLookupHashTableUserData( variableHashes, tmp, s );

	qm_os_memory_free( tmp );

	return var;
}

const char *ape_console_var_get( const char *name )
{
	ApeConsoleVar *var = PlGetConsoleVariable( name );
	if ( var == nullptr )
	{
		return nullptr;
	}

	return var->value;
}

const char *ape_console_var_get_default( const char *name )
{
	ApeConsoleVar *var = PlGetConsoleVariable( name );
	if ( var == nullptr )
	{
		return nullptr;
	}

	return var->default_value;
}

// Set console variable, with sanity checks...
void ape_console_var_set_( ApeConsoleVar *var, const char *value )
{
	assert( var );
	switch ( var->type )
	{
		default:
			ape_console_warning_( "Unknown variable type %d, failed to set!\n", var->type );
			return;

		case PL_VAR_I32:
			if ( pl_strisdigit( value ) != -1 )
			{
				ape_console_warning_( "Unknown argument type %s, failed to set!\n", value );
				return;
			}

			var->i_value = ( int ) strtol( value, nullptr, 10 );
			if ( var->ptrValue != nullptr )
			{
				*( int * ) var->ptrValue = var->i_value;
			}
			break;

		case PL_VAR_STRING:
			var->s_value = &var->value[ 0 ];
			if ( var->ptrValue != nullptr )
			{
				qm_os_string_copy( var->ptrValue, value, APE_CONSOLE_VAR_MAX_STRING );
			}
			break;

		case PL_VAR_F32:
			var->f_value = strtof( value, nullptr );
			if ( var->ptrValue != nullptr )
			{
				*( float * ) var->ptrValue = var->f_value;
			}
			break;

		case PL_VAR_BOOL:
			if ( pl_strisalnum( value ) == -1 )
			{
				ape_console_warning_( "Unknown argument type %s, failed to set!\n", value );
				return;
			}

			if ( strcmp( value, "true" ) == 0 || strcmp( value, "1" ) == 0 )
			{
				var->b_value = true;
			}
			else
			{
				var->b_value = false;
			}

			if ( var->ptrValue != nullptr )
			{
				*( bool * ) var->ptrValue = var->b_value;
			}
			break;
	}

	qm_os_string_copy( var->value, value, sizeof( var->value ) );

	if ( var->CallbackFunction != nullptr )
	{
		var->CallbackFunction( var );
	}
}

void ape_console_var_set( const char *name, const char *value )
{
	ApeConsoleVar *var = PlGetConsoleVariable( name );
	if ( var == nullptr )
	{
		ape_console_warning_( "Failed to find console variable \"%s\"!\n", name );
		return;
	}

	ape_console_var_set_( var, value );
}

unsigned int ape_console_var_match( const char *name, const char **dstOptions, const unsigned int dstSize )
{
	size_t l = strlen( name );

	unsigned int c = 0;
	for ( unsigned int i = 0; i < numVariables; ++i )
	{
		if ( c >= dstSize )
		{
			break;
		}

		if ( pl_strncasecmp( name, variables[ i ]->name, l ) != 0 )
		{
			continue;
		}

		dstOptions[ c++ ] = variables[ i ]->name;
	}

	return c;
}

void ape_console_var_register( const char *name, const char *desc, const char *value, PLVariableType type, void *ptr, ApeConsoleCallback callback, unsigned int flags )
{
	// unlike the original api, the defaults are fetched from the project config
	// why? so that individual projects can define their own appropriate defaults

	AcmBranch *configBranch = com_project_get_config();
	if ( configBranch != nullptr )
	{
		AcmBranch *child = acm_get_child_by_name( configBranch, "defaultConfig" );
		if ( child != nullptr )
		{
			value = acm_get_string( child, name, value );
		}
	}

	ApeConsoleVar *var = ape_console_var_register_( name, desc, value, type, ptr, callback, flags & APE_CONSOLE_VAR_FLAG_ARCHIVE );
	if ( var == nullptr )
	{
		ape_console_warning_( "Failed to register console variable (%s): %s\n", name, PlGetError() );
		return;
	}

	if ( ( configBranch = ape_get_config_() ) == nullptr )
	{
		ape_console_warning_( "Failed to fetch project config, you might be registering your console variable (%s) too soon!\n", name );
		return;
	}

	AcmBranch *child;
	if ( ( child = acm_get_child_by_name( configBranch, name ) ) == nullptr )
	{
		return;
	}

	const char *configValue = acm_branch_get_value( child, nullptr );
	assert( configValue != nullptr );

	// the save and restore of the callback here is a disgusting hack
	// Duplexity thinks this should keep consistent; register method skips it
	// I don't know so I'm doing as suggested, unfortunately we've got to do
	// it in this gross way :(

	ApeConsoleCallback storeCallback = var->CallbackFunction;
	var->CallbackFunction            = nullptr;

	ape_console_var_set_( var, configValue );

	var->CallbackFunction = storeCallback;
}

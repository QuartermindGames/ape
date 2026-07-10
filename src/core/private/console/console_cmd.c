// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Console Command management.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_string.h"
#include "plcore/pl_hashtable.h"
#include "aux/public/aux_project.h"

#include "ape_private.h"

typedef struct ApeConsoleCmd
{
	char *name;
	char *description;
	int   args;
	void ( *Callback )( unsigned int argc, const char *const *argv );
} ApeConsoleCmd;

static ApeConsoleCmd **commands;
static size_t          numCommands;
static size_t          maxCommands = 512;
static PLHashTable    *commandHashes;

static ApeConsoleCmd *console_cmd_get( const char *name )
{
	if ( commandHashes == nullptr )
	{
		PlReportErrorF( PL_RESULT_MEMORY_EOA, "no console commands registered" );
		return nullptr;
	}

	// convert it so it's case insensitive here...
	size_t s   = strlen( name ) + 1;
	char  *tmp = APE_MEMORY_NEW_C( char, s );
	for ( unsigned int i = 0; i < s; ++i )
	{
		tmp[ i ] = ( char ) tolower( name[ i ] );
	}

	ApeConsoleCmd *command = PlLookupHashTableUserData( commandHashes, tmp, s );

	qm_os_memory_free( tmp );

	return command;
}

static void console_cmd_print_details( const ApeConsoleCmd *cmd )
{
	ape_console_print_( " %-25s : %-20s\n",
	                    cmd->name,
	                    cmd->description != nullptr ? cmd->description : "None" );
}

static void console_cmd_cmds_command( unsigned int argc, const char *const *argv )
{
	PL_UNUSEDVAR( argv );
	PL_UNUSEDVAR( argc );

	for ( ApeConsoleCmd **cmd = commands; cmd < commands + numCommands; ++cmd )
	{
		console_cmd_print_details( *cmd );
	}

	ape_console_print_( "%zu commands in total\n", numCommands );
}

#if 0
IMPLEMENT_COMMAND( vars ) {
	PL_UNUSEDVAR( argv );
	PL_UNUSEDVAR( argc );
	for ( ApeConsoleVar **var = _pl_variables; var < _pl_variables + _pl_num_variables; ++var ) {
		PrintVarDetails( ( *var ) );
	}
	Print( "%zu variables in total\n", _pl_num_variables );
}
#endif

void ape_console_cmd_initialize_()
{
	commands = APE_MEMORY_NEW_C( ApeConsoleCmd *, maxCommands );

	//ape_console_cmd_register( "time", "Returns the current time.", 0, time_cmd );
	//ape_console_cmd_register( "mem", "Returns the current memory usage stats.", 0, mem_cmd );
	ape_console_cmd_register( "cmds", "Prints a list of available commands.", 0, console_cmd_cmds_command );
	//ape_console_cmd_register( "vars", "Prints a list of available variables.", 0, vars_cmd );
	//ape_console_cmd_register( "pwd", "Prints out the current working directory.", 0, pwd_cmd );
	//ape_console_cmd_register( "echo", "Echos the given input to the console output.", 1, echo_cmd );
}

void ape_console_cmd_shutdown_()
{
	PlDestroyHashTable( commandHashes );
	commandHashes = nullptr;

	if ( commands )
	{
		for ( ApeConsoleCmd **cmd = commands; cmd < commands + numCommands; ++cmd )
		{
			// todo, should we return here; assume it's the end?
			if ( *cmd == nullptr )
			{
				continue;
			}

			qm_os_memory_free( ( *cmd )->name );
			qm_os_memory_free( ( *cmd )->description );
			qm_os_memory_free( *cmd );
		}

		qm_os_memory_free( commands );
		commands = nullptr;
	}
}

bool ape_console_cmd_parse_( const char *name, unsigned int argc, const char *const *argv )
{
	ApeConsoleCmd *cmd = console_cmd_get( name );
	if ( cmd == nullptr )
	{
		return false;
	}

	assert( cmd->Callback != nullptr );

	if ( cmd->args >= 0 && argc - 1 != cmd->args )
	{
		ape_console_print_( "\tInvalid number of arguments!\n" );
		ape_console_print_( "\t\t%s\n", cmd->description != NULL ? cmd->description : "None" );
		return true;
	}

	cmd->Callback( argc, argv );

	return true;
}

void ape_console_cmd_find_( const char *term )
{
	ape_console_print_( "Commands that match the term \"%s\"\n", term );
	for ( ApeConsoleCmd **cmd = commands; cmd < commands + numCommands; ++cmd )
	{
		if ( pl_strcasestr( ( *cmd )->name, term ) == nullptr && ( *cmd )->description != nullptr && pl_strcasestr( ( *cmd )->description, term ) == nullptr )
		{
			continue;
		}

		console_cmd_print_details( *cmd );
	}
}

bool ape_console_cmd_help_( const char *name )
{
	ApeConsoleCmd *cmd = console_cmd_get( name );
	if ( cmd == nullptr )
	{
		return false;
	}

	console_cmd_print_details( cmd );
	return true;
}

void ape_console_cmd_register( const char *name, const char *description, int args, void ( *CallbackFunction )( unsigned int argc, const char *const *argv ) )
{
	if ( console_cmd_get( name ) != nullptr )
	{
		ape_console_warning_( "Command with name (%s) has already been registered!\n", name );
		return;
	}

	if ( CallbackFunction == nullptr )
	{
		PlReportErrorF( PL_RESULT_COMMAND_FUNCTION, PlGetResultString( PL_RESULT_COMMAND_FUNCTION ) );
		return;
	}

	// Deal with resizing the array dynamically...
	if ( 1 + numCommands > maxCommands )
	{
		commands = qm_os_memory_realloc( commands, ( maxCommands += 128 ) * sizeof( ApeConsoleCmd ) );
	}

	if ( commandHashes == nullptr )
	{
		commandHashes = PlCreateHashTable();
	}

	if ( numCommands < maxCommands )
	{
		commands[ numCommands ] = QM_OS_MEMORY_NEW( ApeConsoleCmd );
		if ( !commands[ numCommands ] )
		{
			return;
		}

		ApeConsoleCmd *cmd = commands[ numCommands ];
		QM_OS_ZERO( cmd, sizeof( ApeConsoleCmd ) );
		cmd->Callback = CallbackFunction;

		size_t s  = strlen( name ) + 1;
		cmd->name = QM_OS_MEMORY_NEW_( char, s );
		strncpy( cmd->name, name, s );
		qm_os_string_to_lower( cmd->name, s );

		PlInsertHashTableNode( commandHashes, cmd->name, s, cmd );

		// restore name back to case variant
		strncpy( cmd->name, name, s );

		if ( description != nullptr )
		{
			s                = strlen( description ) + 1;
			cmd->description = QM_OS_MEMORY_NEW_( char, s );
			strncpy( cmd->description, description, s );
		}

		cmd->args = args;

		numCommands++;
	}
}

unsigned int ape_console_cmd_match( const char *name, const char **dstOptions, const unsigned int dstSize )
{
	size_t l = strlen( name );

	unsigned int c = 0;
	for ( unsigned int i = 0; i < numCommands; ++i )
	{
		if ( c >= dstSize )
		{
			break;
		}

		if ( pl_strncasecmp( name, commands[ i ]->name, l ) != 0 )
		{
			continue;
		}

		dstOptions[ c++ ] = commands[ i ]->name;
	}

	return c;
}

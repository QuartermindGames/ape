// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@oldtimes-software.com>

#include "script.h"

#include "plcore/pl_console.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static void exec_command( unsigned int argc, char **argv )
{
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ss_script_register_commands( void )
{
	PlRegisterConsoleCommand( "sscript_exec", "Execute the specified script.", -1, exec_command );
}

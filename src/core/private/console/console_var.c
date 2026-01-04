// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Console Variable management.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_string.h"
#include "aux/public/aux_project.h"

#include "../ape_private.h"

#include "console.h"

// for now this is pretty much a glorified wrapper around the pl* api, until it's migrated over...

void ape_console_var_register( const char *name, const char *desc, const char *value, PLVariableType type, void *ptr, void ( *callback )( PLConsoleVariable * ), unsigned int flags )
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

	PLConsoleVariable *var = PlRegisterConsoleVariable( name, desc, value, type, ptr, callback, flags & APE_CONSOLE_VAR_FLAG_ARCHIVE );
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

	PlSetConsoleVariable( var, configValue );
}

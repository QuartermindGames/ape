// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Primary code for dealing with editor functionality.

#include "ape_private.h"
#include "editors.h"
#include "client/renderer/renderer.h"

static ApeEditorContext *contexts[ YN_CORE_EDITOR_MAX_CONTEXTS ];
static ApeEditorContext *currentContext = NULL;

static EditorStatus editorStatus = EDITOR_CLOSED;
EditorStatus apeGetEditorStatus( void ) { return editorStatus; }

void apeRegisterEditorConsoleVariables_( void )
{
	ApeEditorContext *YnCore_RegisterWorldEditorContext( void );
	contexts[ YN_CORE_EDITOR_CONTEXT_WORLD ] = YnCore_RegisterWorldEditorContext();

	// setup shared vars per context
	for ( unsigned int i = 0; i < YN_CORE_EDITOR_MAX_CONTEXTS; ++i )
	{
		char buf[ 64 ];

		snprintf( buf, sizeof( buf ), "editor.%s.hideGrid", contexts[ i ]->identifier );
		PlRegisterConsoleVariable( buf,
		                           "Toggles grid for editor.",
		                           "false", PL_VAR_BOOL,
		                           &contexts[ i ]->hideGrid,
		                           NULL, true );
		snprintf( buf, sizeof( buf ), "editor.%s.useLineGrid", contexts[ i ]->identifier );
		PlRegisterConsoleVariable( buf,
		                           "Toggles between a dotted grid and line grid.",
		                           "true", PL_VAR_BOOL,
		                           &contexts[ i ]->useLineGrid,
		                           NULL, true );
		snprintf( buf, sizeof( buf ), "editor.%s.gridScale", contexts[ i ]->identifier );
		PlRegisterConsoleVariable( buf,
		                           "Sets the scale of the grid.",
		                           "4", PL_VAR_I32,
		                           &contexts[ i ]->gridScale,
		                           NULL, true );
	}
}

static void ToggleEditorCallback( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	if ( apeGetCurrentEditorContext() != NULL )
	{
		//TODO: check status, do we need to save?
		editorStatus   = EDITOR_CLOSED;
		currentContext = NULL;
		return;
	}

	apeSetEditorContext( YN_CORE_EDITOR_CONTEXT_WORLD );
}

void apeInitializeEditor_( void )
{
	PlRegisterConsoleCommand( "editor",
	                          "Enable/disable editor mode.",
	                          0, ToggleEditorCallback );

	for ( uint32_t i = 0; i < YN_CORE_EDITOR_MAX_CONTEXTS; ++i )
	{
		assert( contexts[ i ]->Initialize != NULL );
		if ( contexts[ i ]->Initialize == NULL )
		{
			continue;
		}

		contexts[ i ]->Initialize();
	}
}

void apeShutdownEditor_( void )
{
	for ( uint32_t i = 0; i < YN_CORE_EDITOR_MAX_CONTEXTS; ++i )
	{
		assert( contexts[ i ]->Shutdown != NULL );
		if ( contexts[ i ]->Shutdown == NULL )
		{
			continue;
		}

		contexts[ i ]->Shutdown();
	}
}

void apeTickEditor_( void )
{
	if ( currentContext == NULL )
	{
		return;
	}

	assert( currentContext->Tick != NULL );
	if ( currentContext->Tick == NULL )
	{
		return;
	}

	currentContext->Tick();
}

void YnCore_DrawEditor( void )
{
	if ( currentContext == NULL )
	{
		return;
	}

	assert( currentContext->Draw != NULL );
	if ( currentContext->Draw == NULL )
	{
		return;
	}

	currentContext->Draw();
}

void apeDrawEditorGUI_( const ApeViewport *viewport )
{
	if ( currentContext == NULL )
	{
		return;
	}

	assert( currentContext->DrawGUI != NULL );
	if ( currentContext->DrawGUI == NULL )
	{
		return;
	}

	currentContext->DrawGUI();
}

ApeEditorContext *apeGetCurrentEditorContext( void )
{
	return currentContext;
}

ApeEditorContext *apeGetEditorContext( const char *identifier )
{
	for ( uint32_t i = 0; i < YN_CORE_EDITOR_MAX_CONTEXTS; ++i )
	{
		if ( strcmp( contexts[ i ]->identifier, identifier ) != 0 )
		{
			continue;
		}

		return contexts[ i ];
	}

	return NULL;
}

ApeEditorContext *apeSetEditorContext( ApeEditorContextType type )
{
	currentContext = contexts[ type ];
	editorStatus   = EDITOR_OPEN;
	if ( currentContext->OnActive != NULL )
	{
		currentContext->OnActive();
	}
	return currentContext;
}

bool apeIsEditorContextActive( const char *identifier )
{
	if ( currentContext == NULL )
	{
		return false;
	}

	return ( strcmp( currentContext->name, identifier ) == 0 );
}

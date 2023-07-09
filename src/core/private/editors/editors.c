// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Primary code for dealing with editor functionality.

#include "ape_private.h"
#include "editors.h"
#include "client/renderer/renderer.h"

static ApeEditorContext *contexts[ APE_EDITOR_MAX_CONTEXTS ];
static ApeEditorContext *currentContext = NULL;

static EditorStatus editorStatus = EDITOR_CLOSED;
EditorStatus apeGetEditorStatus( void ) { return editorStatus; }

static void ToggleEditorCallback( PL_UNUSED unsigned int argc, PL_UNUSED char **argv )
{
	if ( apeGetCurrentEditorContext() != NULL )
	{
		//TODO: check status, do we need to save?
		editorStatus   = EDITOR_CLOSED;
		currentContext = NULL;
		return;
	}

	apeSetEditorContext( APE_EDITOR_CONTEXT_WORLD );
}

void apeInitializeEditor_( void )
{
	PlRegisterConsoleCommand( "editor", "Enable/disable editor mode.", 0, ToggleEditorCallback );

#if 0
	for ( uint32_t i = 0; i < APE_EDITOR_MAX_CONTEXTS; ++i )
	{
		assert( contexts[ i ]->Initialize != NULL );
		if ( contexts[ i ]->Initialize == NULL )
		{
			continue;
		}

		contexts[ i ]->Initialize();
	}
#endif
}

void apeShutdownEditor_( void )
{
#if 0
	for ( uint32_t i = 0; i < APE_EDITOR_MAX_CONTEXTS; ++i )
	{
		assert( contexts[ i ]->Shutdown != NULL );
		if ( contexts[ i ]->Shutdown == NULL )
		{
			continue;
		}

		contexts[ i ]->Shutdown();
	}
#endif
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

void apeDrawEditor_( void )
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
	for ( uint32_t i = 0; i < APE_EDITOR_MAX_CONTEXTS; ++i )
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

ApeMaterial *apeGetEditorIconMaterial( const char *name )
{
	PLPath path;
	PlSetupPath( path, true, "editor/icons/%s.mat.n", name );
	return apeCacheMaterial( path, APE_CACHE_EDITOR, true, false );
}

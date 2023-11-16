// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Primary code for dealing with editor functionality.

#include "ape_private.h"
#include "editor.h"
#include "client/renderer/renderer.h"

static ApeEditorContext *contexts[ APE_EDITOR_MAX_CONTEXTS ];
static ApeEditorContext *currentContext = NULL;

static ApeEditorStatus editorStatus = APE_EDITOR_STATUS_CLOSED;

static void EditorToggleCallback( PL_UNUSED unsigned int argc, PL_UNUSED char **argv ) {
	if ( editorStatus == APE_EDITOR_STATUS_CLOSING ) {
		return;
	} else if ( editorStatus == APE_EDITOR_STATUS_CLOSED ) {
		apeOpenEditor_();
		return;
	}

	apeCloseEditor_();
}

void apeInitializeEditor_( void ) {
	PlRegisterConsoleCommand( "ape/editor/toggle", "Toggle main editor functionality.", 0, EditorToggleCallback );

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

void apeShutdownEditor_( void ) {
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

void apeTickEditor_( void ) {
	if ( currentContext == NULL ) {
		return;
	}

	assert( currentContext->Tick != NULL );
	if ( currentContext->Tick == NULL ) {
		return;
	}

	currentContext->Tick();
}

void apeDrawEditor_( void ) {
	if ( currentContext == NULL ) {
		return;
	}

	assert( currentContext->Draw != NULL );
	if ( currentContext->Draw == NULL ) {
		return;
	}

	currentContext->Draw();
}

void apeOpenEditor_( void ) {
	editorStatus = APE_EDITOR_STATUS_OPEN;
}

void apeCloseEditor_( void ) {
}

void apeDrawEditorGUI_( const SS_Arl_Viewport *viewport ) {
	if ( currentContext == NULL ) {
		return;
	}

	assert( currentContext->DrawGUI != NULL );
	if ( currentContext->DrawGUI == NULL ) {
		return;
	}

	currentContext->DrawGUI();
}

ApeEditorContext *apeGetCurrentEditorContext( void ) {
	return currentContext;
}

ApeEditorContext *apeGetEditorContext( const char *identifier ) {
	for ( uint32_t i = 0; i < APE_EDITOR_MAX_CONTEXTS; ++i ) {
		if ( strcmp( contexts[ i ]->identifier, identifier ) != 0 ) {
			continue;
		}

		return contexts[ i ];
	}

	return NULL;
}

ApeEditorContext *apeSetEditorContext( ApeEditorContextType type ) {
	currentContext = contexts[ type ];
	editorStatus = APE_EDITOR_STATUS_OPEN;
	if ( currentContext->OnActive != NULL ) {
		currentContext->OnActive();
	}

	return currentContext;
}

bool apeIsEditorContextActive( const char *identifier ) {
	if ( currentContext == NULL ) {
		return false;
	}

	return ( strcmp( currentContext->name, identifier ) == 0 );
}

ApeMaterial *apeGetEditorIconMaterial( const char *name ) {
	PLPath path;
	PlSetupPath( path, true, "editor/icons/%s.mat.n", name );
	return ss_arl_material_cache( path, APE_CACHE_EDITOR, true, false );
}

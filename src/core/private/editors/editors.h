// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <yin/core_editor.h>

PL_EXTERN_C

typedef enum EditorStatus
{
	EDITOR_CLOSED,
	EDITOR_CLOSING,
	EDITOR_OPEN,
} EditorStatus;
EditorStatus apeGetEditorStatus( void );

/**
 * Helper function for fetching icons specific to the editor.
 */
ApeMaterial *apeGetEditorIconMaterial( const char *name );

void apeRegisterEditorConsoleVariables_( void );

void apeInitializeEditor_( void );
void apeShutdownEditor_( void );
void apeTickEditor_( void );
void apeDrawEditor_( void );

void apeDrawEditorGUI_( const ApeViewport *viewport );

ApeEditorContext *apeGetCurrentEditorContext( void );
ApeEditorContext *apeGetEditorContext( const char *identifier );
ApeEditorContext *apeSetEditorContext( ApeEditorContextType type );
bool apeIsEditorContextActive( const char *identifier );

PL_EXTERN_C_END

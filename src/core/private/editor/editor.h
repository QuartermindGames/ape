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
EditorStatus YnCore_GetEditorStatus( void );

void ogeRegisterEditorConsoleVariables_( void );

void YnCore_InitializeEditor( void );
void ogeShutdownEditor( void );
void YnCore_TickEditor( void );

void YnCore_DrawEditorGUI( const ApeViewport *viewport );

ApeEditorContext *YnCore_GetCurrentEditorContext( void );
ApeEditorContext *YnCore_GetEditorContext( const char *identifier );
ApeEditorContext *YnCore_SetEditorContext( ApeEditorContextType type );
bool YnCore_IsEditorContextActive( const char *identifier );

PL_EXTERN_C_END

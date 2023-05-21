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

void YnCore_RegisterEditorConsoleVariables( void );

void YnCore_InitializeEditor( void );
void ogeShutdownEditor( void );
void YnCore_TickEditor( void );

void YnCore_DrawEditorGUI( const OgeViewport *viewport );

YNCoreEditorContext *YnCore_GetCurrentEditorContext( void );
YNCoreEditorContext *YnCore_GetEditorContext( const char *identifier );
YNCoreEditorContext *YnCore_SetEditorContext( YNCoreEditorContextType type );
bool YnCore_IsEditorContextActive( const char *identifier );

PL_EXTERN_C_END

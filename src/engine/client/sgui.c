/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 *
 * Purpose: SGUI; Simple Graphical User Interface
 */

#include "sgui.h"

typedef enum SGUIWidgetType
{
	SGUI_WIDGETTYPE_PANEL,
} SGUIWidgetType;

typedef struct SGUIWidget
{
	int x, y;

} SGUIWidget;

static PLLinkedList *sguiWidgets;

void SGUI_Initialize( void )
{
	sguiWidgets = PlCreateLinkedList();
	if ( sguiWidgets == NULL )
		PrintError( "Failed to create sgui list!\nPL: %s\n", PlGetError() );


}

void SGUI_Tick( void )
{

}

void SGUI_Draw( void )
{

}

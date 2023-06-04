// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "gui_private.h"
#include "gui_panel.h"

/****************************************
 * GUI PANEL
 ****************************************/

static void DrawBorder( GUIPanel *self )
{
	if ( self->border == GUI_PANEL_BORDER_NONE )
	{
		return;
	}

	PLGMesh *mesh = GUI_Draw_GetBatchQueueMesh( NULL );

	PLColourF32 topColour;
	PLColourF32 bottomColour;

	switch ( self->border )
	{
		default:
		case GUI_PANEL_BORDER_INSET:
			topColour    = self->styleSheet->colours[ GUI_COLOUR_INSET_BORDER_TOP ];
			bottomColour = self->styleSheet->colours[ GUI_COLOUR_INSET_BORDER_BOTTOM ];
			break;
		case GUI_PANEL_BORDER_OUTSET:
			topColour    = self->styleSheet->colours[ GUI_COLOUR_OUTSET_BORDER_TOP ];
			bottomColour = self->styleSheet->colours[ GUI_COLOUR_OUTSET_BORDER_BOTTOM ];
			break;
	}

	int x, y;
	GUI_Panel_GetAbsolutePosition( self, &x, &y );

	// top
	GUI_Draw_Quad( mesh,
	               ( GUIVector2 ){ x - self->styleSheet->borderPadding[ 0 ], y },
	               ( GUIVector2 ){ x + self->w + self->styleSheet->borderPadding[ 1 ], y },
	               ( GUIVector2 ){ x, y + self->styleSheet->borderPadding[ 2 ] },
	               ( GUIVector2 ){ x + self->w, y + self->styleSheet->borderPadding[ 3 ] },
	               self->z,
	               &topColour );

#if 0
	GUI_Draw_Quad( mesh,
	               ( GUIVector2 ){ x - self->styleSheet->borderPadding[ 0 ], y },
	               ( GUIVector2 ){ x + self->w + self->styleSheet->borderPadding[ 0 ], y },
	               ( GUIVector2 ){ x, y + self->styleSheet->borderPadding[ 0 ] },
	               ( GUIVector2 ){ x + self->w, y + self->styleSheet->borderPadding[ 0 ] },
	               &topColour );
	// bottom
	GUI_Draw_Quad( mesh,
	               ( GUIVector2 ){ x - self->styleSheet->borderPadding[ 0 ], y },
	               ( GUIVector2 ){ x + self->w + self->styleSheet->borderPadding[ 0 ], y },
	               ( GUIVector2 ){ x, y + self->styleSheet->borderPadding[ 0 ] },
	               ( GUIVector2 ){ x + self->w, y + self->styleSheet->borderPadding[ 0 ] },
	               &topColour );
	GUI_Draw_Quad( mesh,
	               ( GUIVector2 ){ x - self->styleSheet->borderPadding[ 0 ], y },
	               ( GUIVector2 ){ x + self->w + self->styleSheet->borderPadding[ 0 ], y },
	               ( GUIVector2 ){ x, y + self->styleSheet->borderPadding[ 0 ] },
	               ( GUIVector2 ){ x + self->w, y + self->styleSheet->borderPadding[ 0 ] },
	               &topColour );
#endif
}

/**
 * Create a new panel with the given parameters.
 * @param parent Position will be relative to parent.
 * @param x Absolute position, if no parent.
 * @param y Absolute position, if no parent.
 * @param w Absolute width, if no parent.
 * @param h Absolute height, if no parent.
 * @param background
 * @param border
 * @return
 */
GUIPanel *GUI_Panel_Create( GUIPanel *parent, int x, int y, int w, int h, GUIPanelBackground background, GUIPanelBorder border )
{
	GUIPanel *self         = PlMAllocA( sizeof( GUIPanel ) );
	self->x                = x;
	self->y                = y;
	self->w                = w;
	self->h                = h;
	self->background       = background;
	self->border           = border;
	self->styleSheet       = GUI_GetActiveStyleSheet();// todo: revisit, either set via parameter or other...
	self->backgroundColour = PL_COLOURU8( 255, 255, 255, 255 );

	self->children = PlCreateLinkedList();

	self->isDrawing = true;
	self->isVisible = true;

	if ( parent == NULL )
	{
		return self;
	}

	self->parent = parent;
	self->node   = PlInsertLinkedListNode( parent->children, self );

	return self;
}

/**
 * Destroy the given panel. Automatically culls all
 * children of the given panel too.
 */
void GUI_Panel_Destroy( GUIPanel *self )
{
	if ( self == NULL )
	{
		return;
	}

	/* be sure to remove us from the parent */
	if ( self->parent != NULL )
	{
		PlDestroyLinkedListNode( self->node );
	}

	/* and now cull all our children */
	PLLinkedListNode *childNode = PlGetFirstNode( self->children );
	while ( childNode != NULL )
	{
		GUIPanel *childPanel = PlGetLinkedListNodeUserData( childNode );
		childNode            = PlGetNextLinkedListNode( childNode );
		GUI_Panel_Destroy( childPanel );
	}
	PlDestroyLinkedList( self->children );

	PL_DELETE( self );
}

/**
 * This makes it possible for panels to use their own independent styles.
 */
void GUI_Panel_SetStyleSheet( GUIPanel *self, const GUIStyleSheet *styleSheet )
{
	self->styleSheet = styleSheet;
}

void GUI_Panel_Draw( GUIPanel *self )
{
	if ( !self->isDrawing )
	{
		return;
	}

	GUI_Panel_DrawBackground( self );

	DrawBorder( self );

	if ( self->PreDraw != NULL )
	{
		bool override;
		self->PreDraw( self, &override );
		if ( override )
		{
			return;
		}
	}

	/* draw all the children */
	PLLinkedListNode *childNode = PlGetFirstNode( self->children );
	while ( childNode != NULL )
	{
		GUIPanel *childPanel = PlGetLinkedListNodeUserData( childNode );
		GUI_Panel_Draw( childPanel );
		childNode = PlGetNextLinkedListNode( childNode );
	}

	if ( self->PostDraw != NULL )
	{
		self->PostDraw( self );
	}
}

void GUI_Panel_DrawBackground( GUIPanel *self )
{
	if ( self->DrawBackground != NULL )
	{
		bool override;
		self->DrawBackground( self, &override );
		if ( override )
		{
			return;
		}
	}

	PLColour colour;
	switch ( self->background )
	{
		default:
			return;
		case GUI_PANEL_BACKGROUND_DEFAULT:
		{
			colour = PlColourF32ToU8( &self->styleSheet->colours[ ( self->border == GUI_PANEL_BORDER_INSET ) ? GUI_COLOUR_INSET_BACKGROUND : GUI_COLOUR_OUTSET_BACKGROUND ] );
			break;
		}
		case GUI_PANEL_BACKGROUND_SOLID:
		{
			colour = self->backgroundColour;
			break;
		}
	}

	int x, y, w, h;
	GUI_Panel_GetContentPosition( self, &x, &y );
	GUI_Panel_GetContentSize( self, &w, &h );

	PLGMesh *mesh = GUI_Draw_GetBatchQueueMesh( NULL );
	assert( mesh != NULL );
	if ( mesh == NULL )
	{
		return;
	}

	GUI_Draw_FilledRectangle( mesh, x, y, w, h, self->z, &colour );
}

void GUI_Panel_Tick( GUIPanel *self )
{
	assert( self != NULL );
	if ( self == NULL )
	{
		return;
	}

	// Make sure the cursor is always updated/drawn last
	if ( self->cursor != NULL )
	{
		PlMoveLinkedListNodeToBack( self->cursor->node );
	}

	self->isDrawing = self->isVisible;

	bool override;
	if ( self->Tick != NULL )
	{
		self->Tick( self, &override );
		if ( override )
		{
			return;
		}
	}

	/* Tick all children */
	PLLinkedListNode *childNode = PlGetFirstNode( self->children );
	while ( childNode != NULL )
	{
		GUIPanel *childPanel = PlGetLinkedListNodeUserData( childNode );
		GUI_Panel_Tick( childPanel );
		childNode = PlGetNextLinkedListNode( childNode );
	}
}

void GUI_Panel_SetBackgroundColour( GUIPanel *self, const PLColour *colour )
{
	self->backgroundColour = *colour;
}

PLColour GUI_Panel_GetBackgroundColour( GUIPanel *self )
{
	return self->backgroundColour;
}

void GUI_Panel_SetBorder( GUIPanel *self, GUIPanelBorder border )
{
	self->border = border;
}

void GUI_Panel_SetBackground( GUIPanel *self, GUIPanelBackground background )
{
	self->background = background;
}

GUIPanel *GUI_Panel_GetParent( GUIPanel *self )
{
	return self->parent;
}

void GUI_Panel_GetPosition( GUIPanel *self, int *x, int *y )
{
	if ( x != NULL ) *x = self->x;
	if ( y != NULL ) *y = self->y;
}

void GUI_Panel_GetContentPosition( GUIPanel *self, int *x, int *y )
{
	if ( self->border == GUI_PANEL_BORDER_NONE )
	{
		GUI_Panel_GetPosition( self, x, y );
		return;
	}

	/* assume border is 2 pixels (todo: do this properly!) */
	if ( x != NULL ) *x = self->x + GUI_PANEL_BORDER_SIZE;
	if ( y != NULL ) *y = self->y + GUI_PANEL_BORDER_SIZE;
}

/**
 * Returns the absolute position of the element relative to it's parent.
 */
void GUI_Panel_GetAbsolutePosition( GUIPanel *self, int *x, int *y )
{
	int bx = 0, by = 0;
	GUIPanel *parent = self->parent;
	while ( parent != NULL )
	{
		bx += parent->x;
		by += parent->y;
		parent = parent->parent;
	}

	*x = bx + self->x;
	*y = by + self->y;
}

/**
 * Sets the position of the panel. Keep in mind this is relative
 * to it's parent (if it has one).
 */
void GUI_Panel_SetPosition( GUIPanel *self, int x, int y )
{
	/* be sure it respects the parent location */
	GUIPanel *parent = self->parent;
	if ( parent != NULL )
	{
		int cx, cy;
		GUI_Panel_GetContentPosition( parent, &cx, &cy );
		if ( x < cx ) x = cx;
		if ( y < cy ) y = cy;
	}

	self->x = x;
	self->y = y;

#if 0// Don't really think this is necessary? Given children are relative to parent
	/* and now be sure that all the children get updated */
	PLLinkedListNode *childNode = PlGetFirstNode( self->children );
	while ( childNode != NULL )
	{
		GUIPanel *childPanel = PlGetLinkedListNodeUserData( childNode );
		GUI_Panel_SetPosition( childPanel, childPanel->x, childPanel->y );
		childNode = PlGetNextLinkedListNode( childNode );
	}
#endif
}

void GUI_Panel_GetSize( GUIPanel *self, int *w, int *h )
{
	if ( w != NULL ) *w = self->w;
	if ( h != NULL ) *h = self->h;
}

void GUI_Panel_GetContentSize( GUIPanel *self, int *w, int *h )
{
	if ( self->border == GUI_PANEL_BORDER_NONE )
	{
		GUI_Panel_GetSize( self, w, h );
		return;
	}

	/* again, assume border is 2 pixels (todo: do this properly!) */
	if ( w != NULL ) *w = self->w - GUI_PANEL_BORDER_SIZE;
	if ( h != NULL ) *h = self->h - GUI_PANEL_BORDER_SIZE;
}

void GUI_Panel_SetSize( GUIPanel *self, int w, int h )
{
	self->w = w;
	self->h = h;
	// todo: recurse over children?
}

bool GUI_Panel_IsMouseOver( GUIPanel *self, int mx, int my )
{
	// Back in ye olden days, this was simple because we had explicit positions, but no more!
	int x, y;

	return !( mx < self->x || mx > self->x + self->w || my < self->y || my > self->y + self->h );
}

bool GUI_Panel_HandleMouseEvent( GUIPanel *self, int mx, int my, int wheel, int button, bool buttonUp )
{
	if ( !GUI_Panel_IsMouseOver( self, mx, my ) )
		return false;

	PLLinkedListNode *childNode = PlGetFirstNode( self->children );
	while ( childNode != NULL )
	{
		GUIPanel *childPanel = PlGetLinkedListNodeUserData( childNode );
		if ( GUI_Panel_HandleMouseEvent( childPanel, mx, my, wheel, button, buttonUp ) )
		{
			return true;
		}

		childNode = PlGetNextLinkedListNode( childNode );
	}

	if ( self->HandleMouseEvent != NULL && self->HandleMouseEvent( self, mx, my, wheel, button, buttonUp ) )
	{
		return true;
	}

	return false;
}

bool GUI_Panel_HandleKeyboardEvent( GUIPanel *self, int button, bool buttonUp )
{
	PLLinkedListNode *childNode = PlGetFirstNode( self->children );
	while ( childNode != NULL )
	{
		GUIPanel *childPanel = PlGetLinkedListNodeUserData( childNode );
		if ( GUI_Panel_HandleKeyboardEvent( childPanel, button, buttonUp ) )
		{
			return true;
		}

		childNode = PlGetNextLinkedListNode( childNode );
	}

	if ( self->HandleKeyboardEvent != NULL && self->HandleKeyboardEvent( self, button, buttonUp ) )
	{
		return true;
	}

	return false;
}

void GUI_Panel_SetVisible( GUIPanel *self, bool flag )
{
	self->isVisible = flag;
}

// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_console.h>

#include <yin/node.h>

#include "gui_private.h"

/****************************************
 * GUI
 ****************************************/

GuiState guiState;

static PLLinkedList *cachedTextures;
typedef struct GuiCachedTexture
{
	unsigned int hash;
	PLGTexture *texture;
} GuiCachedTexture;
PLGTexture *guiCacheTexture( const char *path )
{
	unsigned int hash = PlGenerateHashSDBM( path );
	PLLinkedListNode *node = PlGetFirstNode( cachedTextures );
	while ( node != NULL )
	{
		GuiCachedTexture *cachedTexture = PlGetLinkedListNodeUserData( node );
		if ( cachedTexture->hash == hash )
		{
			return cachedTexture->texture;
		}

		node = PlGetNextLinkedListNode( node );
	}

	PLGTexture *texture = PlgLoadTextureFromImage( path, PLG_TEXTURE_FILTER_LINEAR );
	if ( texture == NULL )
	{
		return NULL;
	}

	GuiCachedTexture *cachedTexture = PL_NEW( GuiCachedTexture );
	cachedTexture->texture = texture;
	cachedTexture->hash = hash;
	PlInsertLinkedListNode( cachedTextures, cachedTexture );
	return cachedTexture->texture;
}

#define MAX_STYLE_SHEETS 16
GuiStyleSheet styleSheets[ MAX_STYLE_SHEETS ];
const GuiStyleSheet *activeSheet = NULL;
static unsigned int numStyleSheets = 0;

#define GUI_STYLESHEET_VERSION 1

static GuiStyleSheet *ParseStyleSheet( NdBranch *root )
{
	GuiStyleSheet *guiStyleSheet = &styleSheets[ numStyleSheets ];
	PL_ZERO( guiStyleSheet, sizeof( GuiStyleSheet ) );

	unsigned int version = nd_branch_get_child_uint( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 || version > GUI_STYLESHEET_VERSION )
	{
		GUI_WARNING( "Unexpected version in stylesheet, expected %d but found %d!\n", GUI_STYLESHEET_VERSION, version );
		return NULL;
	}

	NdBranch *c;
	c = nd_branch_get_child_by_name( root, "colours" );
	if ( c != NULL )
	{
		NdBranch *i;
		if ( ( i = nd_branch_get_child_by_name( c, PL_STRINGIFY( GUI_COLOUR_INSET_BACKGROUND ) ) ) != NULL )
			nd_branch_get_float32_array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_INSET_BACKGROUND ], 4 );
		if ( ( i = nd_branch_get_child_by_name( c, PL_STRINGIFY( GUI_COLOUR_OUTSET_BACKGROUND ) ) ) != NULL )
			nd_branch_get_float32_array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_OUTSET_BACKGROUND ], 4 );
		if ( ( i = nd_branch_get_child_by_name( c, PL_STRINGIFY( GUI_COLOUR_INSET_BORDER_TOP ) ) ) != NULL )
			nd_branch_get_float32_array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_INSET_BORDER_TOP ], 4 );
		if ( ( i = nd_branch_get_child_by_name( c, PL_STRINGIFY( GUI_COLOUR_INSET_BORDER_BOTTOM ) ) ) != NULL )
			nd_branch_get_float32_array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_INSET_BORDER_BOTTOM ], 4 );
		if ( ( i = nd_branch_get_child_by_name( c, PL_STRINGIFY( GUI_COLOUR_OUTSET_BORDER_TOP ) ) ) != NULL )
			nd_branch_get_float32_array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_OUTSET_BORDER_TOP ], 4 );
		if ( ( i = nd_branch_get_child_by_name( c, PL_STRINGIFY( GUI_COLOUR_OUTSET_BORDER_BOTTOM ) ) ) != NULL )
			nd_branch_get_float32_array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_OUTSET_BORDER_BOTTOM ], 4 );
	}

	c = nd_branch_get_child_by_name( root, "borders" );
	if ( c != NULL )
	{
		unsigned int style = nd_branch_get_child_uint( c, "style", -1 );
		if ( style < GUI_MAX_BORDER_STYLES )
			guiStyleSheet->borderStyle = style;
		else
			GUI_WARNING( "No border style specified, using default.\n" );

		NdBranch *i;
		if ( ( i = nd_branch_get_child_by_name( c, "padding" ) ) != NULL )
			nd_branch_get_int32_array( i, guiStyleSheet->borderPadding, GUI_MAX_BORDER_ELEMENTS );
	}

	return guiStyleSheet;
}

const GuiStyleSheet *ss_gui_cache_style_sheet( const char *path )
{
	NdBranch *root = nd_load_file( path, "guiStyle" );
	if ( root == NULL )
	{
		GUI_WARNING( "Failed to load node file: %s\n", nd_get_error_message() );
		return NULL;
	}

	return ParseStyleSheet( root );
}

void ss_gui_set_style_sheet( const GuiStyleSheet *styleSheet )
{
	activeSheet = styleSheet;
}

const GuiStyleSheet *guiGetActiveStyleSheet( void )
{
	return activeSheet;
}

int gui_LogLevels_[ GUI_MAX_LOG_LEVELS ];

/**
 * Initialize the GUI sub-system.
 */
bool ss_gui_initialize( void )
{
	PL_ZERO_( guiState );

	gui_LogLevels_[ GUI_LOGLEVEL_DEFAULT ] = PlAddLogLevel( "gui", PL_COLOUR_LIGHT_CORAL, true );
	gui_LogLevels_[ GUI_LOGLEVEL_WARNING ] = PlAddLogLevel( "gui/warning", PL_COLOUR_YELLOW, true );
	gui_LogLevels_[ GUI_LOGLEVEL_ERROR ] = PlAddLogLevel( "gui/error", PL_COLOUR_DARK_RED, true );
	gui_LogLevels_[ GUI_LOGLEVEL_DEBUG ] = PlAddLogLevel( "gui/debug", PL_COLOUR_CRIMSON,
#ifndef NDEBUG
	                                                      true
#else
	                                                      false
#endif
	);

	guiInitializeDraw_();
	if ( !guiInitializeFonts_() )
	{
		GUI_ERROR( "Font initialization failed!\n" );
		return false;
	}

	GUI_PRINT( "GUI initialized!\n" );
	return true;
}

void ss_gui_shutdown( void )
{
	guiShutdownDraw_();

	for ( unsigned int i = 0; i < GUI_MAX_LOG_LEVELS; ++i )
		PlRemoveLogLevel( gui_LogLevels_[ i ] );
}

void gui_panel_tick( GuiPanel *root )
{
	guiTickPanel( root );
}

void guiUpdateMousePosition( int x, int y )
{
	guiState.mouseOldPos = guiState.mousePos;
	guiState.mousePos.x = x;
	guiState.mousePos.y = y;
}

void gui_update_mouse_wheel( float x, float y )
{
	guiState.mouseOldWheel = guiState.mouseWheel;
	guiState.mouseWheel.x = x;
	guiState.mouseWheel.y = y;
}

void guiUpdateMouseButton( GuiMouseButton button, bool isDown )
{
}

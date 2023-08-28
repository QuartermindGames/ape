// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_console.h>

#include <yin/node.h>

#include "gui_private.h"

/****************************************
 * GUI
 ****************************************/

GuiState guiState;

static PLLinkedList *cachedTextures;
typedef struct GuiCachedTexture {
	unsigned int hash;
	PLGTexture *texture;
} GuiCachedTexture;
PLGTexture *guiCacheTexture( const char *path ) {
	unsigned int hash = PlGenerateHashSDBM( path );
	PLLinkedListNode *node = PlGetFirstNode( cachedTextures );
	while ( node != NULL ) {
		GuiCachedTexture *cachedTexture = PlGetLinkedListNodeUserData( node );
		if ( cachedTexture->hash == hash ) {
			return cachedTexture->texture;
		}

		node = PlGetNextLinkedListNode( node );
	}

	PLGTexture *texture = PlgLoadTextureFromImage( path, PLG_TEXTURE_FILTER_LINEAR );
	if ( texture == NULL ) {
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

static GuiStyleSheet *ParseStyleSheet( NdBranch *root ) {
	GuiStyleSheet *guiStyleSheet = &styleSheets[ numStyleSheets ];
	PL_ZERO( guiStyleSheet, sizeof( GuiStyleSheet ) );

	unsigned int version = ndGetUInt( root, "version", ( unsigned int ) -1 );
	if ( version == ( unsigned int ) -1 || version > GUI_STYLESHEET_VERSION ) {
		GUI_WARNING( "Unexpected version in stylesheet, expected %d but found %d!\n", GUI_STYLESHEET_VERSION, version );
		return NULL;
	}

	NdBranch *c;
	c = ndGetChildByName( root, "colours" );
	if ( c != NULL ) {
		NdBranch *i;
		if ( ( i = ndGetChildByName( c, PL_STRINGIFY( GUI_COLOUR_INSET_BACKGROUND ) ) ) != NULL )
			ndGetF32Array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_INSET_BACKGROUND ], 4 );
		if ( ( i = ndGetChildByName( c, PL_STRINGIFY( GUI_COLOUR_OUTSET_BACKGROUND ) ) ) != NULL )
			ndGetF32Array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_OUTSET_BACKGROUND ], 4 );
		if ( ( i = ndGetChildByName( c, PL_STRINGIFY( GUI_COLOUR_INSET_BORDER_TOP ) ) ) != NULL )
			ndGetF32Array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_INSET_BORDER_TOP ], 4 );
		if ( ( i = ndGetChildByName( c, PL_STRINGIFY( GUI_COLOUR_INSET_BORDER_BOTTOM ) ) ) != NULL )
			ndGetF32Array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_INSET_BORDER_BOTTOM ], 4 );
		if ( ( i = ndGetChildByName( c, PL_STRINGIFY( GUI_COLOUR_OUTSET_BORDER_TOP ) ) ) != NULL )
			ndGetF32Array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_OUTSET_BORDER_TOP ], 4 );
		if ( ( i = ndGetChildByName( c, PL_STRINGIFY( GUI_COLOUR_OUTSET_BORDER_BOTTOM ) ) ) != NULL )
			ndGetF32Array( i, ( float * ) &guiStyleSheet->colours[ GUI_COLOUR_OUTSET_BORDER_BOTTOM ], 4 );
	}

	c = ndGetChildByName( root, "borders" );
	if ( c != NULL ) {
		unsigned int style = ndGetUInt( c, "style", -1 );
		if ( style < GUI_MAX_BORDER_STYLES )
			guiStyleSheet->borderStyle = style;
		else
			GUI_WARNING( "No border style specified, using default.\n" );

		NdBranch *i;
		if ( ( i = ndGetChildByName( c, "padding" ) ) != NULL )
			ndGetI32Array( i, guiStyleSheet->borderPadding, GUI_MAX_BORDER_ELEMENTS );
	}

	return guiStyleSheet;
}

const GuiStyleSheet *guiCacheStyleSheet( const char *path ) {
	NdBranch *root = ndLoadFile( path, "guiStyle" );
	if ( root == NULL ) {
		GUI_WARNING( "Failed to load node file: %s\n", ndGetErrorMessage() );
		return NULL;
	}

	return ParseStyleSheet( root );
}

void guiSetStyleSheet( const GuiStyleSheet *styleSheet ) {
	activeSheet = styleSheet;
}

const GuiStyleSheet *guiGetActiveStyleSheet( void ) {
	return activeSheet;
}

int gui_LogLevels_[ GUI_MAX_LOG_LEVELS ];

/**
 * Initialize the GUI sub-system.
 */
bool guiInitialize( void ) {
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
	if ( !guiInitializeFonts_() ) {
		GUI_ERROR( "Font initialization failed!\n" );
		return false;
	}

	GUI_PRINT( "GUI initialized!\n" );
	return true;
}

void guiShutdown( void ) {
	guiShutdownDraw_();

	for ( unsigned int i = 0; i < GUI_MAX_LOG_LEVELS; ++i )
		PlRemoveLogLevel( gui_LogLevels_[ i ] );
}

void guiTick( GuiPanel *root ) {
	guiTickPanel( root );
}

void guiUpdateMousePosition( int x, int y ) {
	guiState.mouseOldPos = guiState.mousePos;
	guiState.mousePos.x = x;
	guiState.mousePos.y = y;
}

void guiUpdateMouseWheel( float x, float y ) {
	guiState.mouseOldWheel = guiState.mouseWheel;
	guiState.mouseWheel.x = x;
	guiState.mouseWheel.y = y;
}

void guiUpdateMouseButton( GuiMouseButton button, bool isDown ) {
}

/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "renderer/renderer.h"
#include "renderer/font.h"

static bool isConsoleOpen = false;

static float consoleHeight = 0.0f;
static unsigned int scrollPos = 0;

/* console buffer methods */
#define CON_BUFFER_MAX_LENGTH 256
#define CON_BUFFER_MAX_LINES 4096
static char inputBuffer[ CON_BUFFER_MAX_LENGTH ] = { '\0' };
static struct ConBuffer {
	char buffer[ CON_BUFFER_MAX_LINES ][ CON_BUFFER_MAX_LENGTH ];
	unsigned int numLines;
} outputBuffer = {
        .numLines = 0,
};
static void Con_ClearBuffer( void ) { outputBuffer.numLines = 0; }
static void Con_PushLine( const char *msg ) {
	size_t l = strlen( msg );
	if ( l >= CON_BUFFER_MAX_LENGTH ) {
		PrintWarn( "Attempting to push message to console with an unexpected length!\n" );
		l = CON_BUFFER_MAX_LENGTH - 2;
	}

	strncpy( outputBuffer.buffer[ outputBuffer.numLines ], msg, l );
	outputBuffer.buffer[ outputBuffer.numLines ][ l ] = '\0';

	/* this is when we do what is probably going to be,
	 * a dumb and expensive operation... */
	outputBuffer.numLines++;
	if ( outputBuffer.numLines >= CON_BUFFER_MAX_LINES ) {
		//memmove_s()
	}
}

static void Con_OutputCallback( int level, const char *msg ) {
	u_unused( level );
	Con_PushLine( msg );
}

/* CONSOLE COMMANDS */

#define CMD_CALLBACK( NAME ) static void Cmd_##NAME( unsigned int argc, char **argv )
CMD_CALLBACK( ClearConsole ) {
	u_unused( argc );
	u_unused( argv );
	Con_ClearBuffer();
}
CMD_CALLBACK( ToggleConsole ) {
	u_unused( argc );
	u_unused( argv );
	Con_Toggle();
}

/*------------------------------------------------------------------*/

/**
 * Set the console up.
 */
void Con_Initialize( void ) {
	plSetConsoleOutputCallback( Con_OutputCallback );

	plRegisterConsoleVariable( "map.sky.material", "materials/sky/cloudlayer00.mat", pl_string_var, NULL, "Sets the sky material." );

	plRegisterConsoleVariable( "console.background", "", pl_string_var, NULL, "Background to use for the console." );
	plRegisterConsoleVariable( "console.alpha", "128", pl_int_var, NULL, "Level of transparency to use for the console background." );
	plRegisterConsoleVariable( "console.height", "512", pl_int_var, NULL, "Set the height of the console." );

	plRegisterConsoleCommand( "console.clear", Cmd_ClearConsole, "Clear the console buffer." );
	plRegisterConsoleCommand( "console.toggle", Cmd_ToggleConsole, "Toggle the console." );
}

void Con_Shutdown( void ) {
	Con_ClearBuffer();
}

static void Con_Animate( void *userData, double delta ) {
	u_unused( delta );
	u_unused( userData );

#define SPEED 16.0f
	if ( isConsoleOpen && ( consoleHeight < 512.0f )  ) {
		consoleHeight += SPEED;
		Sch_PushTask( "conanim", Con_Animate, NULL, 1 );
	} else if ( !isConsoleOpen && consoleHeight > 0.0f ) {
		consoleHeight -= SPEED;
		Sch_PushTask( "conanim", Con_Animate, NULL, 1 );
	}

	if ( consoleHeight > 512.0f ) { consoleHeight = 512.0f; }
	else if ( consoleHeight < 0.0f ) { consoleHeight = 0.0f; }
}

/**
 * Toggle the console state.
 */
void Con_Toggle( void ) {
	static unsigned int toggleTime = 0;
	if ( toggleTime > Engine_GetNumTicks() ) {
		return;
	}

	isConsoleOpen = !isConsoleOpen;

	Sch_PushTask( "conanim", Con_Animate, NULL, 2 );

	toggleTime = Engine_GetNumTicks() + 16;
}

void Con_ScrollForward( void ) {
	scrollPos++;
	if ( scrollPos > outputBuffer.numLines - 1 ) {
		scrollPos = outputBuffer.numLines - 1;
	}
}

void Con_ScrollBackward( void ) {
	if ( scrollPos == 0 ) {
		return;
	}
	scrollPos--;
}

/**
 * Returns the current console state, e.g. is it open?
 */
bool Con_GetState( void ) {
	return isConsoleOpen;
}

/**
 * Draw the console panel.
 */
void Con_Draw( const PLViewport *viewport ) {
	static float oldConsoleHeight = 0.0f;

	if ( consoleHeight <= 0.0f ) {
		return;
	}

	static PLConsoleVariable *alpha = NULL,
	                         *background = NULL;
	if ( alpha == NULL ) { alpha = plGetConsoleVariable( "console.alpha" ); }
	if ( background == NULL ) { background = plGetConsoleVariable( "console.background" ); }

	plSetBlendMode( PL_BLEND_DEFAULT );

	plMatrixMode( PL_MODELVIEW_MATRIX );
	plPushMatrix();

	plLoadIdentityMatrix();

	float w = viewport->w - 4.0f;

	plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT_VERTEX ] );

#define CON_TEXT_COLOUR         PLColourRGB( 0, 255, 0 )
#define CON_SIDE_COLOUR         PLColourRGB( 128, 128, 128 )
#define CON_BACK_COLOUR         PLColour( 0, 0, 0, alpha->i_value )
#define CON_INDICATOR_COLOUR    PLColourRGB( 255, 255, 255 )

	plDrawRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 2.0f, 2.0f, w, consoleHeight, CON_BACK_COLOUR );
	plDrawRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 2.0f, 2.0f, 8, consoleHeight, CON_SIDE_COLOUR );
	plDrawRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 0.0f, viewport->h - 12.0f, viewport->w, 12.0f, CON_BACK_COLOUR );

	if ( outputBuffer.numLines > 0 ) {
		/* indicate where we are in the list */
		float cH = ( outputBuffer.numLines / consoleHeight ) + 1.0f;
		float cY = consoleHeight - ( ( outputBuffer.numLines / consoleHeight ) + scrollPos ) - cH;
		plDrawRectangle( plGetMatrix( PL_MODELVIEW_MATRIX ), 2.0f, cY, 8.0f, cH, CON_INDICATOR_COLOUR );

		plSetShaderProgram( gfxDefaultShaderPrograms[ GFX_SHADER_DEFAULT ] );

		float y = consoleHeight - 20.0f;
		for ( unsigned int i = ( outputBuffer.numLines - 1 ) - scrollPos; i > 0; --i ) {
			Font_DrawBitmapString( 12.0f, y, 1.0f, 1.0f, CON_TEXT_COLOUR, outputBuffer.buffer[ i ], true );
			y -= 12.0f;
			if ( y <= -12.0f ) {
				break;
			}
		}
	}

	/* draw input field */
	Font_DrawBitmapCharacter( 1.0f, viewport->h - 12.0f, 1.0f, CON_TEXT_COLOUR, '>' );
	Font_DrawBitmapString( 2.0f, viewport->h - 12.0f, 1.0f, 1.0f, CON_TEXT_COLOUR, inputBuffer, 0 );

	plPopMatrix();

	plSetBlendMode( PL_BLEND_DISABLE );
}

// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Main Forge file.
// Author:  Mark E. Sowden

#include <gtk/gtk.h>

#include "forge.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static GtkWidget *mainWindow;

static GtkWidget *setup_main_window( void )
{
	GtkWidget *window = gtk_window_new();
	gtk_window_set_title( GTK_WINDOW( window ), forge_get_window_title() );

	gtk_window_present( GTK_WINDOW( window ) );

	return window;
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void forge_message_box( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );

	int length = pl_vscprintf( msg, args ) + 1;
	if ( length <= 0 )
		return;

	char *buf = PL_NEW_( char, length );
	vsnprintf( buf, length, msg, args );
	va_end( args );

	GtkAlertDialog *dialog = gtk_alert_dialog_new( "%s", buf );
	gtk_alert_dialog_show( dialog, GTK_WINDOW( mainWindow ) );

	PL_DELETE( buf );
}

const char *forge_get_window_title( void )
{
	//TODO: if level is loaded, should be "Forge v1.2 - <level.rfl>"
	static char tmp[ 128 ];
	snprintf( tmp, sizeof( tmp ), FORGE_APP_TITLE " v%u.%u", FORGE_APP_VERSION_MAJOR, FORGE_APP_VERSION_MINOR );
	return tmp;
}

int main( int argc, char **argv )
{
	gtk_init();

	forge_message_box( "blah blah" );

	mainWindow = setup_main_window();
	if ( mainWindow == NULL )
	{
		forge_message_box( "Failed to create main window!\n" );
		return EXIT_FAILURE;
	}

	// now invoke our main loop
	while ( g_list_model_get_n_items( gtk_window_get_toplevels() ) > 0 )
		g_main_context_iteration( NULL, TRUE );

	return EXIT_SUCCESS;
}

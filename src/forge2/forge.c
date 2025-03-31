#include <adwaita.h>

#include "forge.h"

static void activate( GtkApplication *app, gpointer user_data )
{
	GtkWidget *window = gtk_application_window_new( app );
	gtk_window_set_title( GTK_WINDOW( window ), "Forge" );
	gtk_window_set_default_size( GTK_WINDOW( window ), 800, 600 );
	gtk_window_maximize( GTK_WINDOW( window ) );

	gtk_window_present( GTK_WINDOW( window ) );
}

int main( int argc, char **argv )
{
	AdwApplication *app = adw_application_new( "com.quartermind.forge", G_APPLICATION_DEFAULT_FLAGS );

	g_signal_connect( app, "activate", G_CALLBACK( activate ), NULL );

	int status = g_application_run( G_APPLICATION( app ), argc, argv );
	g_object_unref( app );

	return status;
}

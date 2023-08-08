// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "worlded.h"

static GtkWidget *mainWindow;

static void QuitAction( GSimpleAction *action, GVariant *parameter, GApplication *application )
{
	g_application_quit( application );
}

static void ActivateApp( GApplication *app, gpointer *data )
{
	mainWindow = gtk_application_window_new( GTK_APPLICATION( app ) );
	gtk_window_set_title( GTK_WINDOW( mainWindow ), "APE World Editor" );
	gtk_window_set_default_size( GTK_WINDOW( mainWindow ), 1024, 768 );
	gtk_window_maximize( GTK_WINDOW( mainWindow ) );

	gtk_window_present( GTK_WINDOW( mainWindow ) );
}

int main( int argc, char **argv )
{
	GtkApplication *app = gtk_application_new( "com.ape.world_editor", G_APPLICATION_DEFAULT_FLAGS );
	g_signal_connect( app, "activate", G_CALLBACK( ActivateApp ), NULL );

	int status = g_application_run( G_APPLICATION( app ), argc, argv );

	g_object_unref( app );

	return status;
}

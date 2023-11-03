// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Main Forge file.
// Author:  Mark E. Sowden

#include <gtk/gtk.h>

#include "forge.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static GtkWidget *mainWindow;

static void quit_activated( PL_UNUSED GSimpleAction *action, PL_UNUSED GVariant *parameter, GApplication *application )
{
	g_application_quit( application );
}

static void destroy_message_box( GtkWindow *window, PL_UNUSED GdkEvent *event, char *buf )
{
	PL_DELETE( buf );
	gtk_window_destroy( window );
}

static void destroy_message_box_abort( GtkWindow *window, PL_UNUSED GdkEvent *event, char *buf )
{
	destroy_message_box( window, event, buf );
	abort();
}

static void setup_file_menu( GMenu *menubar )
{
	GMenu *menu = g_menu_new();

	GMenuItem *fileMenu = g_menu_item_new( "File", NULL );
	GMenu *section1 = g_menu_new();

	GMenuItem *childItem;
	childItem = g_menu_item_new( "New", "app.new" );
	g_menu_append_item( section1, childItem );
	g_object_unref( childItem );
	childItem = g_menu_item_new( "Open", "app.open" );
	g_menu_append_item( section1, childItem );
	g_object_unref( childItem );
	childItem = g_menu_item_new( "Save", "app.save" );
	g_menu_append_item( section1, childItem );
	g_object_unref( childItem );
	childItem = g_menu_item_new( "Save As...", "app.save-as" );
	g_menu_append_item( section1, childItem );
	g_object_unref( childItem );
	g_menu_append_section( menu, NULL, G_MENU_MODEL( section1 ) );

	childItem = g_menu_item_new( "Quit", "app.quit" );
	g_menu_append_item( menu, childItem );
	g_object_unref( childItem );

	g_menu_item_set_submenu( fileMenu, G_MENU_MODEL( menu ) );
	g_menu_append_item( menubar, fileMenu );

	g_object_unref( fileMenu );
	g_object_unref( section1 );
	g_object_unref( menu );
}

static void setup_edit_menu( GMenu *menubar )
{
	GMenu *menu = g_menu_new();

	GMenuItem *editItem = g_menu_item_new( "Edit", NULL );
	g_menu_item_set_submenu( editItem, G_MENU_MODEL( menu ) );
	g_menu_append_item( menubar, editItem );

	g_object_unref( menu );
	g_object_unref( editItem );
}

static void setup_view_menu( GMenu *menubar )
{
	GMenu *menu = g_menu_new();

	GMenuItem *editItem = g_menu_item_new( "View", NULL );
	g_menu_item_set_submenu( editItem, G_MENU_MODEL( menu ) );
	g_menu_append_item( menubar, editItem );

	g_object_unref( menu );
	g_object_unref( editItem );
}

static void setup_tools_menu( GMenu *menubar )
{
	GMenu *menu = g_menu_new();

	GMenuItem *editItem = g_menu_item_new( "Tools", NULL );
	g_menu_item_set_submenu( editItem, G_MENU_MODEL( menu ) );
	g_menu_append_item( menubar, editItem );

	g_object_unref( menu );
	g_object_unref( editItem );
}

static void about_activated( GSimpleAction *action, GVariant *parameter, GApplication *application )
{
	GtkAboutDialog *aboutDialog = gtk_about_dialog_new();
}

static void setup_help_menu( GtkApplication *app, GMenu *menubar )
{
	GMenu *menu = g_menu_new();

	GSimpleAction *aboutAction = g_simple_action_new( "about", NULL );
	g_action_map_add_action( G_ACTION_MAP( app ), G_ACTION( aboutAction ) );
	g_signal_connect( aboutAction, "activate", G_CALLBACK( about_activated ), app );

	GMenuItem *aboutItem = g_menu_item_new( "About", "app.about" );
	g_menu_append_item( menu, aboutItem );

	GMenuItem *editItem = g_menu_item_new( "Help", NULL );
	g_menu_item_set_submenu( editItem, G_MENU_MODEL( menu ) );
	g_menu_append_item( menubar, editItem );

	g_object_unref( menu );
	g_object_unref( aboutItem );
	g_object_unref( editItem );
}

static void app_startup( GApplication *application )
{
	GtkApplication *app = GTK_APPLICATION( application );

	GSimpleAction *quitAction = g_simple_action_new( "quit", NULL );
	g_action_map_add_action( G_ACTION_MAP( app ), G_ACTION( quitAction ) );
	g_signal_connect( quitAction, "activate", G_CALLBACK( quit_activated ), app );

	GMenu *menubar = g_menu_new();
	setup_file_menu( menubar );
	setup_edit_menu( menubar );
	setup_view_menu( menubar );
	setup_tools_menu( menubar );
	setup_help_menu( app, menubar );

	gtk_application_set_menubar( GTK_APPLICATION( app ), G_MENU_MODEL( menubar ) );
}


static void app_activate( GApplication *application )
{
	GtkApplication *app = GTK_APPLICATION( application );
	mainWindow = gtk_application_window_new( app );
	gtk_window_set_title( GTK_WINDOW( mainWindow ), forge_get_window_title() );
	gtk_application_window_set_show_menubar( GTK_APPLICATION_WINDOW( mainWindow ), TRUE );

	forge_project_show_selector();
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

GtkWindow *forge_get_main_window( void )
{
	return GTK_WINDOW( mainWindow );
}

void forge_message_box( GtkWindow *window, GtkMessageType type, const char *msg, ... )
{
	va_list args;
	va_start( args, msg );

	int length = pl_vscprintf( msg, args ) + 1;
	if ( length <= 0 )
		return;

	char *buf = PL_NEW_( char, length );
	vsnprintf( buf, length, msg, args );
	va_end( args );

	GtkWidget *messageDialog = gtk_message_dialog_new( window,
	                                                   GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
	                                                   type, GTK_BUTTONS_CLOSE,
	                                                   buf );
	gtk_window_set_title( GTK_WINDOW( messageDialog ), "Forge Alert" );
	if ( type == GTK_MESSAGE_ERROR )
		g_signal_connect( messageDialog, "response", G_CALLBACK( destroy_message_box_abort ), buf );
	else
		g_signal_connect( messageDialog, "response", G_CALLBACK( destroy_message_box ), buf );

	gtk_widget_show( messageDialog );
}

const char *forge_get_window_title( void )
{
	//TODO: if level is loaded, should be "Forge v1.2 - <level.rfl>"
	static char tmp[ 128 ];
	snprintf( tmp, sizeof( tmp ), FORGE_APP_TITLE " v%u.%u (%s)",
	          FORGE_APP_VERSION_MAJOR,
	          FORGE_APP_VERSION_MINOR,
	          FORGE_APP_VERSION_BUILD );
	return tmp;
}

int main( int argc, char **argv )
{
	if ( PlInitialize( argc, argv ) != PL_RESULT_SUCCESS )
	{
		printf( "Failed to initialize plcore: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	com_initialize();

	PlMountLocalLocation( comGetAppDataDirectory() );
	PlMountLocalLocation( comGetDataDirectory() );

	GtkApplication *app = gtk_application_new( "com.oldtimes-software.ape.forge", G_APPLICATION_DEFAULT_FLAGS );
	g_signal_connect( app, "startup", G_CALLBACK( app_startup ), NULL );
	g_signal_connect( app, "activate", G_CALLBACK( app_activate ), NULL );

	int status = g_application_run( G_APPLICATION( app ), argc, argv );
	g_object_unref( app );
	return status;
}

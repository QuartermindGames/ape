// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Graft editor.
// Author:  Mark E. Sowden

#include "graft.h"

typedef struct GraftSplashScreen
{
	GtkWidget *window;
	GtkWidget *progressBar;
} GraftSplashScreen;

GtkWidget *graft_splash_screen_create( GtkWidget *parent )
{
	GtkWidget *stack = gtk_stack_new();
	gtk_stack_set_transition_type( GTK_STACK( stack ), GTK_STACK_TRANSITION_TYPE_CROSSFADE );
	gtk_stack_set_transition_duration( GTK_STACK( stack ), 500 );
	gtk_window_set_child( GTK_WINDOW( parent ), stack );

	GtkWidget *splashBox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 15 );
	gtk_widget_set_halign( splashBox, GTK_ALIGN_CENTER );
	gtk_widget_set_valign( splashBox, GTK_ALIGN_CENTER );

	char tmp[ 64 ];
	snprintf( tmp, sizeof( tmp ), "%s is loading...", GRAFT_NAME );
	GtkWidget *label = gtk_label_new( tmp );
	gtk_box_append( GTK_BOX( splashBox ), label );

	GtkWidget *progress = gtk_progress_bar_new();
	gtk_widget_set_size_request( progress, 300, -1 );
	gtk_box_append( GTK_BOX( splashBox ), progress );
	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( progress ), 0.5 );

	gtk_stack_add_named( GTK_STACK( stack ), splashBox, "splash" );

	GraftSplashScreen *splash = g_new0( GraftSplashScreen, 1 );
	splash->window            = stack;
	splash->progressBar       = progress;

	return stack;
}

static GtkApplication *graftApp;

static void graft_quit_action( GSimpleAction *action, GVariant *parameter, gpointer userData )
{
	g_application_quit( G_APPLICATION( graftApp ) );
}

static void graft_activate( GtkApplication *app, gpointer userData )
{
	GtkWidget *window = gtk_application_window_new( app );
	gtk_window_set_title( GTK_WINDOW( window ), GRAFT_NAME );
	gtk_window_set_default_size( GTK_WINDOW( window ), 1024, 768 );
	gtk_window_maximize( GTK_WINDOW( window ) );

	GtkWidget *header = gtk_header_bar_new();
	gtk_window_set_titlebar( GTK_WINDOW( window ), header );

	// menu
	{
		GMenu *menu = g_menu_new();

		// file
		{
			GMenu *fileMenu = g_menu_new();

			g_menu_append( fileMenu, "New Room...", "app.new_room" );
			GSimpleAction *newRoomAction = g_simple_action_new( "new_room", nullptr );
			g_signal_connect( newRoomAction, "activate", G_CALLBACK( graft_quit_action ), app );
			g_action_map_add_action( G_ACTION_MAP( app ), G_ACTION( newRoomAction ) );

			g_menu_append( fileMenu, "Open Room...", "app.open_room" );
			GSimpleAction *openRoomAction = g_simple_action_new( "open_room", nullptr );
			g_signal_connect( openRoomAction, "activate", G_CALLBACK( graft_quit_action ), app );
			g_action_map_add_action( G_ACTION_MAP( app ), G_ACTION( openRoomAction ) );

			g_menu_append( fileMenu, "Quit", "app.quit" );
			GSimpleAction *quitAction = g_simple_action_new( "quit", nullptr );
			g_signal_connect( quitAction, "activate", G_CALLBACK( graft_quit_action ), app );
			g_action_map_add_action( G_ACTION_MAP( app ), G_ACTION( quitAction ) );

			g_menu_append_submenu( menu, "File", G_MENU_MODEL( fileMenu ) );
		}

		GtkWidget *menuButton = gtk_menu_button_new();
		gtk_menu_button_set_icon_name( GTK_MENU_BUTTON( menuButton ), "open-menu-symbolic" );
		gtk_menu_button_set_menu_model( GTK_MENU_BUTTON( menuButton ), G_MENU_MODEL( menu ) );

		gtk_header_bar_pack_end( GTK_HEADER_BAR( header ), menuButton );
	}

	//GtkWidget *splashScreen = graft_splash_screen_create( window );

	//GtkWidget *editor_label = gtk_label_new( "Main Editor Interface Goes Here" );
	//gtk_stack_add_named( GTK_STACK( splashScreen ), editor_label, "editor" );

	//gtk_stack_set_visible_child_name( GTK_STACK( splashScreen ), "splash" );

	gtk_window_present( GTK_WINDOW( window ) );
}

int qm_os_main( const int argc, char **argv )
{
	graftApp = gtk_application_new( "com.quartermind.graft", G_APPLICATION_DEFAULT_FLAGS );
	g_signal_connect( graftApp, "activate", G_CALLBACK( graft_activate ), nullptr );

	int status = g_application_run( G_APPLICATION( graftApp ), argc, argv );

	g_object_unref( graftApp );

	return status;
}

QM_OS_SYSTEM_IMPLEMENT_MAIN()

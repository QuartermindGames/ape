// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>
// PKGGUI: GUI interface for pkgman using GTK

#include "pkggui.h"

static GtkWidget *mainWindow;

//////////////////////////////////////////////////////////////////////////////////
// Package Creation

static void CreateNewPackage( void )
{
	// Pick the source folder
	char *folder = NULL;
	{
		GtkWidget *dialog = gtk_file_chooser_dialog_new( "Select Folder",
		                                                 GTK_WINDOW( mainWindow ),
		                                                 GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		                                                 "_Cancel",
		                                                 GTK_RESPONSE_CANCEL,
		                                                 "_Open",
		                                                 GTK_RESPONSE_ACCEPT,
		                                                 NULL );
		if ( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT )
			folder = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );

		gtk_widget_destroy( dialog );
	}

	if ( folder == NULL )
		return;

	// And now pick the destination
	char *destination = NULL;
	{
		GtkWidget *dialog = gtk_file_chooser_dialog_new( "Specify Destination",
		                                                 GTK_WINDOW( mainWindow ),
		                                                 GTK_FILE_CHOOSER_ACTION_SAVE,
		                                                 "_Cancel",
		                                                 GTK_RESPONSE_CANCEL,
		                                                 "_Save",
		                                                 GTK_RESPONSE_ACCEPT,
		                                                 NULL );
		if ( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT )
			destination = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );

		gtk_widget_destroy( dialog );
	}

	if ( destination == NULL )
	{
		g_free( folder );
		return;
	}

	FILE *out = fopen( destination, "wb" );
	if ( out == NULL )
	{
		ShowMessageBox( "Failed to write to destination!", false );
		g_free( folder );
		g_free( destination );
		return;
	}

	progressWindow = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title( GTK_WINDOW( progressWindow ), "Generating Package..." );
	gtk_window_set_default_size( GTK_WINDOW( progressWindow ), 200, 100 );
	gtk_window_set_position( GTK_WINDOW( progressWindow ), GTK_WIN_POS_CENTER );
	gtk_window_set_resizable( GTK_WINDOW( progressWindow ), FALSE );
	gtk_window_set_modal( GTK_WINDOW( progressWindow ), TRUE );
	gtk_window_set_transient_for( GTK_WINDOW( progressWindow ), GTK_WINDOW( mainWindow ) );

	GtkWidget *vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 8 );
	gtk_container_add( GTK_CONTAINER( progressWindow ), vbox );
	GtkWidget *spinner = gtk_spinner_new();
	gtk_spinner_start( GTK_SPINNER( spinner ) );
	gtk_box_pack_start( GTK_BOX( vbox ), spinner, TRUE, TRUE, 8 );
	progressBar = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text( GTK_PROGRESS_BAR( progressBar ), true );
	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( progressBar ), "Building file tree..." );
	gtk_box_pack_start( GTK_BOX( vbox ), progressBar, TRUE, TRUE, 8 );

	gtk_widget_show_all( progressWindow );

	// Gather all the files we're going to pack
	PLLinkedList *fileList = PlCreateLinkedList();
	PlScanDirectory( folder, NULL, IndexFile, true, fileList );

	// Fill out the header for the package
	unsigned int numFiles = PlGetNumLinkedListNodes( fileList );
	Common_Pkg_WriteHeader( out, numFiles );

	gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( progressBar ), 0.0 );
	gtk_main_iteration_do( false );

	PLPath tmpPath;
	if ( tmpnam_r( tmpPath ) == NULL )
	{
		Print( "Failed to get temporary location, using './temp' instead...\n" );
		snprintf( tmpPath, sizeof( tmpPath ), "./temp" );
	}

	PLLinkedListNode *node = PlGetFirstNode( fileList );
	for ( unsigned int i = 0;; ++i )
	{
		if ( node == NULL )
			break;

		char tmp[ 64 ];
		snprintf( tmp, sizeof( tmp ), "%u/%u", ( i + 1 ), numFiles );
		gtk_progress_bar_set_text( GTK_PROGRESS_BAR( progressBar ), tmp );
		gtk_progress_bar_set_fraction( GTK_PROGRESS_BAR( progressBar ), ( double ) ( i + 1 ) / numFiles );
		gtk_main_iteration_do( false );

		//TODO: bail when this fails...
		char   *store = PlGetLinkedListNodeUserData( node );
		PLFile *in = PlOpenLocalFile( store, true );
		if ( in != NULL )
		{
			PLFile *newFile = NodeToBin( in, tmpPath );
			if ( newFile != NULL )
			{
				PlCloseFile( in );
				in = newFile;
			}

			Common_Pkg_AddData( out, store + ( strlen( folder ) + 1 ), PlGetFileData( in ), PlGetFileSize( in ) );

			PlCloseFile( in );
		}

		PL_DELETE( store );
		node = PlGetNextLinkedListNode( node );
	}
	PlDestroyLinkedList( fileList );

	// Don't need our temporary cache anymore
	PlDeleteFile( tmpPath );

	fclose( out );

	g_free( folder );

	gtk_progress_bar_set_text( GTK_PROGRESS_BAR( progressBar ), "Opening..." );
	gtk_main_iteration_do( false );

	package = OpenPackage( destination );
	g_free( destination );

	gtk_widget_destroy( progressWindow );
}

static GtkWidget *CreateTreeView( void )
{
	GtkWidget *view = gtk_tree_view_new();

	GtkCellRenderer *renderer;
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
	                                             -1,
	                                             "Name",
	                                             renderer,
	                                             "text", TREE_COLUMN_NAME,
	                                             NULL );
	gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
	                                             -1,
	                                             "Size",
	                                             renderer,
	                                             "text", TREE_COLUMN_SIZE,
	                                             NULL );
	gtk_tree_view_insert_column_with_attributes( GTK_TREE_VIEW( view ),
	                                             -1,
	                                             "Compressed Size",
	                                             renderer,
	                                             "text", TREE_COLUMN_COMPRESSED_SIZE,
	                                             NULL );
	//gtk_tree_view_column_cell_set_cell_data()

	treeListStore = gtk_list_store_new( MAX_TREE_COLUMNS,
	                                    G_TYPE_STRING,
	                                    G_TYPE_UINT64,
	                                    G_TYPE_UINT64 );

	GtkTreeModel *model = GTK_TREE_MODEL( treeListStore );
	gtk_tree_view_set_model( GTK_TREE_VIEW( view ), model );
	g_object_unref( model );

	return view;
}

static GtkWidget *CreateMenus( void )
{
	GtkWidget *menuBar = gtk_menu_bar_new();
	GtkWidget *fileMenu = gtk_menu_new();
	{
		GtkWidget *fileMenuItem = gtk_menu_item_new_with_label( "File" );
		gtk_menu_item_set_submenu( GTK_MENU_ITEM( fileMenuItem ), fileMenu );

		GtkWidget *newMenuItem = gtk_menu_item_new_with_label( "New" );
		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), newMenuItem );
		g_signal_connect( G_OBJECT( newMenuItem ), "activate", G_CALLBACK( CreateNewPackage ), NULL );

		GtkWidget *openMenuItem = gtk_menu_item_new_with_label( "Open" );
		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), openMenuItem );
		g_signal_connect( G_OBJECT( openMenuItem ), "activate", G_CALLBACK( ShowOpenDialog ), NULL );

		GtkWidget *closeMenuItem = gtk_menu_item_new_with_label( "Close" );
		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), closeMenuItem );
		g_signal_connect( G_OBJECT( closeMenuItem ), "activate", G_CALLBACK( ClosePackage ), NULL );

		GtkWidget *extractMenuItem = gtk_menu_item_new_with_label( "Extract..." );
		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), extractMenuItem );
		g_signal_connect( G_OBJECT( extractMenuItem ), "activate", G_CALLBACK( ExtractPackage ), NULL );

		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), gtk_separator_menu_item_new() );

		/*
		GtkWidget *saveMenuItem = gtk_menu_item_new_with_label( "Save" );
		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), saveMenuItem );

		GtkWidget *saveAsMenuItem = gtk_menu_item_new_with_label( "Save As" );
		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), saveAsMenuItem );
		 */

		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), gtk_separator_menu_item_new() );

		GtkWidget *quitMenuItem = gtk_menu_item_new_with_label( "Quit" );
		gtk_menu_shell_append( GTK_MENU_SHELL( fileMenu ), quitMenuItem );
		g_signal_connect( G_OBJECT( quitMenuItem ), "activate", G_CALLBACK( gtk_main_quit ), NULL );

		gtk_menu_shell_append( GTK_MENU_SHELL( menuBar ), fileMenuItem );
	}
	GtkWidget *helpMenu = gtk_menu_new();
	{
		GtkWidget *helpMenuItem = gtk_menu_item_new_with_label( "Help" );
		gtk_menu_item_set_submenu( GTK_MENU_ITEM( helpMenuItem ), helpMenu );

		GtkWidget *aboutMenuItem = gtk_menu_item_new_with_label( "About" );
		gtk_menu_shell_append( GTK_MENU_SHELL( helpMenu ), aboutMenuItem );
		g_signal_connect( G_OBJECT( aboutMenuItem ), "activate", G_CALLBACK( ShowAbout ), NULL );

		gtk_menu_shell_append( GTK_MENU_SHELL( menuBar ), helpMenuItem );
	}

	return menuBar;
}

int main( int argc, char **argv )
{
	PlInitialize( argc, argv );

	Common_Initialize();

	PlRegisterStandardPackageLoaders();

	gtk_init( &argc, &argv );

	mainWindow = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title( GTK_WINDOW( mainWindow ), "PkgGUI" );
	gtk_window_set_default_size( GTK_WINDOW( mainWindow ), 400, 512 );
	gtk_window_set_position( GTK_WINDOW( mainWindow ), GTK_WIN_POS_CENTER );

	g_signal_connect( mainWindow, "destroy", G_CALLBACK( gtk_main_quit ), NULL );

	GtkWidget *vbox = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_container_add( GTK_CONTAINER( mainWindow ), vbox );
	gtk_box_pack_start( GTK_BOX( vbox ), CreateMenus(), FALSE, FALSE, 0 );
	GtkWidget *scrolledWindow = gtk_scrolled_window_new( NULL, NULL );
	gtk_container_add( GTK_CONTAINER( scrolledWindow ), CreateTreeView() );
	gtk_box_pack_start( GTK_BOX( vbox ), scrolledWindow, TRUE, TRUE, 0 );

	gtk_widget_show_all( mainWindow );

	gtk_main();

	return EXIT_SUCCESS;
}

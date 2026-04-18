// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_driver_interface.h>

#include "qmos/public/qm_os_string.h"

#include "forge.h"
#include "forge_window_main.h"
#include "forge_project_dialog.h"

#include "aux/public/aux_project.h"

#include <FXGLVisual.h>

// Override C++ new/delete operators, so we can track memory usage
#if 0//TODO: causing pain on win32 target, let's not bother for now
void *operator new( size_t size ) { return QM_OS_MEMORY_NEW_( char, size ); }
void *operator new[]( size_t size ) { return QM_OS_MEMORY_NEW_( char, size ); }
void operator delete( void *p ) noexcept { qm_os_memory_free( p ); }
void operator delete[]( void *p ) noexcept { qm_os_memory_free( p ); }
#endif

int editorLogLevels[ EDITOR_MAX_LOG_LEVELS ];

PLPath     forge::cachedPaths[ MAX_CACHED_PATHS ] = {};
AcmBranch *forge::editorConfig;

static FXGLVisual *glVisual = nullptr;
FXGLVisual        *forge::get_shared_gl_visual() { return glVisual; }

bool forge::isCookAvailable = true;

forge::Project *forge::editorProject = nullptr;

static std::map< std::string, QmImage * > cachedImages;

FXColor forge::themeColours[ MAX_THEME_COLOURS ]{};

void forge_print_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( editorLogLevels[ EDITOR_LOG_PRINT ], buf );
}

void forge_warning_( const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( editorLogLevels[ EDITOR_LOG_WARNING ], buf );
}

void forge_error_( bool die, const char *message, ... )
{
	va_list args;
	va_start( args, message );
	char buf[ 2048 ];
	vsnprintf( buf, sizeof( buf ), message, args );
	va_end( args );

	PlLogMessage( editorLogLevels[ EDITOR_LOG_ERROR ], buf );

	if ( die )
	{
		abort();
	}
}

char *forge_dialog_save( void *self, const char *title, const char *extension, const char *origin )
{
	size_t extensionSize = strlen( extension );
	char   pattern[ extensionSize + 2 ];
	snprintf( pattern, sizeof( pattern ), "*%s", extension );

	FXString saveName = FXFileDialog::getSaveFilename( ( FXWindow * ) self, title, origin, pattern );
	if ( saveName.empty() )
	{
		return nullptr;
	}

	// ensure the extension is appended, if missing
	char *filename;
	if ( saveName.length() >= extensionSize && strcmp( &saveName[ saveName.length() - extensionSize ], extension ) != 0 )
	{
		filename = qm_os_string_alloc( "%s%s", saveName.text(), extension );
	}
	else
	{
		filename = qm_os_string_alloc( "%s", saveName.text() );
	}

	return filename;
}

#if USE_GTK

typedef struct ForgeDialog
{
	char      *filename;
	gboolean   isCompleted;
	GMainLoop *loop;
} ForgeDialog;

static void on_dialog_response( GtkDialog *dialog, const gint responseId, const gpointer userData )
{
	ForgeDialog *forgeDialog = ( ForgeDialog * ) userData;
	if ( responseId == GTK_RESPONSE_ACCEPT )
	{
		forgeDialog->isCompleted = TRUE;
	}

	gtk_window_destroy( GTK_WINDOW( dialog ) );
}

#endif

char *forge_dialog_open( void *self, const char *title, const char *extension, const char *origin )
{
#if USE_GTK

	GtkWidget *dialog = gtk_file_chooser_dialog_new( title,
	                                                 NULL,
	                                                 GTK_FILE_CHOOSER_ACTION_OPEN,
	                                                 "_Cancel", GTK_RESPONSE_CANCEL,
	                                                 "_Open", GTK_RESPONSE_ACCEPT,
	                                                 NULL );

	if ( origin != nullptr )
	{
		g_autoptr( GFile ) folder = g_file_new_for_path( origin );
		if ( folder != nullptr )
		{
			gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( dialog ), folder, nullptr );
		}
	}

	GtkFileFilter *filter;
	if ( extension != nullptr )
	{
		char pattern[ strlen( extension ) + 3 ];
		snprintf( pattern, sizeof( pattern ), "*%s", extension );

		filter = gtk_file_filter_new();
		gtk_file_filter_set_name( filter, "Supported Files" );
		gtk_file_filter_add_pattern( filter, pattern );
		gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( dialog ), filter );
		gtk_file_chooser_set_filter( GTK_FILE_CHOOSER( dialog ), filter );
	}

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name( filter, "All Files" );
	gtk_file_filter_add_pattern( filter, "*" );
	gtk_file_chooser_add_filter( GTK_FILE_CHOOSER( dialog ), filter );

	ForgeDialog data = {};
	data.loop        = g_main_loop_new( nullptr, FALSE );

	g_signal_connect( dialog, "response", G_CALLBACK( on_dialog_response ), &data );
	g_signal_connect( dialog, "destroy", G_CALLBACK( gtk_window_destroy ), &data );

	gtk_widget_show( dialog );

	while ( !data.isCompleted )
	{
		g_main_context_iteration( nullptr, TRUE );
	}

	g_main_loop_unref( data.loop );

	if ( data.filename == nullptr )
	{
		return nullptr;
	}

	if ( extension != nullptr )
	{
		char *filename;
		if ( strlen( data.filename ) >= strlen( extension ) &&
		     strcmp( &data.filename[ strlen( data.filename ) - strlen( extension ) ], extension ) != 0 )
		{
			filename = qm_os_string_alloc( "%s%s", data.filename, extension );
		}

		g_free( data.filename );
	}

#else

	char pattern[ strlen( extension ) + 2 ];
	snprintf( pattern, sizeof( pattern ), "*%s", extension );

	FXString openName = FXFileDialog::getOpenFilename( ( FXWindow * ) self, title, origin, pattern );
	if ( openName.empty() )
	{
		return nullptr;
	}

	return qm_os_string_alloc( "%s", openName.text() );

#endif
}

ApeRoom *forge_new_room_( const char *path )
{
	QmFsMount *mount = PlGetMountLocationForPath( path );
	if ( mount == nullptr )
	{
		forge_warning_( "Room (%s) must be placed under a mounted location!\n", path );
		return nullptr;
	}

	ApeRoom   *room = ape_room_create( nullptr, "temp" );
	AcmBranch *root = ape_world_node_serialize( APE_WORLD_NODE( room ), nullptr );
	if ( root == nullptr )
	{
		forge_warning_( "Failed to serialize room!\n" );
		ape_world_node_destroy( APE_WORLD_NODE( room ) );
		return nullptr;
	}

	const char *mountPath = qm_fs_mount_get_path( mount );
	snprintf( APE_WORLD_NODE( room )->path, sizeof( APE_WORLD_NODE( room )->path ), "%s", &path[ strlen( mountPath ) + 1 ] );

	if ( !acm_write_file( path, root, ACM_FILE_TYPE_BINARY ) )
	{
		forge_warning_( "Failed to write room (%s): %s\n", path, acm_get_error_message() );
		ape_world_node_destroy( APE_WORLD_NODE( room ) );
		room = nullptr;
	}

	acm_branch_destroy( root );

	return room;
}

ApeRoom *forge_load_room_( const char *path )
{
	QmFsMount *mount = PlGetMountLocationForPath( path );
	if ( mount == nullptr )
	{
		forge_warning_( "Room (%s) must be placed under a mounted location!\n", path );
		return nullptr;
	}

	ApeWorldNode *roomNode = ape_world_node_load( nullptr, path );
	if ( roomNode == nullptr )
	{
		forge_warning_( "Failed to load room (%s)!\n", path );
		return nullptr;
	}

	if ( roomNode->type != APE_WORLD_NODE_TYPE_ROOM )
	{
		forge_warning_( "Room (%s) is not a valid room file!", path );
		ape_world_node_destroy( roomNode );
		return FALSE;
	}

	return ( ApeRoom * ) roomNode;
}

static AcmBranch *generate_project_config( const char *name, const char *path )
{
	AcmBranch *root = acm_push_object( nullptr, "project" );
	acm_push_string( root, "name", name, false );

	static constexpr int version[ 3 ] = { 0, 0, 0 };
	acm_push_array_i32( root, "version", version, 3 );

	AcmBranch *child;
	child = acm_push_array_string( root, "mountLocations", nullptr, 0 );
	acm_push_string( child, nullptr, "ship", false );
	acm_push_string( child, nullptr, "dev", false );

	child = acm_push_array_string( root, "dependencies", nullptr, 0 );
	acm_push_string( child, nullptr, "base", false );

	acm_write_file( path, root, ACM_FILE_TYPE_UTF8 );
	return root;
}

/**
 * Creates a new project.
 */
forge::Project *forge::create_project( const std::string &name, const std::string &folderName )
{
	auto *project = QM_OS_MEMORY_NEW( Project );

	PLPath projectPath;
	PlSetupPath( projectPath, true, "%s/%s", cachedPaths[ PATH_PROJECTS ], folderName.c_str() );

	if ( PlLocalPathExists( projectPath ) )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to create project, path (%s) already exists!", projectPath );
		return nullptr;
	}

	if ( !PlCreatePath( projectPath ) )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to create project path (%s)!", PlGetError() );
		return nullptr;
	}

	project->internalName = folderName;
	project->name         = name;

	// and now create our placeholder node file

	PLPath nodePath;
	project->config = generate_project_config( name.c_str(),
	                                           PlSetupPath( nodePath, true, "%s/%s/%s.prj.n",
	                                                        cachedPaths[ PATH_PROJECTS ],
	                                                        folderName.c_str(),
	                                                        folderName.c_str() ) );

	return project;
}

bool forge::open_project( const char *path )
{
	return com_project_mount( path );
}

static void setup_paths( const char *exePath )
{
	PlSetupPath( forge::cachedPaths[ forge::PATH_EXE ], true, "%s", exePath );
	PlSetupPath( forge::cachedPaths[ forge::PATH_RESOURCES ], true, "%s/../../resources", forge::cachedPaths[ forge::PATH_EXE ] );
	PlSetupPath( forge::cachedPaths[ forge::PATH_PROJECTS ], true, "%s/../../projects", forge::cachedPaths[ forge::PATH_EXE ] );
	PlSetupPath( forge::cachedPaths[ forge::PATH_COOK ], true, "%s/cook" QM_OS_SYSTEM_EXE_EXT, forge::cachedPaths[ forge::PATH_EXE ] );

	if ( !qm_fs_check_file_exists( forge::cachedPaths[ forge::PATH_CONFIG ] ) )
	{
		forge::isCookAvailable = false;
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to find cook (%s); content import may fail!",
		                       forge::cachedPaths[ forge::PATH_COOK ] );
	}

	PLPath tmp;
	if ( PlGetApplicationDataDirectory( "ape", tmp, sizeof( tmp ) ) != nullptr )
	{
		if ( PlCreateDirectory( tmp ) )
		{
			PlSetupPath( forge::cachedPaths[ forge::PATH_CONFIG ], true, "%s", tmp );
		}
		else
		{
			FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to create config location (%s)!", PlGetError() );
		}
	}
	else
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to get config location (%s)!", PlGetError() );
	}

	// fallback to local location if it failed...
	if ( *forge::cachedPaths[ forge::PATH_CONFIG ] == '\0' )
	{
		forge::cachedPaths[ forge::PATH_CONFIG ][ 0 ] = '.';
	}
}

FXIcon *forge::load_fx_icon( FXApp *app, const char *path )
{
	PLPath fullPath;
	PlSetupPath( fullPath, true, "../../%s", path );

	QmImage *image;
	auto     i = cachedImages.find( fullPath );
	if ( i != cachedImages.end() )
	{
		image = i->second;
	}
	else
	{
		image = qm_image_load( fullPath );
		if ( image == nullptr )
		{
			forge_warning_( "Failed to load icon (%s): %s\n", fullPath, PlGetError() );
			return nullptr;
		}

		cachedImages.emplace( fullPath, image );
	}

	FXIcon *icon = new FXIcon( app, static_cast< FXColor * >( PlGetImageData( image, 0, 0 ) ), 0, IMAGE_KEEP,
	                           static_cast< int >( image->width ),
	                           static_cast< int >( image->height ) );
	icon->create();
	return icon;
}

static void setup_colours( FXApp &app )
{
	forge::themeColours[ forge::THEME_COLOUR_BASE ]   = ( FXColor ) acm_get_uint( forge::editorConfig, "baseColour", FXRGB( 50, 50, 50 ) );
	forge::themeColours[ forge::THEME_COLOUR_FORE ]   = ( FXColor ) acm_get_uint( forge::editorConfig, "foreColour", FXRGB( 255, 255, 255 ) );
	forge::themeColours[ forge::THEME_COLOUR_HILITE ] = ( FXColor ) acm_get_uint( forge::editorConfig, "hiliteColour", FXRGB( 100, 100, 100 ) );
	forge::themeColours[ forge::THEME_COLOUR_BACK ]   = ( FXColor ) acm_get_uint( forge::editorConfig, "backColour", FXRGB( 10, 10, 10 ) );

	app.setBackColor( forge::themeColours[ forge::THEME_COLOUR_BACK ] );
	app.setBaseColor( forge::themeColours[ forge::THEME_COLOUR_BASE ] );
	app.setForeColor( forge::themeColours[ forge::THEME_COLOUR_FORE ] );

	app.setBorderColor( forge::themeColours[ forge::THEME_COLOUR_BASE ] );
	app.setHiliteColor( forge::themeColours[ forge::THEME_COLOUR_HILITE ] );
	app.setShadowColor( forge::themeColours[ forge::THEME_COLOUR_HILITE ] );
}

FXIcon *forge_cachedIcons[ MAX_FORGE_ICONS ];

static void cache_icons( FXApp &app )
{
	forge_cachedIcons[ FORGE_ICON_TYPE_MODE_BRUSH ] = forge::load_fx_icon( &app, "resources/brush_mode.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_MODE_EDGE ]  = forge::load_fx_icon( &app, "resources/edge_mode.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_MODE_FACE ]  = forge::load_fx_icon( &app, "resources/face_mode.gif" );

	forge_cachedIcons[ FORGE_ICON_TYPE_GRID_ORIENT ] = forge::load_fx_icon( &app, "resources/grid_orient.gif" );

	forge_cachedIcons[ FORGE_ICON_TYPE_FACE_PORTAL ]       = forge::load_fx_icon( &app, "resources/face_portal.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_FACE_TOGGLE ]       = forge::load_fx_icon( &app, "resources/face_toggle.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_FACE_TOGGLE_OTHER ] = forge::load_fx_icon( &app, "resources/face_toggle_other.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_FACE_SMOOTH ]       = forge::load_fx_icon( &app, "resources/face_smooth.gif" );

	forge_cachedIcons[ FORGE_ICON_TYPE_NODE ]    = forge::load_fx_icon( &app, "resources/node.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_WORLD ]   = forge::load_fx_icon( &app, "resources/world_editor.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_ENTITY ]  = forge::load_fx_icon( &app, "resources/entity.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_TEXTURE ] = forge::load_fx_icon( &app, "resources/texture.gif" );

	forge_cachedIcons[ FORGE_ICON_TYPE_ROOM ]     = forge::load_fx_icon( &app, "resources/room.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_NEW_ROOM ] = forge::load_fx_icon( &app, "resources/new_room.gif" );

	forge_cachedIcons[ FORGE_ICON_TYPE_NEW_WORLD ]  = forge::load_fx_icon( &app, "resources/new_world.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_OPEN_WORLD ] = forge::load_fx_icon( &app, "resources/open_world.gif" );

	forge_cachedIcons[ FORGE_ICON_TYPE_SAVE ]  = forge::load_fx_icon( &app, "resources/save.gif" );
	forge_cachedIcons[ FORGE_ICON_TYPE_CLOSE ] = forge::load_fx_icon( &app, "resources/close.gif" );
}

int main( int argc, char **argv )
{
	// now init common library and fetch the editor config
	aux_initialize( argc, argv );

	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );

	if ( PlgInitializeGraphics() != PL_RESULT_SUCCESS )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Error", "Failed to initialize Hei graphics library (%s)!", PlGetError() );
		return EXIT_FAILURE;
	}

	// attempt to fetch the driver directly from the executable location if possible
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) != nullptr )
	{
		char *driverPath = qm_os_string_alloc( "local://%s", exePath );
		PlgScanForDrivers( driverPath );
		qm_os_memory_free( driverPath );
	}
	else
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to get executable location (%s)!", PlGetError() );
		return EXIT_FAILURE;
	}

	editorLogLevels[ EDITOR_LOG_PRINT ]   = PlAddLogLevel( "forge", PL_COLOUR_BLUE_VIOLET, true );
	editorLogLevels[ EDITOR_LOG_WARNING ] = PlAddLogLevel( "forge/warning", PL_COLOUR_YELLOW, true );
	editorLogLevels[ EDITOR_LOG_ERROR ]   = PlAddLogLevel( "forge/error", PL_COLOUR_RED, true );

	qm_fs_mount_local_location( com_get_app_data_directory() );
	qm_fs_mount_local_location( com_get_local_data_directory() );

	forge::editorConfig = com_get_config( "forge" );

	const char *projectPath = acm_get_string( forge::editorConfig, "projectsPath", "projects" );
	if ( projectPath != nullptr )
	{
		snprintf( forge::cachedPaths[ forge::PATH_PROJECTS ], sizeof( PLPath ), "%s", projectPath );
	}

	FXApp app( FORGE_APP_NAME, FXString::null );
	app.init( argc, argv );

	setup_colours( app );
	cache_icons( app );

	glVisual = new FXGLVisual( &app, VISUAL_DEFAULT );

	// create our editor window with it's GLContext etc., so we can then init our GL driver
	forge::mainWindow = new forge::MainWindow( &app );

	app.create();

	forge::mainWindow->show();

	//HACK: make the engine initialisation happy...
	auto *dummy = new forge::Viewport( forge::mainWindow, forge::get_shared_gl_visual(), nullptr, APE_CAMERA_MODE_PERSPECTIVE );
	dummy->create();

	setup_paths( exePath );

	if ( PlgSetDriver( "opengl" ) != PL_RESULT_SUCCESS )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to set OpenGL driver: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	// allow us to override the project if desired
	const char *projectName = PlGetCommandLineArgumentValue( "/project" );
	if ( projectName != nullptr )
	{
		AcmBranch *branch;
		if ( ( branch = com_project_mount( projectName ) ) == nullptr )
		{
			shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Invalid project specified (%s), aborting!", projectName );
			return EXIT_FAILURE;
		}

		forge::editorProject               = new forge::Project( projectName );
		forge::editorProject->name         = acm_get_string( branch, "name", "none" );
		forge::editorProject->internalName = projectName;
		forge::editorProject->config       = branch;
	}

	// now let us pick a project before we init the engine
	// (for now, changing project will probably require us to restart)
	if ( forge::editorProject == nullptr )
	{
		auto *projectDialog = new forge::ProjectDialog( forge::mainWindow );
		projectDialog->execute();
		delete projectDialog;
	}

	if ( forge::editorProject == nullptr )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "No project selected, aborting!" );
		return EXIT_FAILURE;
	}

#if !defined( _WIN32 )
	// allow us to cook everything before launching, if desired
	if ( PlHasCommandLineArgument( "/cook" ) )
	{
		char *cookPath = qm_os_string_alloc( "%s/cool %s", exePath, projectName );
		if ( system( cookPath ) == -1 )
		{
			FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to execute cook command!" );
		}

		qm_os_memory_free( cookPath );
	}
#endif

	if ( !ape_initialize( argc, argv, FORGE_CONFIG_FILENAME ) )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to initialize APE Tech!" );
		return EXIT_FAILURE;
	}

	dummy->hide();

	return app.run();
}

extern "C"
{
	void shell_get_window_size( int *width, int *height ) {}

	void shell_display_message( SS_Shell_MessageBoxType messageType, const char *message, ... )
	{
		switch ( messageType )
		{
			case SS_SHELL_MESSAGE_BOX_TYPE_ERROR:
				FXMessageBox::error( FXApp::instance(), MBOX_OK, "Forge Error", "%s", message );
				break;
			case SS_SHELL_MESSAGE_BOX_TYPE_INFO:
				FXMessageBox::information( FXApp::instance(), MBOX_OK, "Forge Info", "%s", message );
				break;
			case SS_SHELL_MESSAGE_BOX_TYPE_WARNING:
				FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Forge Warning", "%s", message );
				break;
			default:
				break;
		}
	}

	float shell_get_display_scale()
	{
		// fox doesn't support dpi scaling, annoyingly...
		return 1.0f;
	}

	ApeViewport *ss_shell_viewport_get_active( void )
	{
		return nullptr;
	}

	ApeInputState ss_shell_get_button_state( ApeInputButton inputButton ) { return APE_INPUT_STATE_NONE; }
	ApeInputState ss_shell_get_key_state( int key ) { return APE_INPUT_STATE_NONE; }

	void shell_set_mouse_position( int x, int y )
	{
	}

	void ss_shell_grab_mouse( bool grab ) {}

	void ss_shell_shutdown( void )
	{
		forge::mainWindow->destroy();
	}
}

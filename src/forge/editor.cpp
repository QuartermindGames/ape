// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_driver_interface.h>

#include "editor.h"
#include "editor_window_main.h"
#include "editor_dialog_project.h"

#include "common_project.h"

// Override C++ new/delete operators, so we can track memory usage
#if 0//TODO: causing pain on win32 target, let's not bother for now
void *operator new( size_t size ) { return PL_NEW_( char, size ); }
void *operator new[]( size_t size ) { return PL_NEW_( char, size ); }
void operator delete( void *p ) noexcept { PL_DELETE( p ); }
void operator delete[]( void *p ) noexcept { PL_DELETE( p ); }
#endif

int editorLogLevels[ EDITOR_MAX_LOG_LEVELS ];

PLPath ss::forge::cachedPaths[ MAX_CACHED_PATHS ];
NdBranch *ss::forge::editorConfig;

ss::forge::Project *ss::forge::editorProject = nullptr;

static std::map< std::string, PLImage * > cachedImages;

static NdBranch *generate_project_config( const char *name, const char *path )
{
	NdBranch *root = ndPushBackObject( nullptr, "project" );
	ndPushBackString( root, "name", name );

	const static constexpr int version[ 3 ] = { 0, 0, 0 };
	ndPushBackI32Array( root, "version", version, 3 );

	NdBranch *child;
	child = ndPushBackStringArray( root, "mountLocations", nullptr, 0 );
	ndPushBackString( child, nullptr, "ship" );
	ndPushBackString( child, nullptr, "dev" );

	child = ndPushBackStringArray( root, "dependencies", nullptr, 0 );
	ndPushBackString( child, nullptr, "base" );

	ndWriteFile( path, root, ND_FILE_UTF8 );
	return root;
}

/**
 * Creates a new project.
 */
ss::forge::Project *ss::forge::create_project( const std::string &name, const std::string &folderName )
{
	auto *project = PL_NEW( Project );

	PLPath projectPath;
	PlSetupPath( projectPath, true, "%s/%s", ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ], folderName.c_str() );

	if ( PlLocalPathExists( projectPath ) )
	{
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to create project, path (%s) already exists!", projectPath );
		return nullptr;
	}

	if ( !PlCreatePath( projectPath ) )
	{
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to create project path (%s)!", PlGetError() );
		return nullptr;
	}

	project->internalName = folderName;
	project->name = name;

	// and now create our placeholder node file

	PLPath nodePath;
	project->config = generate_project_config( name.c_str(),
	                                           PlSetupPath( nodePath, true, "%s/%s/%s.prj.n",
	                                                        ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ],
	                                                        folderName.c_str(),
	                                                        folderName.c_str() ) );

	return project;
}

bool ss::forge::open_project( const char *path )
{
	return ss_com_project_mount( path );
}

static void setup_paths( const char *exePath )
{
	PL_ZERO( ss::forge::cachedPaths, sizeof( PLPath ) * ss::forge::MAX_CACHED_PATHS );

	// copy the exe path and ensure it doesn't end in a slash
	PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_EXE ], true, "%s", exePath );

	// resources location - where editor icons are stored
	PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_RESOURCES ], true, "%s/../../resources", ss::forge::cachedPaths[ ss::forge::PATH_EXE ] );

	// projects location - where new projects will be created by default
	PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ], true, "%s/../../projects", ss::forge::cachedPaths[ ss::forge::PATH_EXE ] );

	PLPath tmp;
	if ( PlGetApplicationDataDirectory( "ape", tmp, sizeof( tmp ) ) != nullptr )
	{
		if ( PlCreateDirectory( tmp ) )
			PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_CONFIG ], true, "%s", tmp );
		else
			FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to create config location (%s)!", PlGetError() );
	}
	else
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to get config location (%s)!", PlGetError() );

	// fallback to local location if it failed...
	if ( *ss::forge::cachedPaths[ ss::forge::PATH_CONFIG ] == '\0' )
		ss::forge::cachedPaths[ ss::forge::PATH_CONFIG ][ 0 ] = '.';
}

static void setup_config()
{
	// first try and load it locally
	PLPath path;
	PlSetupPath( path, true, "local://%s/" EDITOR_CONFIG_FILENAME, ss::forge::cachedPaths[ ss::forge::PATH_EXE ] );
	if ( ( ss::forge::editorConfig = ndLoadFile( path, "config" ) ) == nullptr )
	{
		// try again, but from config location
		PlSetupPath( path, true, "local://%s/" EDITOR_CONFIG_FILENAME, ss::forge::cachedPaths[ ss::forge::PATH_CONFIG ] );
		if ( ( ss::forge::editorConfig = ndLoadFile( path, "config" ) ) == nullptr )
		{
			// uh oh! just append an object and return
			ss::forge::editorConfig = ndPushBackObject( nullptr, "config" );
			return;
		}
	}

	const char *projectPath = ndGetStringByName( ss::forge::editorConfig, "projectsPath", "../../projects" );
	if ( projectPath != nullptr )
		snprintf( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ], sizeof( PLPath ), "%s", projectPath );
	else
		// no project path provided, just use a fallback
		snprintf( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ], sizeof( PLPath ), "projects" );
}

FXIcon *ss::forge::load_fx_icon( FXApp *app, const char *path )
{
	char fullPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( fullPath, sizeof( fullPath ), "../../%s", path );

	PLImage *image;
	auto i = cachedImages.find( fullPath );
	if ( i != cachedImages.end() )
		image = i->second;
	else
	{
		image = PlLoadImage( fullPath );
		if ( image == nullptr )
			return nullptr;

		cachedImages.emplace( fullPath, image );
	}

	FXIcon *icon = new FXIcon( app );
	icon->setData( ( FXColor * ) PlGetImageData( image, 0, 0 ), IMAGE_KEEP | IMAGE_ALPHACOLOR, ( int ) image->width, ( int ) image->height );
	return icon;
}

int main( int argc, char **argv )
{
#if !defined( NDEBUG ) && defined( WIN32 )
	setvbuf( stdout, nullptr, _IONBF, 0 );
#endif

	if ( PlInitialize( argc, argv ) != PL_RESULT_SUCCESS )
	{
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Error", "Failed to initialize Hei library (%s)!", PlGetError() );
		return EXIT_FAILURE;
	}

	if ( PlgInitializeGraphics() != PL_RESULT_SUCCESS )
	{
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Error", "Failed to initialize Hei graphics library (%s)!", PlGetError() );
		return EXIT_FAILURE;
	}

	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );

	// attempt to fetch the driver directly from the executable location if possible
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) != nullptr )
	{
		size_t const size = strlen( exePath ) + PL_SYSTEM_MAX_PATH + 1;
		char *driverPath = PL_NEW_( char, size );
		snprintf( driverPath, size, "local://%s", exePath );
		PlgScanForDrivers( driverPath );
		PL_DELETE( driverPath );
	}
	else
		PlgScanForDrivers( "." );

	PLPath tmp;
	if ( PlGetExecutableDirectory( tmp, sizeof( tmp ) ) == nullptr )
	{
		FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Error", "Failed to get executable location (%s)!", PlGetError() );
		return EXIT_FAILURE;
	}

	// now init common library and fetch the editor config
	com_initialize();

	setup_paths( tmp );
	setup_config();

	FXApp app( SS_FORGE_APP_TITLE, FXString::null );
	app.init( argc, argv );

	static constexpr FXColor BASE_COLOUR = FXRGB( 40, 40, 40 );
	static constexpr FXColor FORE_COLOUR = FXRGB( 200, 200, 250 );
	static constexpr FXColor HILITE_COLOUR = FXRGB( 100, 100, 150 );

	app.setBackColor( FXRGB( 10, 10, 10 ) );
	app.setBaseColor( BASE_COLOUR );
	app.setForeColor( FORE_COLOUR );

	app.setBorderColor( BASE_COLOUR );
	app.setHiliteColor( HILITE_COLOUR );
	app.setShadowColor( HILITE_COLOUR );

	// create our editor window with it's GLContext etc., so we can then init our GL driver
	ss::forge::mainWindow = new ss::forge::MainWindow( &app );

	app.create();

	if ( PlgSetDriver( "opengl" ) != PL_RESULT_SUCCESS )
	{
		FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Error", "Failed to set OpenGL driver (%s)!", PlGetError() );
		return EXIT_FAILURE;
	}

	// now let us pick a project before we init the engine
	// (for now, changing project will probably require us to restart)
	auto *projectDialog = new ss::forge::ProjectDialog( ss::forge::mainWindow );
	projectDialog->execute();

	if ( ss::forge::editorProject == nullptr )
	{
		FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Error", "No project selected, aborting!" );
		return EXIT_FAILURE;
	}
	delete projectDialog;

	ss::forge::mainWindow->show();

	if ( !ss_acl_initialize( argc, argv, EDITOR_CONFIG_FILENAME ) )
	{
		FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Error", "Failed to initialize Yin!" );
		return EXIT_FAILURE;
	}

	ss::forge::mainWindow->setup_engine_viewports();

	return app.run();
}

extern "C"
{
	void ss_shell_get_window_size( int *width, int *height ) {}

	void ss_shell_swap_window( SSArlViewport * ) {}

	void ss_shell_display_message( SS_Shell_MessageBoxType messageType, const char *message, ... )
	{
		switch ( messageType )
		{
			case SS_SHELL_MESSAGE_BOX_TYPE_ERROR:
				FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Forge Error", "%s", message );
				break;
			case SS_SHELL_MESSAGE_BOX_TYPE_INFO:
				FXMessageBox::information( FXApp::instance(), FX::MBOX_OK, "Forge Info", "%s", message );
				break;
			case SS_SHELL_MESSAGE_BOX_TYPE_WARNING:
				FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Forge Warning", "%s", message );
				break;
			default:
				break;
		}
	}

	SSArlViewport *ss_shell_viewport_get_active( void )
	{
		return nullptr;
	}

	ApeInputState ss_shell_get_button_state( ApeInputButton inputButton ) { return APE_INPUT_STATE_NONE; }
	ApeInputState ss_shell_get_key_state( int key ) { return APE_INPUT_STATE_NONE; }
	void ss_shell_get_mouse_position( int *x, int *y ) {}
	void ss_shell_set_mouse_position( int x, int y ) {}
	void ss_shell_grab_mouse( bool grab ) {}

	void ss_shell_push_message( int level, const char *msg, const PLColour *colour )
	{
		ss::forge::mainWindow->push_message( level, msg, *colour );
	}

	void ss_shell_shutdown( void )
	{
		ss::forge::mainWindow->destroy();
	}
}

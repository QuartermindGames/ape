// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_driver_interface.h>

#include "editor.h"
#include "main_window.h"
#include "ProjectDialog.h"

#include "common_project.h"

#include <FXGLVisual.h>

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

static FXGLVisual *glVisual = nullptr;
FXGLVisual *ss::forge::get_shared_gl_visual() { return glVisual; }

bool ss::forge::isCookAvailable = true;

ss::forge::Project *ss::forge::editorProject = nullptr;

static std::map< std::string, PLImage * > cachedImages;

FXColor ss::forge::themeColours[ ThemeColour::MAX_THEME_COLOURS ]{};

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
	return com_project_mount( path );
}

static void setup_paths( const char *exePath )
{
	PL_ZERO( ss::forge::cachedPaths, sizeof( PLPath ) * ss::forge::MAX_CACHED_PATHS );

	PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_EXE ], true, "%s", exePath );
	PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_RESOURCES ], true, "%s/../../resources", ss::forge::cachedPaths[ ss::forge::PATH_EXE ] );
	PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ], true, "%s/../../projects", ss::forge::cachedPaths[ ss::forge::PATH_EXE ] );
	PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_COOK ], true, "%s/cook" PL_SYSTEM_EXE_EXTENSION, ss::forge::cachedPaths[ ss::forge::PATH_EXE ] );

	if ( !PlFileExists( ss::forge::cachedPaths[ ss::forge::PATH_CONFIG ] ) )
	{
		ss::forge::isCookAvailable = false;
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to find cook (%s); content import may fail!",
		                       ss::forge::cachedPaths[ ss::forge::PATH_COOK ] );
	}

	PLPath tmp;
	if ( PlGetApplicationDataDirectory( "ape", tmp, sizeof( tmp ) ) != nullptr )
	{
		if ( PlCreateDirectory( tmp ) )
			PlSetupPath( ss::forge::cachedPaths[ ss::forge::PATH_CONFIG ], true, "%s", tmp );
		else
		{
			FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to create config location (%s)!", PlGetError() );
		}
	}
	else
	{
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to get config location (%s)!", PlGetError() );
	}

	// fallback to local location if it failed...
	if ( *ss::forge::cachedPaths[ ss::forge::PATH_CONFIG ] == '\0' )
	{
		ss::forge::cachedPaths[ ss::forge::PATH_CONFIG ][ 0 ] = '.';
	}
}

FXIcon *ss::forge::load_fx_icon( FXApp *app, const char *path )
{
	char fullPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( fullPath, sizeof( fullPath ), "../../%s", path );

	PLImage *image;
	auto i = cachedImages.find( fullPath );
	if ( i != cachedImages.end() )
	{
		image = i->second;
	}
	else
	{
		image = PlLoadImage( fullPath );
		if ( image == nullptr )
		{
			EDITOR_PRINT( "Failed to load icon (%s): %s\n", fullPath, PlGetError() );
			return nullptr;
		}

		cachedImages.emplace( fullPath, image );
	}

	auto *icon = new FXIcon( app );
	icon->setData( ( FXColor * ) PlGetImageData( image, 0, 0 ), IMAGE_KEEP | IMAGE_ALPHACOLOR, ( int ) image->width, ( int ) image->height );
	return icon;
}

static void setup_app_colours( FXApp &app )
{
	ss::forge::themeColours[ ss::forge::THEME_COLOUR_BASE ] = ( FXColor ) ndGetUInt( ss::forge::editorConfig, "baseColour", FXRGB( 50, 50, 50 ) );
	ss::forge::themeColours[ ss::forge::THEME_COLOUR_FORE ] = ( FXColor ) ndGetUInt( ss::forge::editorConfig, "foreColour", FXRGB( 255, 255, 255 ) );
	ss::forge::themeColours[ ss::forge::THEME_COLOUR_HILITE ] = ( FXColor ) ndGetUInt( ss::forge::editorConfig, "hiliteColour", FXRGB( 100, 100, 100 ) );
	ss::forge::themeColours[ ss::forge::THEME_COLOUR_BACK ] = ( FXColor ) ndGetUInt( ss::forge::editorConfig, "backColour", FXRGB( 10, 10, 10 ) );

	app.setBackColor( ss::forge::themeColours[ ss::forge::THEME_COLOUR_BACK ] );
	app.setBaseColor( ss::forge::themeColours[ ss::forge::THEME_COLOUR_BASE ] );
	app.setForeColor( ss::forge::themeColours[ ss::forge::THEME_COLOUR_FORE ] );

	app.setBorderColor( ss::forge::themeColours[ ss::forge::THEME_COLOUR_BASE ] );
	app.setHiliteColor( ss::forge::themeColours[ ss::forge::THEME_COLOUR_HILITE ] );
	app.setShadowColor( ss::forge::themeColours[ ss::forge::THEME_COLOUR_HILITE ] );
}

int main( int argc, char **argv )
{
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
	{
		PlgScanForDrivers( "." );
	}

	PLPath tmp;
	if ( PlGetExecutableDirectory( tmp, sizeof( tmp ) ) == nullptr )
	{
		FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Error", "Failed to get executable location (%s)!", PlGetError() );
		return EXIT_FAILURE;
	}

	// now init common library and fetch the editor config
	com_initialize();

	PlMountLocalLocation( com_get_app_data_directory() );
	PlMountLocalLocation( com_get_local_data_directory() );

	ss::forge::editorConfig = com_get_config( "editor" );

	const char *projectPath = ndGetStringByName( ss::forge::editorConfig, "projectsPath", "projects" );
	if ( projectPath != nullptr )
	{
		snprintf( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ], sizeof( PLPath ), "%s", projectPath );
	}

	FXApp app( SS_FORGE_APP_NAME, FXString::null );
	app.init( argc, argv );

	//setup_app_colours( app );

	glVisual = new FXGLVisual( &app, VISUAL_DEFAULT );

	// create our editor window with it's GLContext etc., so we can then init our GL driver
	ss::forge::mainWindow = new ss::forge::main_window( &app );

	app.create();

	ss::forge::mainWindow->show();
	ss::forge::mainWindow->maximize();

	setup_paths( tmp );

	if ( PlgSetDriver( "opengl" ) != PL_RESULT_SUCCESS )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to set OpenGL driver: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	// now let us pick a project before we init the engine
	// (for now, changing project will probably require us to restart)
	auto *projectDialog = new ss::forge::ProjectDialog( ss::forge::mainWindow );
	projectDialog->execute();

	if ( ss::forge::editorProject == nullptr )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "No project selected, aborting!" );
		return EXIT_FAILURE;
	}
	delete projectDialog;

	if ( !ape_initialize( argc, argv, EDITOR_CONFIG_FILENAME ) )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to initialize APE Tech!" );
		return EXIT_FAILURE;
	}

	return app.run();
}

extern "C"
{
	void shell_get_window_size( int *width, int *height ) {}

	void shell_swap_window( ApeViewport *viewport ) {}

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

	ApeViewport *ss_shell_viewport_get_active( void )
	{
		return nullptr;
	}

	ApeInputState ss_shell_get_button_state( ApeInputButton inputButton ) { return APE_INPUT_STATE_NONE; }
	ApeInputState ss_shell_get_key_state( int key ) { return APE_INPUT_STATE_NONE; }
	void ss_shell_get_mouse_position( int *x, int *y ) {}
	void ss_shell_set_mouse_position( int x, int y )
	{
	}

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

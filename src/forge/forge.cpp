// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_driver_interface.h>

#include "forge.h"
#include "forge_window_main.h"
#include "forge_project_dialog.h"

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

PLPath     forge::cachedPaths[ MAX_CACHED_PATHS ] = {};
AcmBranch *forge::editorConfig;

static FXGLVisual *glVisual = nullptr;
FXGLVisual        *forge::get_shared_gl_visual() { return glVisual; }

bool forge::isCookAvailable = true;

forge::Project *forge::editorProject = nullptr;

static std::map< std::string, PLImage * > cachedImages;

FXColor forge::themeColours[ ThemeColour::MAX_THEME_COLOURS ]{};

static AcmBranch *generate_project_config( const char *name, const char *path )
{
	AcmBranch *root = acm_branch_push_back_object( nullptr, "project" );
	acm_push_string( root, "name", name, false );

	const static constexpr int version[ 3 ] = { 0, 0, 0 };
	acm_branch_push_back_int32_array( root, "version", version, 3 );

	AcmBranch *child;
	child = acm_branch_push_back_string_array( root, "mountLocations", nullptr, 0 );
	acm_push_string( child, nullptr, "ship", false );
	acm_push_string( child, nullptr, "dev", false );

	child = acm_branch_push_back_string_array( root, "dependencies", nullptr, 0 );
	acm_push_string( child, nullptr, "base", false );

	acm_write_file( path, root, ND_FILE_UTF8 );
	return root;
}

/**
 * Creates a new project.
 */
forge::Project *forge::create_project( const std::string &name, const std::string &folderName )
{
	auto *project = PL_NEW( Project );

	PLPath projectPath;
	PlSetupPath( projectPath, true, "%s/%s", forge::cachedPaths[ forge::PATH_PROJECTS ], folderName.c_str() );

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
	project->name         = name;

	// and now create our placeholder node file

	PLPath nodePath;
	project->config = generate_project_config( name.c_str(),
	                                           PlSetupPath( nodePath, true, "%s/%s/%s.prj.n",
	                                                        forge::cachedPaths[ forge::PATH_PROJECTS ],
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
	PlSetupPath( forge::cachedPaths[ forge::PATH_COOK ], true, "%s/cook" PL_SYSTEM_EXE_EXTENSION, forge::cachedPaths[ forge::PATH_EXE ] );

	if ( !PlFileExists( forge::cachedPaths[ forge::PATH_CONFIG ] ) )
	{
		forge::isCookAvailable = false;
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to find cook (%s); content import may fail!",
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
			FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to create config location (%s)!", PlGetError() );
		}
	}
	else
	{
		FXMessageBox::warning( FXApp::instance(), FX::MBOX_OK, "Warning", "Failed to get config location (%s)!", PlGetError() );
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

#if 1

	PLImage *image;
	auto     i = cachedImages.find( fullPath );
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

	FXIcon *icon = new FXIcon( app );
	icon->setData( ( FXColor * ) PlGetImageData( image, 0, 0 ), IMAGE_KEEP | IMAGE_ALPHACOLOR, ( int ) image->width, ( int ) image->height );
	icon->create();
	return icon;

#else

	FXIconSource iconSource( app );
	return iconSource.loadIconFile( fullPath );

#endif
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

FXIcon     *forge_cachedIcons[ MAX_FORGE_ICONS ];
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
		size_t const size       = strlen( exePath ) + PL_SYSTEM_MAX_PATH + 1;
		char        *driverPath = PL_NEW_( char, size );
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

	forge::editorConfig = com_get_config( "editor" );

	const char *projectPath = acm_branch_get_child_string( forge::editorConfig, "projectsPath", "projects" );
	if ( projectPath != nullptr )
	{
		snprintf( forge::cachedPaths[ forge::PATH_PROJECTS ], sizeof( PLPath ), "%s", projectPath );
	}

	FXApp app( FORGE_APP_NAME, FXString::null );
	app.init( argc, argv );

	setup_colours( app );
	cache_icons( app );

	glVisual = new FXGLVisual( &app, VISUAL_DOUBLEBUFFER );

	// create our editor window with it's GLContext etc., so we can then init our GL driver
	forge::mainWindow = new forge::MainWindow( &app );

	app.create();

	forge::mainWindow->show();
	forge::mainWindow->maximize();

	//HACK: make the engine initialisation happy...
	auto *dummy = new forge::Viewport( forge::mainWindow, forge::get_shared_gl_visual(), nullptr, APE_CAMERA_MODE_PERSPECTIVE );
	dummy->create();

	setup_paths( tmp );

	if ( PlgSetDriver( "opengl" ) != PL_RESULT_SUCCESS )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to set OpenGL driver: %s\n", PlGetError() );
		return EXIT_FAILURE;
	}

	// allow us to override the project if desired
	const char *projectName = PlGetCommandLineArgumentValue( "/project" );
	if ( projectName != nullptr )
	{
		AcmBranch *branch;
		if ( ( branch = com_project_mount( projectName ) ) == nullptr )
		{
			ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Invalid project specified (%s), aborting!", projectName );
			return EXIT_FAILURE;
		}

		forge::editorProject               = new forge::Project( projectName );
		forge::editorProject->name         = acm_branch_get_child_string( branch, "name", "none" );
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
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "No project selected, aborting!" );
		return EXIT_FAILURE;
	}

	if ( !ape_initialize( argc, argv, FORGE_CONFIG_FILENAME, true ) )
	{
		ss_shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to initialize APE Tech!" );
		return EXIT_FAILURE;
	}

	dummy->hide();

	return app.run();
}

extern "C"
{
	void shell_get_window_size( int *width, int *height ) {}

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

	void shell_set_mouse_position( int x, int y )
	{
	}

	void ss_shell_grab_mouse( bool grab ) {}

	void ss_shell_push_message( int level, const char *msg, const PLColour *colour )
	{
		forge::mainWindow->push_message( level, msg, *colour );
	}

	void ss_shell_shutdown( void )
	{
		forge::mainWindow->destroy();
	}
}

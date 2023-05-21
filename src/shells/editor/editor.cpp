// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_console.h>
#include <plcore/pl_filesystem.h>

#include <plgraphics/plg.h>
#include <plgraphics/plg_driver_interface.h>

#include "editor.h"
#include "editor_window_main.h"
#include "editor_dialog_project.h"

// Override C++ new/delete operators, so we can track memory usage
#if 0//TODO: causing pain on win32 target, let's not bother for now
void *operator new( size_t size ) { return PL_NEW_( char, size ); }
void *operator new[]( size_t size ) { return PL_NEW_( char, size ); }
void operator delete( void *p ) noexcept { PL_DELETE( p ); }
void operator delete[]( void *p ) noexcept { PL_DELETE( p ); }
#endif

unsigned int editorLogMsgId;
unsigned int editorLogWarnId;
unsigned int editorLogErrorId;

PLPath os::editor::cachedPaths[ MAX_CACHED_PATHS ];
YNNodeBranch *os::editor::editorConfig;

os::editor::Project *os::editor::editorProject = nullptr;

static YNNodeBranch *GenerateProjectConfig( const char *name, const char *path )
{
	YNNodeBranch *root = YnNode_PushBackObject( nullptr, "config" );
	YnNode_PushBackString( root, "title", name );
	const static constexpr int version[ 3 ] = { 0, 0, 0 };
	YnNode_PushBackI32Array( root, "version", version, 3 );
	YnNode_WriteFile( path, root, YN_NODE_FILE_UTF8 );
	return root;
}

/**
 * Creates a new project.
 */
os::editor::Project *os::editor::CreateProject( const std::string &name, const std::string &folderName )
{
	auto *project = PL_NEW( Project );

	PLPath projectPath;
	PlSetupPath( projectPath, true, "%s/%s", os::editor::cachedPaths[ os::editor::PATH_PROJECTS ], folderName.c_str() );

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

	project->rootDir = folderName;
	project->name    = name;

	// and now create our placeholder node file

	PLPath nodePath;
	project->config = GenerateProjectConfig( name.c_str(),
	                                         PlSetupPath( nodePath, true, "%s/%s.prj.n",
	                                                      os::editor::cachedPaths[ os::editor::PATH_PROJECTS ],
	                                                      folderName.c_str() ) );

	return project;
}

os::editor::Project *os::editor::OpenProject( const char *path )
{
#if 0
	if ( os::editor::editorProject.config != nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), 0, "Warning", "Project already open, please close it first before opening another!" );
		return false;
	}

	PLPath configPath;
	PlSetPath( configPath, path, true );
	PlAppendPath( configPath, "/project.cfg.n", true );

	if ( ( os::editor::editorProject.config = NL_LoadFile( configPath, "config" ) ) == nullptr )
	{
		GenerateProjectConfig( "Unnamed Project", path );
		if ( ( os::editor::editorProject.config = NL_LoadFile( configPath, "config" ) ) == nullptr )
		{
			FXMessageBox::warning( FXApp::instance(), 0, "Warning", "Failed to generate project configuration!\nNL: %s", NL_GetErrorMessage() );
			return false;
		}
	}

	os::editor::editorProject.name = NL_GetStrByName( os::editor::editorProject.config, "title", nullptr );
	if ( os::editor::editorProject.name == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), 0, "Warning", "No project title found in project configuration!" );
		return false;
	}

	if ( git_repository_open( &repository, path ) == 0 )
		usingVersionControl = true;
	else
	{
		const git_error *err = git_error_last();
		if ( err != nullptr )
			FXMessageBox::warning( FXApp::instance(), 0, "Warning", "Failed to open git repository for project (%s)!", err->message );
	}

	return true;
#endif
	return nullptr;
}

static void SetupPaths( const char *exePath )
{
	PL_ZERO( os::editor::cachedPaths, sizeof( PLPath ) * os::editor::MAX_CACHED_PATHS );

	// copy the exe path and ensure it doesn't end in a slash
	PlSetupPath( os::editor::cachedPaths[ os::editor::PATH_EXE ], true, "%s", exePath );

	// resources location - where editor icons are stored
	PlSetupPath( os::editor::cachedPaths[ os::editor::PATH_RESOURCES ], true, "%s/../../resources", os::editor::cachedPaths[ os::editor::PATH_EXE ] );

	// projects location - where new projects will be created by default
	PlSetupPath( os::editor::cachedPaths[ os::editor::PATH_PROJECTS ], true, "%s/../../projects", os::editor::cachedPaths[ os::editor::PATH_EXE ] );

	PLPath tmp;
	if ( PlGetApplicationDataDirectory( "yin", tmp, sizeof( tmp ) ) != nullptr )
	{
		if ( PlCreateDirectory( tmp ) )
		{
			PlSetupPath( os::editor::cachedPaths[ os::editor::PATH_CONFIG ], true, "%s", tmp );
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
	if ( *os::editor::cachedPaths[ os::editor::PATH_CONFIG ] == '\0' )
	{
		os::editor::cachedPaths[ os::editor::PATH_CONFIG ][ 0 ] = '.';
	}
}

static void SetupConfig()
{
	// first try and load it locally
	PLPath path;
	PlSetupPath( path, true, "local://%s/editor.cfg.n", os::editor::cachedPaths[ os::editor::PATH_EXE ] );
	if ( ( os::editor::editorConfig = YnNode_LoadFile( path, "config" ) ) == nullptr )
	{
		// try again, but from config location
		PlSetupPath( path, true, "local://%s/editor.cfg.n", os::editor::cachedPaths[ os::editor::PATH_CONFIG ] );
		if ( ( os::editor::editorConfig = YnNode_LoadFile( path, "config" ) ) == nullptr )
		{
			// uh oh! just append an object and return
			os::editor::editorConfig = YnNode_PushBackObject( nullptr, "config" );
			return;
		}
	}

	const char *projectPath = YnNode_GetStringByName( os::editor::editorConfig, "projectsPath", "../../projects" );
	if ( projectPath != nullptr )
	{
		snprintf( os::editor::cachedPaths[ os::editor::PATH_PROJECTS ], sizeof( PLPath ), "%s", projectPath );
	}
	else
	{
		// no project path provided, just use a fallback
		snprintf( os::editor::cachedPaths[ os::editor::PATH_PROJECTS ], sizeof( PLPath ), "projects" );
	}
}

FXIcon *os::editor::LoadFXIcon( FXApp *app, const char *path )
{
	char fullPath[ PL_SYSTEM_MAX_PATH ];
	snprintf( fullPath, sizeof( fullPath ), "./../../%s", path );

	FXIconSource const iconSource( app );
	return iconSource.loadIconFile( fullPath );
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

	// attempt to fetch the driver directly from the executable location if possible
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) != nullptr )
	{
		size_t const size = strlen( exePath ) + PL_SYSTEM_MAX_PATH + 1;
		char *driverPath  = PL_NEW_( char, size );
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
	Common_Initialize();

	SetupPaths( tmp );
	SetupConfig();

	FXApp app( EDITOR_APP_TITLE, FXString::null );
	app.init( argc, argv );

	static constexpr FXColor baseColour   = FXRGB( 40, 40, 40 );
	static constexpr FXColor foreColour   = FXRGB( 200, 200, 250 );
	static constexpr FXColor hiliteColour = FXRGB( 100, 100, 150 );

	app.setBackColor( FXRGB( 10, 10, 10 ) );
	app.setBaseColor( baseColour );
	app.setForeColor( foreColour );

	app.setBorderColor( baseColour );
	app.setHiliteColor( hiliteColour );
	app.setShadowColor( hiliteColour );

	// create our editor window with it's GLContext etc., so we can then init our GL driver
	os::editor::mainWindow = new os::editor::MainWindow( &app );

	app.create();

	if ( PlgSetDriver( "opengl" ) != PL_RESULT_SUCCESS )
	{
		FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Error", "Failed to set OpenGL driver (%s)!", PlGetError() );
		return EXIT_FAILURE;
	}

	// now let us pick a project before we init the engine
	// (for now, changing project will probably require us to restart)
	auto *projectDialog = new os::editor::ProjectDialog( os::editor::mainWindow );
	projectDialog->execute();

	if ( os::editor::editorProject == nullptr )
	{
		FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Error", "No project selected, aborting!" );
		return EXIT_FAILURE;
	}
	delete projectDialog;

	os::editor::mainWindow->show();

	if ( !ogeInitialize( "editor.cfg.n" ) )
	{
		FXMessageBox::error( FXApp::instance(), FX::MBOX_OK, "Error", "Failed to initialize Yin!" );
		return EXIT_FAILURE;
	}

	return app.run();
}

extern "C"
{
	OgeViewport *ogeShellInterface_CreateWindow( const char *title, int width, int height, bool fullscreen, uint8_t mode )
	{
		return nullptr;
	}

	void ogeShellInterface_GetWindowSize( int *width, int *height ) {}
	void YnCore_ShellInterface_DisplayMessageBox( YNCoreMessageType messageType, const char *message, ... )
	{
	}

	YNCoreInputState YnCore_ShellInterface_GetButtonState( YNCoreInputButton inputButton ) { return YN_CORE_INPUT_STATE_NONE; }
	YNCoreInputState YnCore_ShellInterface_GetKeyState( int key ) { return YN_CORE_INPUT_STATE_NONE; }
	void ogeShellInterface_GetMousePosition( int *x, int *y ) {}
	void YnCore_ShellInterface_SetMousePosition( int x, int y ) {}
	void YnCore_ShellInterface_GrabMouse( bool grab ) {}

	void YnCore_ShellInterface_PushMessage( int level, const char *msg, const PLColour *colour )
	{
		os::editor::mainWindow->PushMessage( level, msg, *colour );
	}

	void ogeShellInterface_Shutdown( void ) {}
}

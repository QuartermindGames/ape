// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Main entry point for Craft editor.
// Author:  Mark E. Sowden

#include <QApplication>
#include <QToolBar>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>

#include "plcore/pl_filesystem.h"

#include "aux/public/aux_log.h"

#include "core/public/ape/ape_public_world.h"

#include "craft_private.h"
#include "craft_project.h"

/* Unlike Forge, which was a bit of a disaster, I'm trying to design this a little
 * more appropriately as a library. This means there isn't a main or anything like
 * that; the caller initializes the library and then calls to process events, then
 * shuts it all down once done.
 *
 * In the long term, it'd be great to be able to launch the editor within the game
 * via the console. And this design should *hopefully* make that a more practical.
 */

static QApplication *app;

static CraftMainWindow *mainWindow;

/////////////////////////////////////////////////////////////////////////////////////

void CraftMainWindow::show_about()
{
	QString aboutMessage = "<h3>Craft Editor</h3><hr>"
	                       "<p>Craft is an editor environment, developed by Quartermind Games, for use with ApeTech.</p>"
	                       "<p>This software uses the <a href=\"https://www.qt.io/development/qt-framework\">Qt Framework</a>.</p>";
	aboutMessage += "<p><b>Version:</b> <code>" +
	                QString::number( CRAFT_VERSION[ 0 ] ) + "." +
	                QString::number( CRAFT_VERSION[ 1 ] ) + "." +
	                QString::number( CRAFT_VERSION[ 2 ] ) + " " GIT_COMMIT_COUNT " (" GIT_BRANCH ")</code></p>";
	aboutMessage += "<p>Copyright &copy; 2020-2026 Quartermind Games & Contributors</p>";

	QMessageBox::about( this, QString( "About " ) + CRAFT_TITLE, aboutMessage );
}

void CraftMainWindow::open_room()
{
	const QString projectPath = com_project_get_local_path();
	const QString filename    = QFileDialog::getOpenFileName( this, "Open Room", projectPath + "/dev/rooms/", "Room Files (*." APE_WORLD_ROOM_EXTENSION ")" );
	if ( filename.isEmpty() )
	{
		return;
	}
}

void CraftMainWindow::show_project_picker()
{
	if ( picker_ == nullptr )
	{
		picker_ = new CraftProjectPicker( this );
	}

	picker_->show();
	picker_->setFocus();
}

CraftMainWindow::CraftMainWindow( QWidget *parent ) : QMainWindow( parent )
{
	resize( 1024, 768 );

	// file menu
	QMenu *fileMenu = menuBar()->addMenu( "&File" );
	fileMenu->addAction( "New &Room..." );
	fileMenu->addAction( "Open Room...", this, &CraftMainWindow::open_room );

	fileMenu->addSeparator();
	fileMenu->addAction( "&Save..." );
	fileMenu->addAction( "Save &As..." );

	fileMenu->addSeparator();
	fileMenu->addAction( "&Exit", this, &QMainWindow::close );

	// help menu
	QMenu *helpMenu = menuBar()->addMenu( "&Help" );
	helpMenu->addAction( "&About", this, &CraftMainWindow::show_about );

	tabs_ = new QTabWidget( this );
	tabs_->setTabsClosable( true );

	setCentralWidget( tabs_ );
}

/////////////////////////////////////////////////////////////////////////////////////

std::string craft_paths[ CRAFT_PATH_MAX ];

static int craftLogInfo;
static int craftLogWarning;
static int craftLogError;

extern "C" bool craft_initialize( int argc, char **argv )
{
	craftLogInfo    = aux_log_register_source( "craft", PL_COLOUR_WHITE, true );
	craftLogWarning = aux_log_register_source( "craft", PL_COLOUR_YELLOW, true );
	craftLogError   = aux_log_register_source( "craft", PL_COLOUR_RED, true );

	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) == nullptr )
	{
		fprintf( stderr, "Failed to get executable directory: %s\n", PlGetError() );
		return false;
	}

	craft_paths[ CRAFT_PATH_EXE ]       = exePath;
	craft_paths[ CRAFT_PATH_RESOURCES ] = craft_paths[ CRAFT_PATH_EXE ] + "/../../resources";
	craft_paths[ CRAFT_PATH_PROJECTS ]  = craft_paths[ CRAFT_PATH_EXE ] + "/../../projects";

	app = new QApplication( argc, argv );

	mainWindow = new CraftMainWindow();
	mainWindow->show();
	mainWindow->setFocus();

	if ( !aux_project_is_mounted() )
	{
		mainWindow->show_project_picker();
	}

	return true;
}

extern "C" void craft_process_events()
{
	app->processEvents();
}

void craft_print_( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );

	const QString out;
	out.vasprintf( msg, args );
	va_end( args );

	if ( out.isEmpty() )
	{
		return;
	}

	aux_log_push_message( craftLogInfo, "%s\n", out.data() );
}

void craft_print_warning_( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );

	const QString out;
	out.vasprintf( msg, args );
	va_end( args );

	if ( out.isEmpty() )
	{
		return;
	}

	aux_log_push_message( craftLogWarning, "%s\n", out.data() );
}

void craft_print_error_( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );

	const QString out;
	out.vasprintf( msg, args );
	va_end( args );

	if ( out.isEmpty() )
	{
		return;
	}

	aux_log_push_message( craftLogError, "%s\n", out.data() );

	exit( EXIT_FAILURE );
}

extern "C" void craft_prompt_warning( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );

	const QString out;
	out.vasprintf( msg, args );
	va_end( args );

	if ( out.isEmpty() )
	{
		return;
	}

	aux_log_push_message( craftLogWarning, "%s\n", out.data() );

	QMessageBox::warning( mainWindow, QString( CRAFT_TITLE ) + " Warning", out );
}

extern "C" void craft_prompt_error( const char *msg, ... )
{
	va_list args;
	va_start( args, msg );

	const QString out;
	out.vasprintf( msg, args );
	va_end( args );

	if ( out.isEmpty() )
	{
		return;
	}

	aux_log_push_message( craftLogError, "%s\n", out.data() );

	QMessageBox::critical( mainWindow, QString( CRAFT_TITLE ) + " Error", out );

	exit( EXIT_FAILURE );
}

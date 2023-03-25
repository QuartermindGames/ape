// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_window_main.h"
#include "editor_window_material.h"
#include "editor_window_model.h"

#include <FXGLVisual.h>

os::editor::MainWindow *os::editor::mainWindow = nullptr;

FXDEFMAP( os::editor::MainWindow )
MainWindowMap[] = {
        //FXMAPFUNC( SEL_CONFIGURE, MainWindow::ID_CANVAS, mao::MainWindow::OnConfigure ),
        //FXMAPFUNC( SEL_PAINT, MainWindow::ID_CANVAS, mao::MainWindow::OnExpose ),
        //FXMAPFUNC( SEL_CHORE, MainWindow::ID_TIMEOUT, mao::MainWindow::OnTimeout ),
        FXMAPFUNC( SEL_COMMAND, os::editor::MainWindow::ID_PROJECT_NEW, os::editor::MainWindow::OnNew ),
        FXMAPFUNC( SEL_COMMAND, os::editor::MainWindow::ID_PROJECT_OPEN, os::editor::MainWindow::OnOpen ),

        FXMAPFUNC( SEL_COMMAND, os::editor::MainWindow::ID_MODEL_OPEN, os::editor::MainWindow::OpenModel ),
        FXMAPFUNC( SEL_COMMAND, os::editor::MainWindow::ID_MATERIAL_OPEN, os::editor::MainWindow::OpenMaterial ),

        FXMAPFUNC( SEL_COMMAND, os::editor::MainWindow::ID_ABOUT, os::editor::MainWindow::OnAbout ),
        FXMAPFUNC( SEL_COMMAND, os::editor::MainWindow::ID_PROJECT_PACKAGE, os::editor::MainWindow::OnPackageProject ),
        //FXMAPFUNC( SEL_COMMAND, MainWindow::ID_TOGGLE_EDIT, mao::MainWindow::OnToggleEdit ),
        //FXMAPFUNC( SEL_KEYRELEASE, MainWindow::ID_CANVAS, mao::MainWindow::OnInput ),
        FXMAPFUNC( SEL_TIMEOUT, os::editor::MainWindow::ID_TICK, os::editor::MainWindow::OnTick ),
};

FXIMPLEMENT( os::editor::MainWindow, FXMainWindow, MainWindowMap, ARRAYNUMBER( MainWindowMap ) )

os::editor::MainWindow::MainWindow( FXApp *app )
    : FXMainWindow( app, EDITOR_APP_TITLE, nullptr, nullptr, DECOR_ALL, 0, 0, 1024, 768, 0, 0 )
{
	menuBar_ = new FXMenuBar( this, LAYOUT_SIDE_TOP | LAYOUT_FILL_X );

	auto *menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuCommand( menuPane, "New World\t\tCreate a new world.", nullptr, this, ID_WORLD_NEW );
	new FXMenuCommand( menuPane, "Open World\t\tOpen an existing world.", nullptr, this, ID_WORLD_OPEN );
	new FXMenuCommand( menuPane, "Save World\t\tSave the world.", nullptr, this, ID_WORLD_SAVE );
	new FXMenuCommand( menuPane, "Save World As...\t\tSave the world to the specified destination.", nullptr, this, ID_WORLD_SAVEAS );
	new FXMenuCommand( menuPane, "Close World\t\tClose the current world.", nullptr, this, ID_WORLD_CLOSE );
	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "Open Model\t\tOpen an existing model.", nullptr, this, ID_MODEL_OPEN );
	new FXMenuCommand( menuPane, "Open Material\t\tOpen an existing material.", nullptr, this, ID_MATERIAL_OPEN );
	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "Package Project\t\tPackage the current project.", nullptr, this, ID_PROJECT_PACKAGE );
	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "Settings\t\tConfigure editor settings and more.", nullptr, this );
	new FXMenuSeparator( menuPane );
	new FXMenuCommand( menuPane, "&Quit\t\tQuit the application.", nullptr, this, ID_CLOSE );
	new FXMenuTitle( menuBar_, "&File", nullptr, menuPane );

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuTitle( menuBar_, "&Edit", nullptr, menuPane );

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuTitle( menuBar_, "&View", nullptr, menuPane );

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuTitle( menuBar_, "&Tools", nullptr, menuPane );

	menuPane = new FXMenuPane( menuBar_->getParent() );
	new FXMenuCommand( menuPane, "&About\t\tOpen about dialog.", nullptr, this, ID_ABOUT );
	new FXMenuTitle( menuBar_, "&Help", nullptr, menuPane );

#if 1
	toolBar_ = new FXToolBar( this, FRAME_RAISED | FRAME_THICK | LAYOUT_FILL_X );
	editModeButtons[ EDITOR_GEOMETRYMODE_BRUSH ]  = new FXToggleButton( toolBar_, "", "", os::editor::LoadFXIcon( getApp(), "resources/brush_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	editModeButtons[ EDITOR_GEOMETRYMODE_VERTEX ] = new FXToggleButton( toolBar_, "", "", os::editor::LoadFXIcon( getApp(), "resources/vertex_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	editModeButtons[ EDITOR_GEOMETRYMODE_EDGE ]   = new FXToggleButton( toolBar_, "", "", os::editor::LoadFXIcon( getApp(), "resources/edge_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	editModeButtons[ EDITOR_GEOMETRYMODE_FACE ]   = new FXToggleButton( toolBar_, "", "", os::editor::LoadFXIcon( getApp(), "resources/face_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	//editModeButtons[ currentEditMode ]->setState( true );
	new FXVerticalSeparator( toolBar_ );
	new FXToggleButton( toolBar_, "", "", os::editor::LoadFXIcon( getApp(), "resources/grid.gif" ), 0, &gridSizeTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXTextField( toolBar_, 4, &gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );
	new FXVerticalSeparator( toolBar_ );
	new FXButton( toolBar_, "", os::editor::LoadFXIcon( getApp(), "resources/play.gif" ) );
#endif

	auto *statusFrame = new FXHorizontalFrame( this, LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 );
	for ( auto &i : statusBars_ )
		i = new FXStatusBar( statusFrame, LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X );

	glVisual_ = new FXGLVisual( getApp(), VISUAL_DOUBLEBUFFER );

	mainFrame = new FXVerticalFrame( this, LAYOUT_FILL );

	auto *vs = new FXSplitter( mainFrame, LAYOUT_MIN_HEIGHT | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_VERTICAL );

	auto *hs = new FXSplitter( vs, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );
	new FXTabBook( new FXVerticalFrame( hs, LAYOUT_SIDE_TOP | LAYOUT_FILL_Y ) );
	viewportFrame = new ViewportFrame( hs, glVisual_, ( YNCoreCameraMode ) YN_CORE_CAMERA_MODE_PERSPECTIVE );
	viewportFrame->setHeight( 768 );

	// Add the console at the bottom
	consoleFrame = new os::editor::ConsoleFrame( vs );

	//mainFrame->hide();

	getApp()->addTimeout( this, MainWindow::ID_TICK, YN_CORE_TICK_RATE );
}

void os::editor::MainWindow::create()
{
	FXMainWindow::create();

	show( PLACEMENT_SCREEN );
	maximize();
}

long os::editor::MainWindow::OnTick( FXObject *, FXSelector, void * )
{
	YnCore_TickFrame();

	getApp()->addTimeout( this, MainWindow::ID_TICK, YN_CORE_TICK_RATE );
	return 0;
}

long os::editor::MainWindow::OnNew( FXObject *, FXSelector, void * )
{
	PlParseConsoleString( "editor.create_world" );
	return 0;
}

long os::editor::MainWindow::OnOpen( FXObject *, FXSelector, void * )
{
	return 0;
}

long os::editor::MainWindow::OpenModel( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getOpenFilename( this, "Select an existing model", "./", "*.model.n" );
	if ( filename.empty() )
	{
		return false;
	}

	return true;
}

long os::editor::MainWindow::OpenMaterial( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getOpenFilename( this, "Select an existing material", "./", "*.mat.n" );
	if ( filename.empty() )
	{
		return false;
	}

	YNCoreMaterial *material = YnCore_Material_Cache( filename.text(), YN_CORE_CACHE_GROUP_WORLD, false, false );
	if ( material == nullptr )
	{
		return false;
	}

	if ( materialWindow != nullptr )
	{
		materialWindow->destroy();
		delete materialWindow;
	}

	materialWindow = new MaterialWindow( getApp(), material );
	materialWindow->show();

	return true;
}

long os::editor::MainWindow::OnAbout( FXObject *, FXSelector, void * )
{
	return 0;
}

long os::editor::MainWindow::OnPackageProject( FXObject *, FXSelector, void * )
{
	mainFrame->show();
	mainFrame->recalc();
	return 0;
}

/**
 * Push a message to the console.
 */
void os::editor::MainWindow::PushMessage( int level, const char *msg, const PLColour &colour )
{
	consoleFrame->PushMessage( level, msg, colour );
}

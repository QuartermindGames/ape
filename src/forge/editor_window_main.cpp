// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "editor_window_main.h"
#include "editor_window_material.h"
#include "editor_window_model.h"
#include "editor_dialog_about.h"

#include <FXGLVisual.h>

ss::forge::MainWindow *ss::forge::mainWindow = nullptr;

FXDEFMAP( ss::forge::MainWindow )
MainWindowMap[] = {
        //FXMAPFUNC( SEL_CONFIGURE, MainWindow::ID_CANVAS, mao::MainWindow::OnConfigure ),
        //FXMAPFUNC( SEL_PAINT, MainWindow::ID_CANVAS, mao::MainWindow::OnExpose ),
        //FXMAPFUNC( SEL_CHORE, MainWindow::ID_TIMEOUT, mao::MainWindow::OnTimeout ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::MainWindow::ID_WORLD_NEW, ss::forge::MainWindow::on_new ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::MainWindow::ID_WORLD_OPEN, ss::forge::MainWindow::on_open ),

        FXMAPFUNC( SEL_COMMAND, ss::forge::MainWindow::ID_MODEL_OPEN, ss::forge::MainWindow::open_model ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::MainWindow::ID_MATERIAL_OPEN, ss::forge::MainWindow::open_material ),

        FXMAPFUNC( SEL_COMMAND, ss::forge::MainWindow::ID_ABOUT, ss::forge::MainWindow::on_about ),
        FXMAPFUNC( SEL_COMMAND, ss::forge::MainWindow::ID_PROJECT_PACKAGE, ss::forge::MainWindow::on_package_project ),
        //FXMAPFUNC( SEL_COMMAND, MainWindow::ID_TOGGLE_EDIT, mao::MainWindow::OnToggleEdit ),
        //FXMAPFUNC( SEL_KEYRELEASE, MainWindow::ID_CANVAS, mao::MainWindow::OnInput ),
        FXMAPFUNC( SEL_TIMEOUT, ss::forge::MainWindow::ID_TICK, ss::forge::MainWindow::on_tick ),
};

FXIMPLEMENT( ss::forge::MainWindow, FXMainWindow, MainWindowMap, ARRAYNUMBER( MainWindowMap ) )

ss::forge::MainWindow::MainWindow( FXApp *app )
    : FXMainWindow( app, SS_FORGE_APP_TITLE, nullptr, nullptr, DECOR_ALL, 0, 0, 1024, 768, 0, 0 )
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
	new FXMenuCommand( menuPane, "Open Texture\t\tOpen an existing texture.", nullptr, this, ID_TEXTURE_OPEN );
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
	editModeButtons[ EDITOR_GEOMETRYMODE_BRUSH ] = new FXToggleButton( toolBar_, "", "", ss::forge::load_fx_icon( getApp(), "resources/brush_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	editModeButtons[ EDITOR_GEOMETRYMODE_VERTEX ] = new FXToggleButton( toolBar_, "", "", ss::forge::load_fx_icon( getApp(), "resources/vertex_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	editModeButtons[ EDITOR_GEOMETRYMODE_EDGE ] = new FXToggleButton( toolBar_, "", "", ss::forge::load_fx_icon( getApp(), "resources/edge_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	editModeButtons[ EDITOR_GEOMETRYMODE_FACE ] = new FXToggleButton( toolBar_, "", "", ss::forge::load_fx_icon( getApp(), "resources/face_mode.gif" ), 0, this, MainWindow::ID_TOGGLE_EDIT, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	//editModeButtons[ currentEditMode ]->setState( true );
	new FXVerticalSeparator( toolBar_ );
	new FXToggleButton( toolBar_, "", "", ss::forge::load_fx_icon( getApp(), "resources/grid.gif" ), 0, &gridSizeTarget, FXDataTarget::ID_VALUE, TOGGLEBUTTON_KEEPSTATE | TOGGLEBUTTON_NORMAL );
	new FXTextField( toolBar_, 4, &gridSizeTarget, FXDataTarget::ID_VALUE, TEXTFIELD_LIMITED | TEXTFIELD_INTEGER | FRAME_NORMAL );
	new FXVerticalSeparator( toolBar_ );
	new FXButton( toolBar_, "", ss::forge::load_fx_icon( getApp(), "resources/play.gif" ) );
#endif

	new FXStatusBar( this, LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X );

	glVisual_ = new FXGLVisual( getApp(), VISUAL_DOUBLEBUFFER );

	mainFrame = new FXVerticalFrame( this, LAYOUT_FILL );

	auto *vs = new FXSplitter( mainFrame, LAYOUT_MIN_HEIGHT | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_VERTICAL );

	unsigned int mode = SS_ARL_CAMERA_MODE_PERSPECTIVE;
	auto *hs = new FX4Splitter( vs, LAYOUT_MIN_WIDTH | LAYOUT_SIDE_TOP | LAYOUT_FILL | SPLITTER_HORIZONTAL );
	for ( auto &i : viewportFrame )
		i = new ViewportFrame( hs, glVisual_, ( SSArlCameraMode ) mode++ );

	hs->setHeight( 720 );

	// Add the console at the bottom
	consoleFrame = new ss::forge::ConsoleFrame( vs );

	getApp()->addTimeout( this, MainWindow::ID_TICK, SS_SHELL_TICK_RATE );
}

void ss::forge::MainWindow::create()
{
	FXMainWindow::create();

	show( PLACEMENT_SCREEN );
	maximize();
}

long ss::forge::MainWindow::on_tick( FXObject *, FXSelector, void * )
{
	ss_acl_tick_frame();

	getApp()->addTimeout( this, MainWindow::ID_TICK, SS_SHELL_TICK_RATE );
	return 0;
}

long ss::forge::MainWindow::on_new( FXObject *, FXSelector, void * )
{
	PlParseConsoleString( "editor.create_world" );
	return 0;
}

long ss::forge::MainWindow::on_open( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getOpenFilename( this, "Select a world", FXString( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ] ) + "/", "*.wld.n" );
	if ( filename.empty() )
		return false;

	ss_acl_level_load( filename.text() );

	return 0;
}

long ss::forge::MainWindow::open_model( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getOpenFilename( this, "Select an existing model", FXString( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ] ) + "/", "*.mdl.n" );
	if ( filename.empty() )
		return false;

	return true;
}

long ss::forge::MainWindow::open_texture( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getOpenFilename( this, "Select an existing texture", FXString( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ] ) + "/", "*.png" );
	if ( filename.empty() )
		return false;

	return 0;
}

long ss::forge::MainWindow::open_material( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getOpenFilename( this, "Select an existing material", FXString( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ] ) + "/", "*.mat.n" );
	if ( filename.empty() )
		return false;

	ApeMaterial *material = ss_arl_material_cache( filename.text(), APE_CACHE_EDITOR, false, false );
	if ( material == nullptr )
	{
		FXMessageBox::warning( FXApp::instance(), MBOX_OK, "Warning", "Failed to load material (%s)!", filename.text() );
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

long ss::forge::MainWindow::on_about( FXObject *, FXSelector, void * )
{
	forge::AboutDialog *aboutDialog = new forge::AboutDialog( this );
	aboutDialog->execute();
	return true;
}

long ss::forge::MainWindow::on_package_project( FXObject *, FXSelector, void * )
{
	FXString filename = FXFileDialog::getSaveFilename( this, "Select a destination", FXString( ss::forge::cachedPaths[ ss::forge::PATH_PROJECTS ] ) + "/", "*.pkg" );
	if ( filename.empty() )
		return false;

	return true;
}

void ss::forge::MainWindow::setup_engine_viewports()
{
	for ( auto i : viewportFrame )
		i->setup_engine_viewport();
}

/**
 * Push a message to the console.
 */
void ss::forge::MainWindow::push_message( int level, const char *msg, const PLColour &colour )
{
	consoleFrame->push_message( level, msg, colour );
}

/**
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
**/

#pragma once

#include <fx3d.h>

#define MAX_VIEWPORTS 4

namespace huang {
	class Viewport;
	class MainWindow : public FXMainWindow {
		FXDECLARE( MainWindow )
	private:
		FXToggleButton *editModeButtons[ MAX_EDIT_MODES ];
		uint8_t currentEditMode{ EDIT_MODE_VERTEX };

		FXDataTarget myGridSizeTarget;
		FXDataTarget myGridStateTarget;

		FXDataTarget showNamesTarget;
		FXDataTarget showCoordinatesTarget;
		FXDataTarget showLightsTarget;
		FXDataTarget showPathsTarget;
		FXDataTarget showWaterTarget;
		FXDataTarget showWorldTarget;
		FXDataTarget showActorsTarget;

	protected:
		FXToolBar *toolBar{ nullptr };
		FXMenuBar *menubar{ nullptr };

		FXMenuPane *filemenu{ nullptr };
		FXMenuPane *editMenu{ nullptr };
		FXMenuPane *viewMenu{ nullptr };

		FX4Splitter *splitter{ nullptr };
		FX4Splitter *subsplitter{ nullptr };

		FXTabBook *tabBook{ nullptr };

		FXGLVisual *glVisual{ nullptr };

		Viewport *viewports[ MAX_VIEWPORTS ]{
			nullptr, 
			nullptr, 
			nullptr, 
			nullptr 
		};

		MainWindow() {}
		
	public:
		MainWindow( FXApp *a );
		virtual void create();
		virtual ~MainWindow();

		long OnExpose( FXObject *, FXSelector, void * );
		long OnTimeout( FXObject *, FXSelector, void * );
		long OnConfigure( FXObject *, FXSelector, void * );

		long OnCmdNew( FXObject *, FXSelector, void * );
		long OnCmdOpen( FXObject *, FXSelector, void * );
		long OnCmdAbout( FXObject *, FXSelector, void * );

		long OnToggleEdit( FXObject *, FXSelector, void * );

		long OnInput( FXObject *, FXSelector, void * );

		void ResetViews();

		void CreateWorld();
		void LoadWorld( const char *path );

		uint8_t GetEditMode() const { return currentEditMode; }

		enum {
			ID_CANVAS = FXMainWindow::ID_LAST,

			ID_NEW,
			ID_OPEN,
			
			ID_COPY,
			ID_PASTE,

			ID_GRID_TOGGLE,
			ID_TOGGLE_EDIT,

			ID_TIMEOUT,
			ID_OPENGL,
			ID_ABOUT,
		};
	};
}

extern huang::MainWindow *g_mainWindow;

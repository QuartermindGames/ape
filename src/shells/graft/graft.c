// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Graft editor.
// Author:  Mark E. Sowden

#include "graft.h"

#include "plcore/pl_filesystem.h"

#include "plgraphics/plg.h"
#include "plgraphics/plg_driver_interface.h"

#include "shells/sdl3/shell_sdl3.c"

static SDL_GLContext sdlGLContext;
static ShellWindow  *mainWindow;

/////////////////////////////////////////////////////////////////////////////////////
// Engine Callbacks
/////////////////////////////////////////////////////////////////////////////////////

void shell_set_mouse_position( int x, int y )
{
}

void shell_get_window_size( int *width, int *height )
{
}

void ss_shell_grab_mouse( bool grab )
{
}

float shell_get_display_scale()
{
	return 1.0f;
}

ApeViewport *ss_shell_viewport_get_active( void )
{
	return nullptr;
}

void ss_shell_shutdown()
{
	shell_shutdown();

	if ( sdlGLContext != nullptr )
	{
		SDL_GL_DestroyContext( sdlGLContext );
		sdlGLContext = nullptr;
	}

	qm_os_memory_free( mainWindow );
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

static bool setup_display()
{
	const char *projectName = com_project_get_name();
	char       *title       = qm_os_string_alloc( "%s (%s)", GRAFT_NAME, projectName );
	mainWindow              = shell_window_create( title, 640, 480, false );

	qm_os_memory_free( title );

	if ( mainWindow == nullptr )
	{
		return false;
	}

	PlgInitializeGraphics();

	// attempt to fetch the driver directly from the executable location if possible
	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) != NULL )
	{
		char *driverPath = qm_os_string_alloc( "local://%s", exePath );
		PlgScanForDrivers( driverPath );
		qm_os_memory_free( driverPath );
	}
	else
	{
		PlgScanForDrivers( "." );
	}

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 4 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );

	if ( ( sdlGLContext = SDL_GL_CreateContext( mainWindow->handle ) ) == nullptr )
	{
		fprintf( stderr, "Failed to create GL context: %s\n", SDL_GetError() );
		return false;
	}

	SDL_GL_MakeCurrent( mainWindow->handle, sdlGLContext );
	SDL_GL_SetSwapInterval( -1 );

	//TODO: we're just hardcoding this until this is all redone
	static constexpr char DRIVER[] = "opengl";
	if ( PlgSetDriver( DRIVER ) != PL_RESULT_SUCCESS )
	{
		return false;
	}

	shell_window_maximize( mainWindow );

	return true;
}

static void tick_frame()
{
}

static void draw_frame( ShellWindow *window )
{
	ape_render_frame( window->viewport );

	SDL_GL_SwapWindow( mainWindow->handle );
}

int qm_os_main( const int argc, char **argv )
{
	if ( !shell_initialize( argc, argv ) )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to initialize shell!\n" );
		return EXIT_FAILURE;
	}

	if ( !setup_display() )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to setup display!\n" );
		return EXIT_FAILURE;
	}

	if ( !ape_initialize( argc, argv, "editor" ) )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to initialize engine!\n" );
		return EXIT_FAILURE;
	}

	// and now we can set up our viewport
	// but seriously, we should be able to init ape before the display!
	unsigned int w, h;
	shell_window_get_size( mainWindow, &w, &h );
	if ( ( mainWindow->viewport = ape_viewport_create( 0, 0, w, h, mainWindow->handle, false ) ) == nullptr )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to setup viewport!\n" );
		return EXIT_FAILURE;
	}

	if ( !shell_setup_tick_timer() )
	{
		shell_display_message( SS_SHELL_MESSAGE_BOX_TYPE_ERROR, "Failed to setup ticket timer!" );
		return EXIT_FAILURE;
	}

	while ( ape_is_running() )
	{
		COM_PROFILE_START( "frametime" );

		SDL_Event event;
		while ( SDL_PollEvent( &event ) )
		{
			switch ( event.type )
			{
				default:
					break;

				case SDL_EVENT_USER:
				{
					tick_frame();
					break;
				}

					// input

				case SDL_EVENT_TEXT_INPUT:
				{
					ape_input_handle_text_event( event.text.text );
					break;
				}
				case SDL_EVENT_MOUSE_WHEEL:
				{
					float x = event.wheel.x > 0 ? 1.0f : event.wheel.x < 0 ? -1.0f
					                                                       : 0.0f;
					float y = event.wheel.y > 0 ? 1.0f : event.wheel.y < 0 ? -1.0f
					                                                       : 0.0f;
					ape_input_handle_mouse_wheel_event( x, y );
					break;
				}
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_MOUSE_BUTTON_UP:
					ape_input_handle_mouse_button_event( event.button.button, event.button.type == SDL_EVENT_MOUSE_BUTTON_DOWN );
					break;
				case SDL_EVENT_MOUSE_MOTION:
					ape_input_handle_mouse_motion_event( event.motion.x, event.motion.y );
					break;
				case SDL_EVENT_KEY_DOWN:
				case SDL_EVENT_KEY_UP:
				{
					int key = shell_translate_key_input( event.key.key );
					if ( key == KEY_INVALID )
					{
						//PrintWarn( "Unhandled key, %d\n", key );
						break;
					}

					ape_input_handle_keyboard_event( key, event.type == SDL_EVENT_KEY_DOWN );
					break;
				}

				case SDL_EVENT_WINDOW_RESIZED:
				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				{
					if ( mainWindow->handle == NULL || event.window.windowID != SDL_GetWindowID( mainWindow->handle ) )
					{
						break;
					}

					//SDL_GetWindowSizeInPixels( sdlWindow, &drawW, &drawH );
					//ape_viewport_set_size( windowViewport, drawW, drawH );
					break;
				}
			}
		}

		draw_frame( mainWindow );

		COM_PROFILE_END( "frametime" );
	}

	return EXIT_SUCCESS;
}

QM_OS_SYSTEM_IMPLEMENT_MAIN()

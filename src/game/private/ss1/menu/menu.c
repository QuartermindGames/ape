// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "qmos/public/qm_os_random.h"

#include "menu.h"

#include "shared/menu/menu.h"
#include "shared/menu/menu_pie.h"
#include "shared/menu/menu_compass.h"

static const char *menuFontPath = "guis/fonts/dejavu_sans_mono_bold_24.fnt";
static ApeGuiFont *menuFont;

static const char *menuTitleFontPath = "guis/fonts/cinzel_decorative_black_64.fnt";
static ApeGuiFont *menuTitleFont;

SS1MenuState ss1_menuState_;

static GameMenu     mainMenu;
static unsigned int currentMenuOption = 0;

static void capture_screenshot_callback( const GameMenuOption * )
{
	// hide the menu so it's not included in the capture
	game_menu_set_active( nullptr );
}

static GameMenuOption debugMenuOptions[] = {
        { "Test Model\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "test_model" } },
        { "Test Net\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "test_net" } },
        { "Test Audio\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "audio_play" } },
        { "Test Audio 3D\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "audio_test_3d" } },
        { nullptr, nullptr, nullptr, GAME_MENU_OPTION_TYPE_SEPERATOR },
        { "Show Lights\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showLights" } },
        { "Show Node Volumes\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "world.showNodeVolumes" } },
        { "Show Portals\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "world.showPortals" } },
        { "Show Face Bounds\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showFaceBounds" } },
        { "Show Face Normals\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showFaceNormals" } },
        { "Wireframe\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.wireframe" } },
        { "Shadow Wireframe\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showShadowWireframe" } },
        { nullptr, nullptr, nullptr, GAME_MENU_OPTION_TYPE_SEPERATOR },
        { "Capture\n", nullptr, capture_screenshot_callback, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "capture" } },
        { "Screenshot\n", nullptr, capture_screenshot_callback, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "screenshot" } },
};
static GameMenu debugMenu = {
        "Debug Menu\n",
        debugMenuOptions,
        QM_OS_ARRAY_ELEMENTS( debugMenuOptions ),
        &mainMenu,
};

static GameMenuOption quitMenuOptions[] = {
        { "Yes\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "quit" } },
        { "No\n", &mainMenu, nullptr, GAME_MENU_OPTION_TYPE_BUTTON },
};
static GameMenu confirmQuitMenu = {
        "Are you sure?\n",
        quitMenuOptions,
        QM_OS_ARRAY_ELEMENTS( quitMenuOptions ),
        &mainMenu,
};

static GameMenuOption startMenuOptions[] = {
        {"art_geom_00\n",    nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "game_load_room rooms/art/art_geom_00.rom.n" }},
        {"test_collision\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "game_load_room test/test_collision" }        },
        {"test_portal\n",    nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "game_load_room test/test_portal" }           },
        {"test_smoothing\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_BUTTON, .button = { "game_load_room test/test_smoothing" }        },
};
static GameMenu startMenu = {
        "Start Server\n",
        startMenuOptions,
        QM_OS_ARRAY_ELEMENTS( startMenuOptions ),
        &mainMenu,
};

static GameMenuOption optionsMenuOptions[] = {
        { "FPS Counter\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showFps" } },

        { nullptr, nullptr, nullptr, GAME_MENU_OPTION_TYPE_SEPERATOR },
        { "Post-Processing\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "postfx" } },
        { "Bloom\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "post_bloom" } },
        { "Dithering\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "post_dither" } },
        { "FXAA\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "postfx_fxaa" } },

        { nullptr, nullptr, nullptr, GAME_MENU_OPTION_TYPE_SEPERATOR },
        { "Stencil Shadows\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.useStencilShadowVolumes" } },
        { "Cap Render Rate to Tick Rate\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderTimeLock" } },

        { nullptr, nullptr, nullptr, GAME_MENU_OPTION_TYPE_SEPERATOR },
        { "Lens Flares\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.flareEnabled" } },

        { nullptr, nullptr, nullptr, GAME_MENU_OPTION_TYPE_SEPERATOR },
        { "Use Qoi for Capture\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "capture.useQoi" } },
};
static GameMenu optionsMenu = {
        "Options\n",
        optionsMenuOptions,
        QM_OS_ARRAY_ELEMENTS( optionsMenuOptions ),
        &mainMenu,
};

static GameMenuOption mainMenuOptions[] = {
        {"Start Server\n", &startMenu,       nullptr, GAME_MENU_OPTION_TYPE_BUTTON},
        {"Options\n",      &optionsMenu,     nullptr, GAME_MENU_OPTION_TYPE_BUTTON},
#if !defined( NDEBUG )
        {"Debug\n",        &debugMenu,       nullptr, GAME_MENU_OPTION_TYPE_BUTTON},
#endif
        {"Quit\n",         &confirmQuitMenu, nullptr, GAME_MENU_OPTION_TYPE_BUTTON},
};
static GameMenu mainMenu = {
        "Main Menu\n",
        mainMenuOptions,
        QM_OS_ARRAY_ELEMENTS( mainMenuOptions ),
};

static GameMenuOption backgroundMenuOptions[] = {
        {"Yes\n", &mainMenu, nullptr, GAME_MENU_OPTION_TYPE_BUTTON},
        {"No\n",  &mainMenu, nullptr, GAME_MENU_OPTION_TYPE_BUTTON},
};
static GameMenu backgroundPrompt = {
        .heading    = "Enable 3D menu background?\n",
        .options    = backgroundMenuOptions,
        .numOptions = QM_OS_ARRAY_ELEMENTS( backgroundMenuOptions ),
        .flags      = GAME_MENU_FLAG_PROMPT | GAME_MENU_FLAG_BACKGROUND,
};

static GamePieMenu *interactPie;

static void initialize_menu( GameMenu *menu )
{
	for ( unsigned int i = 0; i < menu->numOptions; ++i )
	{
		if ( menu->options[ i ].nextMenu != nullptr )
		{
			//initialize_menu( menu->options[ i ].nextMenu );
		}

		if ( menu->options[ i ].type == GAME_MENU_OPTION_TYPE_CHECKBOX )
		{
			menu->options[ i ].checkbox.var = PlGetConsoleVariable( menu->options[ i ].checkbox.varName );
			assert( menu->options[ i ].checkbox.var != nullptr );
		}
	}
}

static void next_menu_option( const GameMenu *menu )
{
	currentMenuOption++;
	if ( currentMenuOption >= menu->numOptions )
	{
		currentMenuOption = 0;
	}
}

static void prev_menu_option( const GameMenu *menu )
{
	if ( currentMenuOption == 0 )
	{
		currentMenuOption = menu->numOptions - 1;
	}
	else
	{
		currentMenuOption--;
	}
}

static void handle_menu_action( ApeInputState state, const char *id )
{
	if ( !( state & APE_INPUT_STATE_PRESSED ) )
	{
		return;
	}

	if ( strcmp( id, "menu_toggle" ) == 0 )
	{
		GameMenu *menu = game_menu_get_active() ? nullptr : &mainMenu;
		game_menu_set_active( menu );
		currentMenuOption = 0;
		return;
	}

	GameMenu *menu = game_menu_get_active();
	if ( menu == nullptr )
	{
		return;
	}

	if ( strcmp( id, "menu_down" ) == 0 )
	{
		do
		{
			next_menu_option( menu );
		} while ( menu->options[ currentMenuOption ].type == GAME_MENU_OPTION_TYPE_SEPERATOR );
	}
	else if ( strcmp( id, "menu_up" ) == 0 )
	{
		do
		{
			prev_menu_option( menu );
		} while ( menu->options[ currentMenuOption ].type == GAME_MENU_OPTION_TYPE_SEPERATOR );
	}
	else if ( strcmp( id, "menu_select" ) == 0 )
	{
		const GameMenuOption *option = &menu->options[ currentMenuOption ];
		if ( option->callback != nullptr )
		{
			option->callback( option );
		}

		switch ( option->type )
		{
			default:
				break;
			case GAME_MENU_OPTION_TYPE_BUTTON:
			{
				if ( option->button.command != nullptr )
				{
					PlParseConsoleString( option->button.command );
				}
				break;
			}
			case GAME_MENU_OPTION_TYPE_CHECKBOX:
			{
				if ( option->checkbox.var != nullptr )
				{
					PlSetConsoleVariable( option->checkbox.var, option->checkbox.var->b_value ? "0" : "1" );
				}
				break;
			}
		}

		if ( option->nextMenu != nullptr )
		{
			menu->lastOption  = currentMenuOption;
			currentMenuOption = option->nextMenu->lastOption;
			game_menu_set_active( option->nextMenu );
		}
	}
	else if ( strcmp( id, "menu_back" ) == 0 )
	{
		if ( menu->parent != nullptr )
		{
			menu->lastOption  = currentMenuOption;
			currentMenuOption = menu->parent->lastOption;
			game_menu_set_active( menu->parent );
		}
	}
}

void ss1_menu_initialize( void )
{
	menuFont = gui_font_load( menuFontPath );
	if ( menuFont == nullptr )
	{
		game_error_( "Failed to load menu font (%s)!\n", menuFontPath );
	}

	menuTitleFont = gui_font_load( menuTitleFontPath );
	if ( menuTitleFont == nullptr )
	{
		game_error_( "Failed to load title font (%s)!\n", menuTitleFontPath );
	}

	game_menu_compass_initialize_( menuTitleFont );

	// mmm delicious pie
	interactPie = menu_pie_create();
	menu_pie_add_option( interactPie, "testing 1", ape_material_cache( "materials/ui/pie/cursor.mat.n", APE_CACHE_GROUP_WORLD, true ), nullptr );
	menu_pie_add_option( interactPie, "testing 2", ape_material_cache( "materials/ui/pie/icon_mouth.mat.n", APE_CACHE_GROUP_WORLD, true ), nullptr );
	menu_pie_add_option( interactPie, "testing 3", ape_material_cache( "materials/ui/pie/icon_tape.mat.n", APE_CACHE_GROUP_WORLD, true ), nullptr );
	//menu_pie_make_active( interactPie, true );

	// setup the splash screens that will get shown on startup
	static const GameMenuSplash splashes[] = {
	        {
             .materialPath = "materials/ui/qm_logo.mat.n",
             .samplePath   = nullptr,
             .fadeInTime   = 2.0f,
             .fadeOutTime  = 2.0f,
	         },
	};
	game_menu_splash_setup_queue_( splashes, QM_OS_ARRAY_ELEMENTS( splashes ) );

	// iterate over and init the menus
	initialize_menu( &mainMenu );
	initialize_menu( &debugMenu );
	initialize_menu( &optionsMenu );

#if 0//TODO: this is what we want to ship with, but the background prompt is going to go

	GameMenu *menu;
	if ( ss1_gameState.isFirstLaunch )
	{
		menu = &backgroundPrompt;
	}
	else
	{
		menu = &startMenu;
	}

	game_menu_set_active( menu );

#else

	game_menu_set_active( &mainMenu );

#endif

	ape_client_input_register_action( "menu_up", &( ApeInputButton ) { APE_INPUT_UP }, 1, &( ApeInputKey ) { APE_INPUT_KEY_UP }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_down", &( ApeInputButton ) { APE_INPUT_DOWN }, 1, &( ApeInputKey ) { APE_INPUT_KEY_DOWN }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_left", &( ApeInputButton ) { INPUT_LEFT }, 1, &( ApeInputKey ) { APE_INPUT_KEY_LEFT }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_right", &( ApeInputButton ) { INPUT_RIGHT }, 1, &( ApeInputKey ) { APE_INPUT_KEY_RIGHT }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_select", &( ApeInputButton ) { INPUT_A }, 1, &( ApeInputKey ) { KEY_ENTER }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_back", &( ApeInputButton ) { INPUT_B }, 1, &( ApeInputKey ) { APE_INPUT_KEY_LEFT }, 1, handle_menu_action );
	ape_client_input_register_action( "menu_toggle", &( ApeInputButton ) { INPUT_START }, 1, &( ApeInputKey ) { APE_INPUT_KEY_ESCAPE }, 0, handle_menu_action );
}

void ss1_menu_shutdown()
{
	ape_gui_font_destroy( menuFont );
}

void ss1_menu_tick( const double delta )
{
	game_menu_splash_tick_( delta );
	menu_pie_tick( interactPie );

	ApeCamera *camera = ss1_gameState.camera;
	if ( camera != nullptr )
	{
		game_menu_compass_tick_( camera, delta );
	}
}

static void draw_dial( const int16_t value, const float radius, const float thickness, const float centerX, const float centerY, const float precision, const QmMathColour4ub *colour )
{
	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
	assert( material != nullptr );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );
	assert( mesh != nullptr );

	static constexpr float RANDOM_VARIATION = 10.0f;

	unsigned int seed = ( unsigned int ) precision;

	float endAngle = ( float ) value / 100.0f * 2.0f * PL_PI;
	for ( float angle = 0.0f; angle <= endAngle; angle += precision )
	{
		float x, y;

		float wobble = cosf( angle * ape_get_num_ticks() / 100.0f ) * 2.0f;

		// outer
		x = centerX + wobble + ( radius + qm_os_random_float( &seed, RANDOM_VARIATION ) ) * cosf( angle );
		y = centerY + wobble + ( radius + qm_os_random_float( &seed, RANDOM_VARIATION ) ) * sinf( angle );
		PlgImmPushVertex( x, y, 0.0f );
		PlgImmColour( colour->r, colour->g, colour->b, colour->a );

		// inner
		x = centerX + wobble + ( ( radius - thickness ) + qm_os_random_float( &seed, RANDOM_VARIATION ) ) * cosf( angle );
		y = centerY + wobble + ( ( radius - thickness ) + qm_os_random_float( &seed, RANDOM_VARIATION ) ) * sinf( angle );
		PlgImmPushVertex( x, y, 0.0f );
		PlgImmColour( colour->r / 2, colour->g / 2, colour->b / 2, colour->a );
	}

	ape_material_draw( material, mesh, nullptr );
}

static void draw_hud( const ApeViewport *viewport )
{
	game_menu_compass_draw_( viewport );

	static const int       health           = 100;
	static constexpr float HEALTH_RADIUS    = 70.0f;
	static constexpr float HEALTH_THICKNESS = 30.0f;
	static float           updateAggro      = 0.0f;

	float x = viewport->width / 2.0f + HEALTH_RADIUS / 2.0f;

	PlPushMatrix();
	PlLoadIdentityMatrix();
	PlTranslateMatrix( qm_math_vector3f( x, viewport->height - ( HEALTH_RADIUS + 20.0f ), 0.0f ) );
	PlRotateMatrix( sinf( ape_get_num_ticks() / 20.0f ) / 40.0f * ( updateAggro + 1.0f ), &QM_MATH_VECTOR3F( 0.0f, 0.0f, 1.0f ) );

	draw_dial( health, HEALTH_RADIUS, HEALTH_THICKNESS, 10.0f, 10.0f, 1.0f, &QM_MATH_COLOUR4UB( 0, 0, 0, 255 ) ); // health
	draw_dial( 100, HEALTH_RADIUS / 2, HEALTH_THICKNESS, 10.0f, 10.0f, 1.0f, &QM_MATH_COLOUR4UB( 0, 0, 0, 255 ) );// stamina

	draw_dial( health, HEALTH_RADIUS, HEALTH_THICKNESS, 0.0f, 0.0f, 1.0f, &QM_MATH_COLOUR4UB( 255, 0, 0, 255 ) ); // health
	draw_dial( 100, HEALTH_RADIUS / 2, HEALTH_THICKNESS, 0.0f, 0.0f, 1.0f, &QM_MATH_COLOUR4UB( 0, 255, 0, 255 ) );// stamina

	PlPopMatrix();
}

void ss1_menu_draw( const ApeViewport *viewport )
{
	static constexpr float MENU_SCALE = 1.0f;

	if ( !game_menu_splash_is_complete_() )
	{
		game_menu_splash_draw_( viewport );
		return;
	}

	GameMenu *menu = game_menu_get_active();
	if ( menu != nullptr )
	{
		assert( menu != nullptr );

		float x = 50.0f;
		float y = 64.0f;

		static constexpr char title[] = "Nihlexa";

		gui_font_set_shadow_offset( 2.0f, 2.0f );
		//gui_font_set_slant( 20.0f );
		gui_font_draw_string( menuTitleFont, x, y, &x, nullptr, MENU_SCALE, &PL_COLOUR_WHITE, title, strlen( title ), true );
		//gui_font_set_slant( 0.0f );

		y = 200.0f;

		if ( menu->flags & GAME_MENU_FLAG_PROMPT )
		{
			float w;
			gui_font_get_string_pixel_size( menuFont, MENU_SCALE, G_STR_( menu->heading ), strlen( menu->heading ), &w, nullptr );
			x = ( viewport->width - w ) / 2.0f;
			y = ( viewport->height - gui_font_get_line_spacing( menuFont ) * 2.0f ) / 2.0f;
		}
		else
		{
			x = 80.0f;
		}

		gui_font_set_shadow_offset( GUI_FONT_SHADOW_DEFAULT );
		gui_font_draw_string( menuFont, x, y, nullptr, &y, 1.0f, &PL_COLOUR_WHITE, G_STR_( menu->heading ), strlen( menu->heading ), true );
		x += 30.0f;
		for ( unsigned int i = 0; i < menu->numOptions; ++i )
		{
			if ( menu->options[ i ].type == GAME_MENU_OPTION_TYPE_SEPERATOR )
			{
				y += gui_font_get_line_spacing( menuFont ) / 2.0f;
				continue;
			}

			char tmp[ 128 ];
			if ( menu->options[ i ].type == GAME_MENU_OPTION_TYPE_CHECKBOX )
			{
				snprintf( tmp, sizeof( tmp ), "[%s] %s", menu->options[ i ].checkbox.var->b_value ? "X" : " ", menu->options[ i ].string );
			}
			else
			{
				snprintf( tmp, sizeof( tmp ), "%s", menu->options[ i ].string );
			}

			size_t len = strlen( tmp );

			static constexpr float SCALE = 0.7f;
			if ( currentMenuOption == i )
			{
				float w;
				gui_font_get_string_pixel_size( menuFont, SCALE, tmp, len, &w, nullptr );
				gui_font_draw_string( menuFont, x + w, y, nullptr, nullptr, SCALE, &PL_COLOUR_GOLD, "<", strlen( "<" ), true );
			}
			gui_font_draw_string( menuFont, x, y, nullptr, &y, SCALE, &PL_COLOUR_WHITE, tmp, len, true );
		}

		if ( menu->flags & GAME_MENU_FLAG_BACKGROUND )
		{
			ape_draw_textured_quad( ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX ),
			                        0.0f, 0.0f,
			                        viewport->width, viewport->height,
			                        &PL_COLOUR_BLACK, 0 );
		}

		gui_font_display( menuFont );
		gui_font_display( menuTitleFont );

		return;
	}

	draw_hud( viewport );

	// draw our fancy little pie menu for interactions
	menu_pie_draw( interactPie, ( float ) viewport->width / 2, ( float ) viewport->height / 2 );
}

// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_random.h"

#include "ape_video.h"

#include "menu/menu.h"
#include "menu/menu.h"
#include "menu/menu_pie.h"
#include "menu/menu_compass.h"

#include "ss1_game.h"

static const char *menuFontPath      = "guis/fonts/dejavu_sans_mono_bold_24.fnt";
static const char *menuTitleFontPath = "guis/fonts/cinzel_decorative_black_64.fnt";

static GameMenu mainMenu;

static void capture_screenshot_callback( const GameMenuOption * )
{
	// hide the menu so it's not included in the capture
	game_menu_set_active( nullptr );
}

static GameMenuOption debugMenuOptions[] = {
        {"Profiler\n",                     nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "gui.profiler" }                },
        {"Cap Render Rate to Tick Rate\n", nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderTimeLock" }              },
        GAME_MENU_OPTION_SEPERATOR(),
        {"Test Model\n",                   nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_BUTTON,   .button = { "test_model" }                    },
        {"Test Net\n",                     nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_BUTTON,   .button = { "test_net" }                      },
        {"Test Audio\n",                   nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_BUTTON,   .button = { "audio_play" }                    },
        {"Test Audio 3D\n",                nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_BUTTON,   .button = { "audio_test_3d" }                 },
        GAME_MENU_OPTION_SEPERATOR(),
        {"Show Lights\n",                  nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showLights" }         },
        {"Show Node Volumes\n",            nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "world.showNodeVolumes" }       },
        {"Show Portals\n",                 nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "world.showPortals" }           },
        {"Show Face Bounds\n",             nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showFaceBounds" }     },
        {"Show Face Normals\n",            nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showFaceNormals" }    },
        {"Wireframe\n",                    nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.wireframe" }          },
        {"Shadow Wireframe\n",             nullptr, nullptr,                     GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showShadowWireframe" }},
        GAME_MENU_OPTION_SEPERATOR(),
        {"Capture\n",                      nullptr, capture_screenshot_callback, GAME_MENU_OPTION_TYPE_BUTTON,   .button = { "capture" }                       },
        {"Screenshot\n",                   nullptr, capture_screenshot_callback, GAME_MENU_OPTION_TYPE_BUTTON,   .button = { "screenshot" }                    },
        GAME_MENU_OPTION_SEPERATOR(),
        {"Save Camera\n",                  nullptr, capture_screenshot_callback, GAME_MENU_OPTION_TYPE_BUTTON,   .button = { "qm1_camera_save_pos" }           },
        {"Restore Camera\n",               nullptr, capture_screenshot_callback, GAME_MENU_OPTION_TYPE_BUTTON,   .button = { "qm1_camera_restore_pos" }        },
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
        {"FPS Counter\n",         nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.showFps" }                },

        GAME_MENU_OPTION_SEPERATOR(),
        {"Post-Processing\n",     nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "postfx" }                          },
        {"Depth of Field\n",      nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "post_dof" }                        },
        {"Bloom\n",               nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "post_bloom" }                      },
        {"Dithering\n",           nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "post_dither" }                     },
        {"FXAA\n",                nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "postfx_fxaa" }                     },

        GAME_MENU_OPTION_SEPERATOR(),
        {"Stencil Shadows\n",     nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.useStencilShadowVolumes" }},

        GAME_MENU_OPTION_SEPERATOR(),
        {"Lens Flares\n",         nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "renderer.flareEnabled" }           },

        GAME_MENU_OPTION_SEPERATOR(),
        {"Use Qoi for Capture\n", nullptr, nullptr, GAME_MENU_OPTION_TYPE_CHECKBOX, .checkbox = { "capture.useQoi" }                  },
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

static ApeGuiFont *menuFont;
static ApeGuiFont *menuTitleFont;

void qm1_menu_hud_initialize_();
void qm1_menu_hud_shutdown_();
void qm1_menu_hud_draw_( const ApeViewport *viewport );

void ss1_menu_initialize_( void )
{
	qm1_menu_hud_initialize_();

	menuFont      = gui_font_load( menuFontPath, gui_get_default_font( GUI_FONT_DEFAULT_MEDIUM ) );
	menuTitleFont = gui_font_load( menuTitleFontPath, gui_get_default_font( GUI_FONT_DEFAULT_LARGE ) );

	game_menu_initialize();
	game_menu_set_title( "Nihlexa" );
	game_menu_set_font( menuFont );
	game_menu_set_title_font( menuTitleFont );

	game_hud_compass_initialize_( menuTitleFont );

	// mmm delicious pie
	interactPie = menu_pie_create();
	menu_pie_add_option( interactPie, "testing 1", ape_material_cache( "materials/ui/pie/cursor.mat.n", APE_CACHE_GROUP_WORLD, true ), nullptr );
	menu_pie_add_option( interactPie, "testing 2", ape_material_cache( "materials/ui/pie/icon_mouth.mat.n", APE_CACHE_GROUP_WORLD, true ), nullptr );
	menu_pie_add_option( interactPie, "testing 3", ape_material_cache( "materials/ui/pie/icon_tape.mat.n", APE_CACHE_GROUP_WORLD, true ), nullptr );
	//menu_pie_make_active( interactPie, true );

	// setup the splash screens that will get shown on startup
	const GameMenuSplash splashes[] = {
	        //GAME_MENU_SPLASH_IMAGE( "materials/ui/logos/logo_qm.mat.n", nullptr, 2.0f, 2.0f ),
	        GAME_MENU_SPLASH_IMAGE( "materials/ui/logos/logo_ape.mat.n", nullptr, 2.0f, 2.0f ),
	        GAME_MENU_SPLASH_VIDEO( "videos/CRE8LOGO.SMK" ),
	};
	game_menu_splash_setup_queue_( splashes, QM_OS_ARRAY_ELEMENTS( splashes ) );

	// iterate over and init the menus
	game_menu_setup( &mainMenu );
	game_menu_setup( &debugMenu );
	game_menu_setup( &optionsMenu );

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
}

void ss1_menu_shutdown_()
{
	qm1_menu_hud_shutdown_();

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

#if 0

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

#endif
}

void ss1_menu_draw( const ApeViewport *viewport )
{
	static constexpr float MENU_SCALE = 1.0f;

	if ( !game_menu_splash_is_complete_() )
	{
		game_menu_splash_draw_( viewport );
		return;
	}

	if ( game_menu_is_open() )
	{
		game_menu_draw_( viewport );
		return;
	}

	qm1_menu_hud_draw_( viewport );

	// draw our fancy little pie menu for interactions
	menu_pie_draw( interactPie, ( float ) viewport->width / 2, ( float ) viewport->height / 2 );
}

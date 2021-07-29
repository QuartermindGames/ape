/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "yin.h"
#include "client.h"
#include "renderer/renderer.h"
#include "audio/audio.h"
#include "game_interface.h"
#include "sgui.h"

void CL_Initialize( void )
{
	R_Initialize();
	A_Initialize();

	Menu_Initialize();
}

void CL_Shutdown( void )
{
	A_Shutdown();
	R_Shutdown();
}

void CL_Display( void )
{
	PROFILE_START( PROFILE_DRAW_ALL );

	R_SetupDefaultState();

	PlgClearBuffers( PLG_BUFFER_DEPTH | PLG_BUFFER_COLOUR );

	Game_Display();

	R_DrawMenu();

	PROFILE_END( PROFILE_DRAW_ALL );
}

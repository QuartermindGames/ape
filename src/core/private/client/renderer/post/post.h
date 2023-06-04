/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

#include "core_private.h"

#include "../renderer.h"

typedef struct PostProcessEffect
{
	void ( *RegisterConsoleVariables )( void );
	bool ( *Setup )( void );
	void ( *Cleanup )( void );
	void ( *Draw )( const ApeViewport *viewport );
} PostProcessEffect;

void R_PP_Cleanup( void );
void R_PP_SetupEffects( void );

void R_PP_RegisterConsoleVariables( void );

void R_PP_PreDraw( void );
void R_PP_Draw( const ApeViewport *viewport );

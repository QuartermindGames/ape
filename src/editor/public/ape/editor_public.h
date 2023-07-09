// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "yin/core_editor.h"

/**
 * Initialize the editor. Should only be called for engine startup.
 */
void edInitialize( void );

/**
 * Register any console variables exposed by the editor.
 */
void edRegisterConsoleVariables( void );

/**
 * Shutdown the editor. Should only be called for engine shutdown.
 */
void edShutdown( void );

/**
 * Tick editor logic.
 */
void edTick( void );

/**
 * Draw scene in edit mode.
 */
void edDraw( void );

/**
 * Returns true if the editor mode is active.
 */
bool edIsActive( void );

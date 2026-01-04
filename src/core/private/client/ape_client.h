// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "ape/ape_public_client.h"

PL_EXTERN_C

void ape_initialize_client_( void );
void ape_shutdown_client_( void );

void ape_render_frame_( ApeViewport *viewport );

void ape_tick_client_( double delta );

PL_EXTERN_C_END

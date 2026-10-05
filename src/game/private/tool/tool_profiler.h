// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

/**
 * Initializes the profiler tool.
 */
void game_tool_profiler_initialize_();

/**
 * Indicates if the profiler tool is active or not.
 */
bool game_tool_profiler_is_active_();

/**
 * Performs the profiling.
 */
void game_tool_profiler_draw_( const ApeViewport *viewport );

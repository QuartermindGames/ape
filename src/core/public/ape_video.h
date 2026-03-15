// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

/*
 * Some things to be aware of about the video API for now.
 * This is just the *bare* minimum. There is currently no streaming,
 * because this isn't really designed for streaming long-form or large
 * videos. If that's desired, we'll probably want to completely redo
 * how this works internally so an interface is provided for whatever
 * specific format we're trying to support...
 */

typedef struct ApeViewport ApeViewport;
typedef struct ApeVideo    ApeVideo;

/**
 * Attempts to load the video video.
 * @param path Path to load from.
 * @return Video instance on success, otherwise null on fail.
 */
ApeVideo *ape_video_load( const char *path );

void ape_video_destroy( ApeVideo *video );

void ape_video_draw( ApeVideo *video, const ApeViewport *viewport );
void ape_video_tick( ApeVideo *video, double delta );

void ape_video_set_playback_state( ApeVideo *video, bool paused );

/**
 * Query if the video is currently playing.
 * @param video Video instance.
 * @return True if the video is playing, otherwise false.
 */
bool ape_video_is_playing( const ApeVideo *video );

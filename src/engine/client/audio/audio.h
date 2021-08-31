/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

/**
 * Public handle for a sound loaded
 * in by the audio system.
 */
typedef struct ASound ASound;

void A_Initialize( void );
void A_Shutdown( void );
void A_CleanupSounds( bool force );

//bool A_IsValidSoundSlot( const ASoundReference *s );

ASound *A_CacheSound( const char *path );
void	A_EmitSound( ASound *s, int8_t volume );
void	A_ReleaseSound( ASound *s );

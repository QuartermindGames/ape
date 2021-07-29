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
typedef struct ASoundReference
{
	int	 slot;
	char path[ PL_SYSTEM_MAX_PATH ];
} ASoundReference;

void A_Initialize( void );
void A_Shutdown( void );
void A_CleanupSounds( bool force );

bool A_IsValidSoundSlot( const ASoundReference *s );

ASoundReference A_CacheSound( const char *path );
void			A_EmitSound( const ASoundReference *s, const PLVector3 *position, const PLVector3 *velocity );
void			A_ReleaseSound( const ASoundReference *s );

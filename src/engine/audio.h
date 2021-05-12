/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

/**
 * Public handle for a sound loaded
 * in by the audio system.
 */
typedef struct ASoundReference {
	int slot;
	char path[ PL_SYSTEM_MAX_PATH ];
} ASoundReference;

void A_Initialize( void );
void A_Shutdown( void );
void A_CleanupSounds( bool force );

bool A_IsValidSoundSlot( const ASoundReference *s );

ASoundReference A_CacheSound( const char *path );
void A_ReleaseSound( const ASoundReference *s );

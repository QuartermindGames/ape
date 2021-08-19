/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <SDL2/SDL_audio.h>

#include "yin.h"
#include "audio.h"

typedef struct ASound
{
	char		 path[ PL_SYSTEM_MAX_PATH ];
	bool		 reserved;
	unsigned int length;
	uint8_t *	 buffer;
	int			 numReferences;
} ASound;
ASound *	 audioSounds = NULL;
unsigned int numSounds	 = 0;
unsigned int maxSounds	 = 4096;

static SDL_AudioSpec	 sdlAudioSpec;
static SDL_AudioDeviceID sdlAudioDeviceId;

static bool audioInitialized = false;

void A_Initialize( void )
{
	if ( audioInitialized )
		return;

	Print( "Initializing audio\n" );

	SDL_AudioSpec desiredSpec;
	SDL_zero( desiredSpec );
	desiredSpec.channels = 2;
	desiredSpec.format	 = AUDIO_F32;
	desiredSpec.freq	 = 48000;
	desiredSpec.samples	 = 4096;
	desiredSpec.callback = NULL;

	sdlAudioDeviceId = SDL_OpenAudioDevice( NULL, 0, &desiredSpec, &sdlAudioSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE );
	if ( sdlAudioDeviceId == 0 )
	{
		PrintWarn( "Failed to open audio device!\nSDL: %s\n", SDL_GetError() );
		return;
	}

	/* allocate storage for our samples */
	audioSounds = globalSystem.CAlloc( maxSounds, sizeof( ASound ), true );

	audioInitialized = true;
}

bool A_IsValidSoundSlot( const ASoundReference *s )
{
	if ( s->slot == -1 || s->slot >= maxSounds )
		return false;

	ASound *sound = &audioSounds[ s->slot ];
	return ( strcmp( s->path, sound->path ) == 0 );
}

static void A_FreeSound( unsigned int s )
{
	audioSounds[ s ].path[ 0 ] = '\0';

	SDL_FreeWAV( audioSounds[ s ].buffer );
	audioSounds[ s ].buffer = NULL;
	audioSounds[ s ].length = 0;

	audioSounds[ s ].reserved = false;

	numSounds--;
}

void A_CleanupSounds( bool force )
{
	/* if we're not forcing cleanup, allocate a
     * new sound list to fill with the ones we
     * will retain... */
	maxSounds = numSounds;
	ASound *newAudioSounds;
	if ( !force )
		newAudioSounds = globalSystem.CAlloc( maxSounds, sizeof( ASound ), true );

	unsigned int j = 0;
	for ( unsigned int i = 0; i < numSounds; ++i )
	{
		if ( audioSounds[ i ].numReferences > 0 && !force )
		{
			newAudioSounds[ j++ ] = audioSounds[ i ];
			continue;
		}

		if ( force && audioSounds[ i ].numReferences > 0 )
			PrintWarn( "Force cleaning dirty slot %d!\n", i );

		SDL_FreeWAV( audioSounds[ i ].buffer );
		audioSounds[ i ].buffer = NULL;
		audioSounds[ i ].length = 0;

		/* mark it as unreserved, so we can utilise it again later */
		audioSounds[ i ].reserved = false;
	}

	numSounds = j;
	if ( !force )
	{
		globalSystem.Free( audioSounds );
		audioSounds = newAudioSounds;
	}
}

static int A_FetchCachedSoundSlotByPath( const char *path )
{
	for ( int i = 0; i < numSounds; ++i )
	{
		if ( pl_strcasecmp( path, audioSounds[ i ].path ) != 0 )
			continue;

		return i;
	}

	return -1;
}

static ASoundReference A_SetupReference( int slot, const char *path )
{
	ASoundReference s;
	memset( &s, 0, sizeof( ASoundReference ) );
	s.slot = slot;
	if ( path != NULL )
		strncpy( s.path, path, sizeof( s.path ) - 1 );

	return s;
}

/**
 * Fetches and adds the specified sound to memory,
 * if it's not in there already.
 *
 * Be sure to release the sound once you're done with
 * it!
 */
ASoundReference A_CacheSound( const char *path )
{
	/* check if it's cached already */
	int s = A_FetchCachedSoundSlotByPath( path );
	if ( s != -1 )
	{
		audioSounds[ s ].numReferences++;
		return A_SetupReference( s, path );
	}

	/* attempt to load the sample into memory */
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		PrintWarn( "Failed to cache sound, \"%s\"!\nPL: %s\n", path, PlGetError() );
		return A_SetupReference( -1, NULL );
	}

	int		 length = ( int ) PlGetFileSize( file );
	uint8_t *buffer = globalSystem.MAlloc( length, true );
	memcpy( buffer, PlGetFileData( file ), length );

	PlCloseFile( file );

	SDL_RWops *rw = SDL_RWFromConstMem( buffer, length );

	/* setup our new sound slot */
	int freeSlot = 0;
	for ( ; freeSlot < maxSounds; ++freeSlot )
	{
		if ( audioSounds[ freeSlot ].reserved )
			continue;

		break;
	}

	if ( freeSlot >= maxSounds )
	{
		maxSounds += 256;
		audioSounds = globalSystem.ReAlloc( audioSounds, maxSounds, true );
	}

	ASound *newSound = &audioSounds[ freeSlot ];
	snprintf( newSound->path, sizeof( newSound->path ), "%s", path );
	SDL_AudioSpec wavSpec;
	bool status = ( SDL_LoadWAV_RW( rw, SDL_TRUE, &wavSpec, &newSound->buffer, &newSound->length ) != NULL );

	SDL_RWclose( rw );
	globalSystem.Free( buffer );

	if ( !status )
	{
		PrintWarn( "Failed to load wav, \"%s\"!\nSDL: %s\n", path, SDL_GetError() );
		return A_SetupReference( -1, NULL );
	}

	numSounds++;

	return A_SetupReference( freeSlot, path );
}

void A_EmitSound( const ASoundReference *s, const PLVector3 *position, const PLVector3 *velocity )
{
	if ( !A_IsValidSoundSlot( s ) )
		return;

	SDL_QueueAudio( sdlAudioDeviceId, audioSounds[ s->slot ].buffer, audioSounds[ s->slot ].length );
}

void A_ReleaseSound( const ASoundReference *s )
{
	if ( !A_IsValidSoundSlot( s ) )
	{
		PrintWarn( "Cannot release sound, invalid slot %d!\n", s );
		return;
	}

	audioSounds[ s->slot ].numReferences--;
	if ( audioSounds[ s->slot ].numReferences <= 0 )
		A_FreeSound( s->slot );
}

void A_Shutdown( void )
{
	if ( !audioInitialized )
		return;

	SDL_CloseAudioDevice( sdlAudioDeviceId );
	sdlAudioDeviceId = 0;

	audioInitialized = false;
}

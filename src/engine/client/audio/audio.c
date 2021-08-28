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

#if 0
bool A_IsValidSoundSlot( const ASoundReference *s )
{
	if ( s->slot == -1 || s->slot >= maxSounds )
		return false;

	ASound *sound = &audioSounds[ s->slot ];
	return ( strcmp( s->path, sound->path ) == 0 );
}
#endif

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

/**
 * Fetches and adds the specified sound to memory,
 * if it's not in there already.
 *
 * Be sure to release the sound once you're done with
 * it!
 */
ASound *A_CacheSound( const char *path )
{
	/* check if it's cached already */
	int s = A_FetchCachedSoundSlotByPath( path );
	if ( s != -1 )
	{
		audioSounds[ s ].numReferences++;
		return &audioSounds[ s ];
	}

	/* attempt to load the sample into memory */
	PLFile *file = PlOpenFile( path, true );
	if ( file == NULL )
	{
		PrintWarn( "Failed to cache sound, \"%s\"!\nPL: %s\n", path, PlGetError() );
		return NULL;
	}

	int		 length = ( int ) PlGetFileSize( file );
	uint8_t *buffer = globalSystem.MAlloc( length, true );
	memcpy( buffer, PlGetFileData( file ), length );

	PlCloseFile( file );

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

	SDL_RWops *rw = SDL_RWFromConstMem( buffer, length );

	ASound *newSound = &audioSounds[ freeSlot ];
	snprintf( newSound->path, sizeof( newSound->path ), "%s", path );

	SDL_AudioSpec wavSpec;
	bool status = ( SDL_LoadWAV_RW( rw, SDL_TRUE, &wavSpec, &newSound->buffer, &newSound->length ) != NULL );

	//SDL_RWclose( rw );
	//globalSystem.Free( buffer );

	if ( !status )
	{
		PrintWarn( "Failed to load wav, \"%s\"!\nSDL: %s\n", path, SDL_GetError() );
		return NULL;
	}

	numSounds++;

	Print( "Cached sound, \"%s\"\n", path );

	return newSound;
}

void A_EmitSound( ASound *s, const PLVector3 *position, const PLVector3 *velocity )
{
#if 0
	if ( !A_IsValidSoundSlot( s ) )
		return;
#endif
	SDL_QueueAudio( sdlAudioDeviceId, s->buffer, s->length );
}

#if 0
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
#endif

void A_Shutdown( void )
{
	if ( !audioInitialized )
		return;

	SDL_CloseAudioDevice( sdlAudioDeviceId );
	sdlAudioDeviceId = 0;

	audioInitialized = false;
}

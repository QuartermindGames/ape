/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <SDL2/SDL_mixer.h>

#include "yin.h"
#include "audio.h"

static const int MAX_PLAYING_SOUNDS = 16;

typedef struct ASound
{
	char path[ PL_SYSTEM_MAX_PATH ];
	bool reserved;
	Mix_Chunk *sample;
	int numReferences;
	int channel;
} ASound;
ASound *audioSounds = NULL;
unsigned int numSounds = 0;
unsigned int maxSounds = 4096;

static bool audioInitialized = false;

void A_Initialize( void )
{
	if ( audioInitialized )
		return;

	Print( "Initializing audio\n" );

	if ( Mix_OpenAudio( 44100, MIX_DEFAULT_FORMAT, 2, 2048 ) != 0 )
	{
		PrintWarn( "Failed to open audio device!\nSDL: %s\n", Mix_GetError() );
		return;
	}

	Mix_AllocateChannels( MAX_PLAYING_SOUNDS );

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

	Mix_FreeChunk( audioSounds[ s ].sample );
	audioSounds[ s ].sample = NULL;

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

		Mix_FreeChunk( audioSounds[ i ].sample );
		audioSounds[ i ].sample = NULL;

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
		if ( !audioSounds[ i ].reserved )
			continue;

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

	int wav_length = PlGetFileSize( file );
	const void *wav_data = PlGetFileData( file );

	newSound->sample = Mix_LoadWAV_RW( SDL_RWFromConstMem( wav_data, wav_length ), 1 );
	if ( newSound->sample == NULL )
	{
		PrintWarn( "Failed to load wav, \"%s\"!\nMix_LoadWAV_RW: %s\n", path, Mix_GetError() );

		PlCloseFile( file );
		return NULL;
	}

	PlCloseFile( file );

	newSound->reserved = true;
	numSounds++;

	Print( "Cached sound, \"%s\"\n", path );

	return newSound;
}

void A_EmitSound( ASound *s, int8_t volume )
{
	s->channel = Mix_PlayChannel( -1, s->sample, 0 );
	if ( s->channel == -1 )
	{
		PrintWarn( "Mix_PlayChannel: %s\n", Mix_GetError() );
		return;
	}

	Mix_Volume( s->channel, volume );
}

void A_Shutdown( void )
{
	if ( !audioInitialized )
		return;

	Mix_CloseAudio();

	audioInitialized = false;
}

/****************************************
 * Music Player
 ****************************************/

static ASound *music = NULL;

void A_DestroyMusic( void )
{
	if ( music == NULL )
	{
		return;
	}

	A_FreeSound( A_FetchCachedSoundSlotByPath( music->path ) );
	music = NULL;
}

void A_CacheMusic( const char *path )
{
	/* free up anything we cached already */
	A_DestroyMusic();

	music = A_CacheSound( path );
}

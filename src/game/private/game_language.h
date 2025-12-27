// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

typedef struct GameLanguage
{
	char id[ 4 ];
	char description[ 64 ];
} GameLanguage;

/**
 * Returns an array of all the available languages.
 *
 * @param num 	Output number, representing the number of elements in the array.
 * @return 		A pointer to an array of GameLanguage instances.
 */
const GameLanguage **game_language_get_available( unsigned int *num );

/**
 * Sets the current language based on it's internal ID.
 * If the ID is specified as null, it will always use the fallback.
 *
 * @param id	Internal ID of the language desired.
 */
void game_language_set_current( const char *id );

/**
 * Looks up the given string by it's ID, and if it's not found, returns the fallback.
 *
 * @param id 		The ID of the desired string.
 * @param fallback 	The fallback if it's not found.
 * @return 			Either the translated string or the fallback.
 */
const char *game_language_lookup_string( const char *id, const char *fallback );

// A macro just for ease...
#define G_STR( ID, FALLBACK ) game_language_lookup_string( ( ID ), ( FALLBACK ) )
#define G_STR_( ID )          game_language_lookup_string( ( ID ), ( ID ) )

PL_EXTERN_C_END

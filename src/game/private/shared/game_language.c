// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Code for handling language translations.
// Author:  Mark E. Sowden

#include "plcore/pl_array_vector.h"

#include "game_private.h"
#include "game_language.h"

static PLHashTable   *languageStringsTable;
static PLVectorArray *languages;
static char           currentLanguage[ PL_VAR_VALUE_LENGTH ];

static void parse_language_entry( const char *name, AcmBranch *stringsBranch )
{
	AcmBranch *child = acm_branch_get_first_child( stringsBranch );
	while ( child != nullptr )
	{
		const char *id = acm_branch_get_child_string( child, "id", nullptr );
		if ( id == nullptr )
		{
			game_warning_( "Language (%s) entry without a valid ID!\n", name );
			child = acm_get_next_child( child );
			continue;
		}

		const char *value = acm_branch_get_child_string( child, "value", nullptr );
		if ( value == nullptr )
		{
			game_warning_( "Language (%s) entry without a valid value!\n", value );
			child = acm_get_next_child( child );
			continue;
		}

		char *buf = PL_NEW_( char, strlen( value ) + 1 );
		strcpy( buf, value );

		char key[ 64 ];
		snprintf( key, sizeof( key ), "%s:%s", name, id );
		PlInsertHashTableNode( languageStringsTable, key, strlen( key ), buf );

		child = acm_get_next_child( child );
	}
}

static void test_translate_command( unsigned int argc, char **argv )
{
	game_print_( "%s -> %s\n", argv[ 1 ], G_STR_( argv[ 1 ] ) );
}

static void test_command( unsigned int, char ** )
{
	// store the old language
	char tmp[ sizeof( currentLanguage ) ];
	strcpy( tmp, currentLanguage );

	game_language_set_current( "ger" );
	game_print_( "%s", G_STR( "test_message", "Test failed!" ) );

	// restore the original language
	game_language_set_current( tmp );
}

void game_language_initialize_()
{
	PlRegisterConsoleVariable( "language", "The current language.", "", PL_VAR_STRING, currentLanguage, nullptr, true );
	PlRegisterConsoleCommand( "language_test_translate", "Test language translations.", 1, test_translate_command );
	PlRegisterConsoleCommand( "language_test", "Generic test for language translation.", 0, test_command );

	static const char *languagesPath = "scripts/strings.cfg.n";
	AcmBranch         *root          = acm_load_file( languagesPath, "languages" );
	if ( root == nullptr )
	{
		game_warning_( "No languages file found (%s), translations will be unavailable and strings may display incorrectly!\n", languagesPath );
		return;
	}

	languages            = PlCreateVectorArray( 4 );
	languageStringsTable = PlCreateHashTable();

	AcmBranch *child = acm_branch_get_first_child( root );
	while ( child != nullptr )
	{
		const char *id = acm_branch_get_child_string( child, "id", nullptr );
		if ( id == nullptr )
		{
			game_warning_( "Encountered a language entry without a valid name!\n" );
			child = acm_get_next_child( child );
			continue;
		}

		const char *description = acm_branch_get_child_string( child, "description", id );

		AcmBranch *stringsBranch = acm_branch_get_child_by_name( child, "strings" );
		if ( stringsBranch == nullptr )
		{
			game_warning_( "Encountered a language (%s) without any strings!\n", id );
			child = acm_get_next_child( child );
			continue;
		}

		parse_language_entry( id, stringsBranch );

		GameLanguage *language = PL_NEW( GameLanguage );
		snprintf( language->id, sizeof( language->id ), "%s", id );
		snprintf( language->description, sizeof( language->description ), "%s", description );
		PlPushBackVectorArrayElement( languages, language );

		child = acm_get_next_child( child );
	}

	game_print_( "Found %u language/s.\n", PlGetNumVectorArrayElements( languages ) );

	const char *languageConfig = acm_branch_get_child_string( game_get_config(), "language", nullptr );
	game_language_set_current( languageConfig );
}

void game_language_shutdown_()
{
	PlDestroyHashTableEx( languageStringsTable, PlFree );
	PlDestroyVectorArrayEx( languages, PlFree );
}

const GameLanguage **game_language_get_available( unsigned int *num )
{
	return ( const GameLanguage ** ) PlGetVectorArrayDataEx( languages, num );
}

void game_language_set_current( const char *id )
{
	if ( id == nullptr && *currentLanguage == '\0' )
	{
		return;
	}

	PlSetConsoleVariableByName( "language", id == nullptr ? "" : id );
	// Should we bother doing validation here??
}

const char *game_language_lookup_string( const char *id, const char *fallback )
{
	assert( id != nullptr );

	// if the current language is null, we use the default - fallback!
	if ( *currentLanguage == '\0' )
	{
		return fallback;
	}

	static constexpr unsigned int MAX_LOOKUP_SIZE = 1024;
	unsigned int                  requiredSize    = strlen( currentLanguage ) + strlen( id ) + 2;
	if ( requiredSize > MAX_LOOKUP_SIZE )
	{
		game_warning_( "Hit max lookup size for given string (%s)!\n", id );
		return fallback;
	}

	char lookup[ MAX_LOOKUP_SIZE ];
	snprintf( lookup, sizeof( lookup ), "%s:%s", currentLanguage, id );

	const char *result = PlLookupHashTableUserData( languageStringsTable, lookup, strlen( lookup ) );
	if ( result == nullptr )
	{
		game_warning_( "Failed to find translation for string (%s)!\n", lookup );
		result = fallback;
	}

	return result;
}

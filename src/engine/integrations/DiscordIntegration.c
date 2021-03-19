/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

/****************************************
 * DISCORD INTEGRATION
 ****************************************/

#include "yin.h"

#if defined( DISCORD_INTEGRATION )
#if defined( _MSVC_VER )
#pragma pack( push, 8 )
#endif
#include "integrations/discord_game_sdk.h"
#if defined( _MSVC_VER )
#pragma pack( pop )
#endif

#define DISCORD_DLL "discord_game_sdk"
#define DISCORD_DEFAULT_CLIENT_ID 822170320169074719

typedef struct DApplication {
	struct IDiscordCore *core;
	struct IDiscordUserManager *users;
	struct IDiscordAchievementManager *achievements;
	struct IDiscordActivityManager *activities;
	struct IDiscordRelationshipManager *relationships;
	struct IDiscordApplicationManager *application;
	struct IDiscordLobbyManager *lobbies;
	DiscordUserId user_id;
} DApplication;
static DApplication app;

static PLLibrary *discordLib = NULL;

static bool InitializeDiscordInterface( void ) {
	enum EDiscordResult ( *IDiscordCreate )( DiscordVersion version, struct DiscordCreateParams * params, struct IDiscordCore * *result );
	IDiscordCreate = plGetLibraryProcedure( discordLib, "DiscordCreate" );
	if ( IDiscordCreate == NULL ) {
		PrintWarn( "Failed to fetch \"DiscordCreate\" function in Discord Game SDK!\nPL: %s\n", plGetError() );
		return false;
	}

	memset( &app, 0, sizeof( DApplication ) );

	struct IDiscordUserEvents userEvents;
	memset( &userEvents, 0, sizeof( userEvents ) );

	struct IDiscordActivityEvents activitiesEvents;
	memset( &activitiesEvents, 0, sizeof( activitiesEvents ) );

	struct IDiscordRelationshipEvents relationshipsEvents;
	memset( &relationshipsEvents, 0, sizeof( relationshipsEvents ) );

	struct DiscordCreateParams params;
	DiscordCreateParamsSetDefault( &params );
	params.client_id = DISCORD_DEFAULT_CLIENT_ID; /* todo: request ID from game */
	params.flags = DiscordCreateFlags_Default;
	params.event_data = &app;
	params.activity_events = &activitiesEvents;
	params.relationship_events = &relationshipsEvents;
	params.user_events = &userEvents;

	if ( IDiscordCreate( DISCORD_VERSION, &params, &app.core ) != DiscordResult_Ok ) {
		PrintWarn( "Failed to create Discord interface!\n" );
		return false;
	}

	app.users = app.core->get_user_manager( app.core );
	app.achievements = app.core->get_achievement_manager( app.core );
	app.activities = app.core->get_activity_manager( app.core );
	app.application = app.core->get_application_manager( app.core );
	app.lobbies = app.core->get_lobby_manager( app.core );
	app.relationships = app.core->get_relationship_manager( app.core );

	return true;
}

void DiscordIntegration_Initialize( void ) {
	discordLib = plLoadLibrary( DISCORD_DLL, true );
	if ( discordLib == NULL ) {
		PrintWarn( "Failed to find Discord Game SDK!\nPL: %s\n", plGetError() );
		return;
	}

	if ( !InitializeDiscordInterface() ) {
		plUnloadLibrary( discordLib );
		discordLib = NULL;
	}
}

void DiscordIntegration_Tick( void ) {
	if ( discordLib == NULL ) {
		return;
	}

	if ( app.core->run_callbacks( app.core ) != DiscordResult_Ok ) {
		PrintWarn( "Failed to run Discord callbacks!\n" );
	}
}

void DiscordIntegration_Shutdown( void ) {
	if ( discordLib == NULL ) {
		return;
	}

	app.core->destroy( app.core );

	plUnloadLibrary( discordLib );
	discordLib = NULL;
}
#endif

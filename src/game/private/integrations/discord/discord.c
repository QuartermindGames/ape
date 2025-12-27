// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Discord social integration.
// Author:  Mark E. Sowden

#include "../../game_private.h"
#include "../integrations.h"

// Discord third-party SDK
#include "discord_game_sdk.h"

static bool discordEnabled;

static const char *DISCORD_DLL = "discord_game_sdk";
static PLLibrary  *discordLibrary;

typedef struct DiscordAppData
{
	struct IDiscordCore            *core;
	struct IDiscordUsers           *users;
	struct IDiscordActivityManager *activityManager;
} DiscordAppData;
static DiscordAppData discordAppData;

/*
static struct IDiscordUserEvents         discordUserEvents;
static struct IDiscordActivityEvents     discordActivityEvents;
static struct IDiscordRelationshipEvents discordRelationshipEvents;
static struct IDiscordLobbyEvents        discordLobbyEvents;
static struct IDiscordNetworkEvents      discordNetworkEvents;
static struct IDiscordOverlayEvents      discordOverlayEvents;
static struct IDiscordStoreEvents        discordStoreEvents;
static struct IDiscordVoiceEvents        discordVoiceEvents;
static struct IDiscordAchievementEvents  discordAchievementEvents;
*/

static enum EDiscordResult ( *DiscordCreatePtr )( DiscordVersion version, struct DiscordCreateParams *params, struct IDiscordCore **result );

static void activity_callback( void *callback_data, enum EDiscordResult result )
{
	if ( result != DiscordResult_Ok )
	{
		game_warning_( "Encountered an error during Discord GameSDK activity update: %u\n", result );
		game_integrations_discord_shutdown_();
	}
}

bool game_integrations_discord_initialize_( int64_t clientId )
{
	// because we're loading at runtime, we'll need to fetch some things
	// this is done so that when shipping, you can exclude the necessary dll/so
	// if you don't want the integration

	PLPath exePath;
	if ( PlGetExecutableDirectory( exePath, sizeof( exePath ) ) == nullptr )
	{
		game_warning_( "Failed to fetch executable directory: %s\n", exePath );
		return false;
	}

	PLPath path;
	PlSetupPath( path, true, "%s/%s", exePath, DISCORD_DLL );

	discordLibrary = PlLoadLibrary( path, true );
	if ( discordLibrary == nullptr )
	{
		game_warning_( "Failed to load Discord GameSDK library (%s): %s\n",
		               DISCORD_DLL, PlGetError() );
		return false;
	}

	DiscordCreatePtr = PlGetLibraryProcedure( discordLibrary, "DiscordCreate" );
	if ( DiscordCreatePtr == nullptr )
	{
		game_warning_( "Failed to get \"DiscordCreate\" function in Discord GameSDK library: %s\n",
		               PlGetError() );
		return false;
	}

	struct DiscordCreateParams params;
	DiscordCreateParamsSetDefault( &params );
	params.client_id  = clientId;
	params.flags      = DiscordCreateFlags_NoRequireDiscord;
	params.event_data = &discordAppData;

	/*
	params.user_events         = &discordUserEvents;
	params.activity_events     = &discordActivityEvents;
	params.relationship_events = &discordRelationshipEvents;
	params.lobby_events        = &discordLobbyEvents;
	params.network_events      = &discordNetworkEvents;
	params.overlay_events      = &discordOverlayEvents;
	params.store_events        = &discordStoreEvents;
	params.voice_events        = &discordVoiceEvents;
	params.achievement_events  = &discordAchievementEvents;
 	*/

	enum EDiscordResult result = DiscordCreatePtr( DISCORD_VERSION, &params, &discordAppData.core );
	if ( result != DiscordResult_Ok )
	{
		game_warning_( "Encountered an error during Discord GameSDK initialization: %u\n", result );
		return false;
	}


	discordAppData.activityManager = discordAppData.core->get_activity_manager( discordAppData.core );

	discordEnabled = true;

	game_debug_( "Initialized Discord integration\n" );

	return true;
}

void game_integrations_discord_shutdown_()
{
	if ( discordLibrary != nullptr )
	{
		PlUnloadLibrary( discordLibrary );
		discordLibrary = nullptr;

		DiscordCreatePtr = nullptr;
	}

	discordEnabled = false;

	game_debug_( "Shutdown Discord integration\n" );
}

void game_integrations_discord_update_activity_( const char *details, const char *state, const char *image, const char *imageText )
{
	if ( !discordEnabled )
	{
		return;
	}

	struct DiscordActivity activity = {
	        .type       = DiscordActivityType_Playing,
	        .timestamps = {
	                       .start = 0,
	                       .end   = 0,
	                       },
	};

	snprintf( activity.details, sizeof( activity.details ), "%s", details );
	if ( state != nullptr ) snprintf( activity.state, sizeof( activity.state ), "%s", state );

	snprintf( activity.assets.large_image, sizeof( activity.assets.large_image ), "%s", image );
	if ( imageText != nullptr ) snprintf( activity.assets.large_text, sizeof( activity.assets.large_text ), "%s", imageText );

	discordAppData.activityManager->update_activity( discordAppData.activityManager, &activity, nullptr, activity_callback );
}

bool game_integrations_discord_tick_()
{
	if ( !discordEnabled )
	{
		return false;
	}

	if ( discordAppData.core->run_callbacks( discordAppData.core ) != DiscordResult_Ok )
	{
		game_warning_( "Callbacks failed for Discord GameSDK - disabling!\n" );
		game_integrations_discord_shutdown_();
		return false;
	}

	return true;
}

// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Chat dialog handling.

#include "../game_private.h"

#include "ape/ape_public_protocol.h"

static bool chatIsActive;

static const char *chatFontPath = "guis/fonts/dejavu_sans_mono_bold_24.fnt";
static ApeGuiFont *chatFont;

typedef struct ChatMessage
{
	char           buffer[ 128 ];
	GamePlayerName sender;
} ChatMessage;

static constexpr unsigned int CHAT_MAX_MESSAGES = 16;
static ChatMessage            chatMessages[ 16 ];

void game_menu_chat_initialize_()
{
	chatFont = gui_font_load( chatFontPath, gui_get_default_font( GUI_FONT_DEFAULT_MEDIUM ) );
}

void game_menu_chat_shutdown_()
{
	ape_gui_font_destroy( chatFont );
}

bool game_menu_chat_toggle_()
{
	return chatIsActive = !chatIsActive;
}

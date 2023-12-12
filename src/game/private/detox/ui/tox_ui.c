// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: For handling general UI routines.
// Author:  Mark E. Sowden

#include "tox_ui.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef enum ToxUIElementType
{
	TOX_UI_ELEMENT_TYPE_HEALTH,
	TOX_UI_ELEMENT_TYPE_STAMINA,
	TOX_UI_ELEMENT_TYPE_ITEM,
	TOX_UI_ELEMENT_TYPE_ITEM_COUNT,

	TOX_UI_MAX_ELEMENT_TYPES
} ToxUIType;

/////////////////////////////////////////////////////////////////////////////////////
// Public

void tox_ui_tick( void ) {}

// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

typedef struct GameInventorySlotDescriptor GameInventorySlotDescriptor;

typedef struct GameItemComponent
{
	const GameInventorySlotDescriptor *descriptor;
} GameItemComponent;

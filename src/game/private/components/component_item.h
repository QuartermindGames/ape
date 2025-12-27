// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef struct GameInventorySlotDescriptor GameInventorySlotDescriptor;

typedef struct GameItemComponent
{
	const GameInventorySlotDescriptor *descriptor;
} GameItemComponent;

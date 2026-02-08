// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef struct ApeRoom          ApeRoom;
typedef struct ApeLightmapPixel ApeLightmapPixel;

void ape_room_upload_lightmap_( ApeRoom *self, unsigned int width, unsigned int height );

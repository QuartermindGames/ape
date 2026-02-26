// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

typedef struct ApeRoom          ApeRoom;
typedef struct ApeLightmapPixel ApeLightmapPixel;

void         ape_room_upload_lightmaps_( ApeRoom *self );
ApeLightmap *ape_room_create_lightmap_( ApeRoom *self );
void         ape_room_destroy_lightmaps_( ApeRoom *self );

// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "aux_math.h"

typedef struct AuxTexturePackerNode AuxTexturePackerNode;

AuxTexturePackerNode *aux_texture_packer_node_create_root( unsigned int w, unsigned int h );

AuxMathRectI32 aux_texture_packer_node_get_rect( const AuxTexturePackerNode *self );

AuxTexturePackerNode *aux_texture_packer_node_insert( AuxTexturePackerNode *self, unsigned int w, unsigned int h );

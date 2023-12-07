// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: World management systems.
// Author:  Mark E. Sowden

#include "mag_game.h"
#include "mag_world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static ApeMaterial *testMaterial = NULL;

/////////////////////////////////////////////////////////////////////////////////////
// Public

void mag_world_draw( SSArlViewport *viewport )
{
	if ( testMaterial == NULL )
		testMaterial = ss_arl_material_cache( "materials/debug/debug_sprite.mat.n", APE_CACHE_EDITOR, true, false );

	static float rotate = 0.0f;
	ss_arl_draw_sprite( testMaterial,
	                    &( PLQuad ){ 0.0f, 0.0f, 128.0f, 128.0f },
	                    &( PLVector3 ){ 250.f, 250.f, 0.f },
	                    &( PLVector3 ){ -( 128.0f / 2.0f ), -( 128.0f / 2.0f ), 0.0f },
	                    &( PLVector3 ){ 0.0f, 0.0f, rotate }, 1.0f );
	rotate += 0.0005f;
}

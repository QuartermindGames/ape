// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Vector Utility
// Author:  Mark E. Sowden

#include "../ape_private.h"

#include "editor.h"

typedef struct VectorEditor
{
	ApeMaterial *backgroundMaterial;
} VectorEditor;

void ape_editor_vector_register_console_()
{
}

static bool setup_vector_instance( ApeEditorInstance *self )
{
	VectorEditor *vector       = PL_NEW( VectorEditor );
	vector->backgroundMaterial = ape_material_cache( "materials/editor/default.mat.n", APE_CACHE_GROUP_EDITOR, true, false );

	self->modeData = vector;
	return true;
}

static void cleanup_vector_instance( ApeEditorInstance *self )
{
	VectorEditor *vector = ( VectorEditor * ) self->modeData;
	assert( vector != nullptr );

	ape_material_release( vector->backgroundMaterial );

	PL_DELETE( vector );
}

static void draw_vector_overlay( ApeEditorInstance *self )
{
	VectorEditor *vector = ( VectorEditor * ) self->modeData;
	assert( vector != nullptr );

	ApeViewport *viewport = ape_viewport_get_active();
	if ( viewport == nullptr )
	{
		return;
	}

	PlMatrixMode( PL_TEXTURE_MATRIX );

	PlPushMatrix();
	PlLoadIdentityMatrix();
	PlScaleMatrix( PL_VECTOR3( 8.0f, 1.0f, 1.0f ) );

	ape_draw_textured_quad( vector->backgroundMaterial, 0.0f, 0.0f, viewport->width, viewport->height, &PL_COLOURU8( 255, 255, 255, 255 ) );

	PlPopMatrix();
}

const ApeEditorModeInterface ape_editorVectorModeInterface_ = {
        .setup       = setup_vector_instance,
        .cleanup     = cleanup_vector_instance,
        .drawOverlay = draw_vector_overlay,
};

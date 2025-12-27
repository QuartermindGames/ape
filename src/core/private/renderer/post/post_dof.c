// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Depth of Field
// Author:  Mark E. Sowden

#include "post.h"

#include "renderer/renderer_render_target.h"
#include "renderer/material/material.h"

static bool  dofEnabled;
static float dofFocusPoint;
static float dofFocusScale;
static float dofAperture;

static int dofFocusPointSlot;
static int dofFocusScaleSlot;
static int dofApertureSlot;

static ApeMaterial      *dofMaterial;
static ApeShaderProgram *dofShader;
static ApeRenderTarget  *dofTarget;

static void register_dof_console_variables()
{
	PlRegisterConsoleVariable( "post_dof", "Enable/disable depth of field.", "true", PL_VAR_BOOL, &dofEnabled, nullptr, true );

	PlRegisterConsoleVariable( "post_dof.focusPoint", "", "0.0", PL_VAR_F32, &dofFocusPoint, nullptr, false );
	PlRegisterConsoleVariable( "post_dof.focusScale", "", "0.0", PL_VAR_F32, &dofFocusScale, nullptr, false );
	PlRegisterConsoleVariable( "post_dof.aperture", "", "0", PL_VAR_F32, &dofAperture, nullptr, false );
}

static bool setup_dof_effect()
{
	if ( ( dofShader = ape_get_shader_by_name( "post_dof", APE_SHADER_DEFAULT_NULL ) ) == nullptr )
	{
		dofEnabled = false;
		return false;
	}

	if ( ( dofMaterial = ape_material_cache( "materials/engine/post/post_dof.mat.n", APE_CACHE_GROUP_GLOBAL, false ) ) == nullptr )
	{
		dofEnabled = false;
		return false;
	}

	dofFocusPointSlot = PlgGetShaderUniformSlot( dofShader->internal, "focusPoint" );
	dofFocusScaleSlot = PlgGetShaderUniformSlot( dofShader->internal, "focusScale" );
	dofApertureSlot   = PlgGetShaderUniformSlot( dofShader->internal, "aperture" );

	if ( ( dofTarget = ape_render_target_create_( "post_dof", 800, 600, PLG_BUFFER_COLOUR, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_NEAREST, 0 ) ) == nullptr )
	{
		dofEnabled = false;
		return false;
	}

	dofEnabled = true;
	return true;
}

static void cleanup_dof_effect()
{
	ape_render_target_release_( dofTarget );
	dofTarget = nullptr;
}

static void draw_dof_effect( const ApeViewport *viewport, const ApeCamera *camera )
{
	if ( !dofEnabled )
	{
		return;
	}

	ApeRenderTarget *postRenderTarget = ape_postfx_get_render_target_();
	assert( postRenderTarget != nullptr );

	PLGTexture *colourTexture = ape_render_target_get_texture_( postRenderTarget, APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR );
	PLGTexture *depthTexture  = ape_render_target_get_texture_( postRenderTarget, APE_RENDER_TARGET_ATTACHMENT_TYPE_DEPTH );
	assert( colourTexture != nullptr && depthTexture != nullptr );

	{
		int bw, bh;
		bw = viewport->width;
		bh = viewport->height;

		ape_setup_2d_viewport_( bw, bh );

		ape_render_target_bind_( dofTarget, PLG_FRAMEBUFFER_DEFAULT );
		ape_render_target_set_size_( dofTarget, bw, bh );

		ape_shader_set_active_( dofShader );

		float focusPoint = dofFocusPoint + camera->dof.focusPoint;
		PlgSetShaderUniformValueByIndex( dofShader->internal, dofFocusPointSlot, &focusPoint, false );

		float focusScale = dofFocusScale + camera->dof.focusScale;
		PlgSetShaderUniformValueByIndex( dofShader->internal, dofFocusScaleSlot, &focusScale, false );

		float aperture = dofAperture + camera->dof.aperture;
		PlgSetShaderUniformValueByIndex( dofShader->internal, dofApertureSlot, &aperture, false );

		ape_draw_textured_quad( dofMaterial, 0.0f, 0.0f, ( float ) bw, ( float ) bh, &PL_COLOUR_WHITE, 0 );

		PlgSetTexture( nullptr, 0 );
	}

	// finalize
	{
		PLGTexture *ditherTexture = ape_render_target_get_texture_( dofTarget, APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR );
		assert( ditherTexture != nullptr );

		ape_setup_2d_viewport_( viewport->width, viewport->height );
		ape_render_target_bind_( postRenderTarget, PLG_FRAMEBUFFER_DEFAULT );

		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT );

		PlgSetTexture( ditherTexture, 0 );

		ape_draw_textured_quad( nullptr, viewport->x, viewport->y, viewport->width, viewport->height, &PL_COLOUR_WHITE, 0 );

		PlgSetTexture( nullptr, 0 );
	}
}

ApePostProcessEffect ape_postDofEffect_ = {
        .registerConsoleVariables = register_dof_console_variables,
        .setup                    = setup_dof_effect,
        .cleanup                  = cleanup_dof_effect,
        .draw                     = draw_dof_effect,
};

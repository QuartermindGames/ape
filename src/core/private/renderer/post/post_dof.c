// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Depth of Field
// Author:  Mark E. Sowden

#include "post.h"

#include "renderer/material/material.h"

static bool dofEnabled;

static ApeShaderProgram *dofShader;
static ApeRenderTarget  *dofTarget;

static void register_dof_console_variables()
{
	PlRegisterConsoleVariable( "post_dof", "Enable/disable depth of field.", "true", PL_VAR_BOOL, &dofEnabled, nullptr, true );
}

static bool setup_dof_effect()
{
	if ( ( dofShader = ape_get_shader_by_name( "post_dof", APE_SHADER_DEFAULT_NULL ) ) == nullptr )
	{
		return false;
	}
	if ( ( dofTarget = ape_render_target_create( "post_dof", 800, 600, PLG_BUFFER_COLOUR, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_NEAREST, 0 ) ) == nullptr )
	{
		return false;
	}

	return true;
}

static void cleanup_dof_effect()
{
	ape_render_target_release( dofTarget );
	dofTarget = nullptr;
}

static void draw_dof_effect( const ApeViewport *viewport )
{
	if ( !dofEnabled )
	{
		return;
	}
}

ApePostProcessEffect ape_postDofEffect_ = {
        .registerConsoleVariables = register_dof_console_variables,
        .setup                    = setup_dof_effect,
        .cleanup                  = cleanup_dof_effect,
        .draw                     = draw_dof_effect,
};

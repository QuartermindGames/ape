// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Material Animator
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "material.h"

#include "renderer/renderer_texture.h"

void ape_material_animator_free_( ApeMaterialAnimator *self )
{
	switch ( self->type )
	{
		default:
			ape_console_error_( false, "Unhandled animator type for free (%u)!\n", self->type );
			break;
		case APE_MATERIAL_ANIMATOR_TYPE_TEXTURE:
		{
			ApeMaterialAnimatorTexture *texture = &self->texture;
			for ( unsigned int i = 0; i < self->numFrames; ++i )
			{
				ape_texture_release_( texture->frames[ i ] );
			}

			qm_os_memory_free( texture->frames );
			break;
		}
	}
}

static bool ape_material_animator_parse_texture_sequence_( ApeMaterialAnimatorTexture *self, ApeMaterialAnimator *animator, ApeMaterialPass *pass, AcmBranch *root )
{
	// first validate the variable we've got
	if ( animator->target.var->type != APE_MATERIAL_VAR_TEXTURE )
	{
		ape_console_warning_( "Texture sequence must target texture type!\n" );
		return false;
	}

	AcmBranch *child = acm_get_child_by_name( root, "frames" );
	if ( child == nullptr )
	{
		ape_console_warning_( "No frames specified for texture sequence!\n" );
		return false;
	}

	unsigned int numFrames = acm_get_num_of_children( child );
	if ( numFrames == 0 )
	{
		ape_console_warning_( "Empty frames list for texture sequence!\n" );
		return false;
	}

	if ( numFrames >= UINT8_MAX )
	{
		ape_console_warning_( "Too many frames (maximum of 255 supported)!\n" );
		numFrames = UINT8_MAX - 1;
	}

	child = acm_get_first_child( child );

	animator->numFrames = numFrames;
	self->frames        = QM_OS_MEMORY_NEW_( ApeTexture *, animator->numFrames );
	for ( unsigned int i = 0; i < animator->numFrames; ++i )
	{
		assert( child != nullptr );

		// this is a little lazy, but just fetch the raw value
		const char *path  = acm_branch_get_value( child, nullptr );
		self->frames[ i ] = ape_texture_cache_( path, pass->textureFilter, true );

		child = acm_get_next_child( child );
	}

	return true;
}

static bool ape_material_animator_parse_( ApeMaterialAnimator *self, ApeMaterialPass *pass, AcmBranch *root )
{
	const char *value;
	if ( ( value = acm_get_string( root, "type", nullptr ) ) != nullptr )
	{
		if ( strcmp( value, "textureSequence" ) == 0 )
		{
			self->type = APE_MATERIAL_ANIMATOR_TYPE_TEXTURE;
		}
		else
		{
			ape_console_warning_( "Encountered animator with invalid type (%s)!\n", value );
			return false;
		}
	}
	else
	{
		ape_console_warning_( "Encountered animator without a type!\n" );
		return false;
	}

	AcmBranch *typeObject = acm_get_child_by_name( root, value );
	if ( typeObject == nullptr )
	{
		ape_console_warning_( "No object for type!\n" );
		return false;
	}

	if ( ( value = acm_get_string( root, "target", nullptr ) ) != nullptr )
	{
		// $ indicates that it's a shader variable we're modifying,
		// so we'll need to look that up
		if ( *value == '$' )
		{
			const char *name = value + 1;
			if ( *name == '\0' )
			{
				ape_console_warning_( "Invalid variable target name provided!\n" );
				return false;
			}

			self->target.type = APE_MATERIAL_ANIMATOR_TARGET_TYPE_VAR;
			self->target.var  = ape_material_pass_get_variable_( pass, name );
			if ( self->target.var == nullptr )
			{
				ape_console_warning_( "Failed to find specified variable (%s)!\n", name );
				return false;
			}

			// we set the animator for the var here, to save us looking it up later
			self->target.var->animator = self;
		}
#if 0//TODO: for later...
		else
		{
			self->target.type = APE_MATERIAL_ANIMATOR_TARGET_TYPE_BUILTIN;
			if ( strcmp( value, "scroll" ) == 0 )
			{
			}
			else
			{
				ape_console_warning_( "Unknown target type (%s)!\n", value );
				return false;
			}
		}
#endif
	}
	else
	{
		ape_console_warning_( "Encountered animator without a target!\n" );
		return false;
	}

	self->state.frame     = acm_get_uint( root, "frame", 0 );
	self->state.speed     = acm_get_f32( root, "speed", 1.0f );
	self->state.isLooping = acm_get_bool( root, "isLooping", true );

	switch ( self->type )
	{
		default:
			break;
		case APE_MATERIAL_ANIMATOR_TYPE_TEXTURE:
			return ape_material_animator_parse_texture_sequence_( &self->texture, self, pass, typeObject );
	}

	ape_console_error_( false, "Unhandled animator type (%u)!\n" );
	return false;
}

void ape_material_animator_parse_array_( AcmBranch *root, ApeMaterialPass *pass )
{
	unsigned int numAnimators = acm_get_num_of_children( root );
	if ( numAnimators == 0 )
	{
		ape_console_warning_( "Empty animators list!\n" );
		return;
	}

	AcmBranch *child = acm_get_first_child( root );
	if ( child == nullptr )
	{
		ape_console_warning_( "Failed to get first animator in list!\n" );
		return;
	}

	pass->animators = QM_OS_MEMORY_NEW_( ApeMaterialAnimator, numAnimators );
	for ( unsigned int i = 0; i < numAnimators; ++i )
	{
		assert( child != nullptr );
		if ( !ape_material_animator_parse_( &pass->animators[ i ], pass, child ) )
		{
			break;
		}

		child = acm_get_next_child( child );

		pass->numAnimators++;
	}

	// check if some animators failed to load,
	// and shrink the list down if so
	if ( pass->numAnimators < numAnimators )
	{
		pass->animators = qm_os_memory_realloc( pass->animators, sizeof( ApeMaterialAnimator ) * pass->numAnimators );
		if ( pass->animators == nullptr )
		{
			ape_console_error_( true, "Failed to shrink animators list!\n" );
		}
	}
}

void ape_material_animator_tick_( ApeMaterialAnimator *self, const double delta )
{
	ApeMaterialAnimatorState *state = &self->state;

	// this'll need revisiting when we add support for vector, float and other types of animators

	state->timeAccumulator += delta;
	float timePerFrame = 1.0f / state->speed;
	while ( state->timeAccumulator >= timePerFrame )
	{
		state->timeAccumulator -= timePerFrame;

		state->frame++;
		if ( state->frame >= self->numFrames )
		{
			if ( state->isLooping )
			{
				state->frame = 0;
			}
			else
			{
				state->frame           = self->numFrames - 1;
				state->timeAccumulator = 0.0;
				return;
			}
		}
	}
}

bool ape_material_animator_is_playing_( const ApeMaterialAnimator *self )
{
	return self->state.frame < self->numFrames || self->state.isLooping;
}

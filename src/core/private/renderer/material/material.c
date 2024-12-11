// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include <plcore/pl_linkedlist.h>
#include <float.h>

#include "ape_private.h"
#include "../renderer.h"
#include "../renderer_render_target.h"
#include "material.h"
#include "world/world.h"
#include "game/game_public.h"
#include "gui/gui_private.h"

static PLLinkedList *materials[ APE_MAX_CACHE_GROUPS ];

static PLGTexture *specularFallbackTexture;
static PLGTexture *normalFallbackTexture;

typedef struct ApeMaterial
{
	char              path[ PL_SYSTEM_MAX_PATH ];
	ApeMaterialPass   passes[ SS_ARL_MAX_MATERIAL_PASSES ];
	uint              numPasses;
	PLLinkedListNode *node;

	uint width;
	uint height;

	int8_t surfaceType;

	uint flags;

	ApeMemoryReference mem;
} ApeMaterial;

static ApeMaterial *defaultMaterials[ APE_MAX_DEFAULT_MATERIALS ];

ApeMaterial *ape_material_get_default( ApeDefaultMaterial defaultMaterial )
{
	assert( defaultMaterial != APE_MAX_DEFAULT_MATERIALS );
	return defaultMaterials[ defaultMaterial ];
}

PLGTexture *ape_material_get_texture_( ApeMaterial *self, uint pass, const char *hint )
{
	if ( pass >= self->numPasses )
	{
		ape_warning_( "Invalid material pass (%u >= %u)!\n", pass, self->numPasses );
		return nullptr;
	}

	ApeMaterialPass *materialPass = &self->passes[ pass ];
	for ( uint i = 0; i < materialPass->numVariables; ++i )
	{
		if ( materialPass->variables[ i ].type != APE_MATERIAL_VAR_TEXTURE )
		{
			continue;
		}

		if ( strcmp( materialPass->variables[ i ].name, hint ) != 0 )
		{
			continue;
		}

		return ( PLGTexture * ) materialPass->variables[ i ].data.ptr;
	}

	return nullptr;
}

void ape_initialize_materials_( void )
{
	ape_print_( "Initializing material system\n" );

	for ( uint i = 0; i < APE_MAX_CACHE_GROUPS; ++i )
	{
		materials[ i ] = PlCreateLinkedList();
		if ( materials[ i ] == NULL )
		{
			ape_error_( true, "Failed to create materials list: %s\n", PlGetError() );
		}
	}

	normalFallbackTexture   = ape_texture_load_direct_( "materials/shaders/textures/normal.tga", PLG_TEXTURE_FILTER_LINEAR );
	specularFallbackTexture = ape_texture_load_direct_( "materials/shaders/textures/black.png", PLG_TEXTURE_FILTER_LINEAR );

	// cache default materials we need
	static const char *defaultMaterialPaths[ APE_MAX_DEFAULT_MATERIALS ] =
	        {
	                [APE_MATERIAL_DEFAULT_FALLBACK] = "materials/engine/fallback.mat.n",
	                [APE_MATERIAL_DEFAULT_VERTEX]   = "materials/engine/vertex.mat.n",
	                [APE_MATERIAL_DEFAULT_SHADOW]   = "materials/engine/shadow.mat.n",
	                [APE_MATERIAL_DEFAULT_HIDDEN]   = "materials/editor/hidden.mat.n",

	                [APE_MATERIAL_DEFAULT_EDITOR]           = "materials/editor/default_64.mat.n",//TODO: get rid of and make this configurable
	                [APE_MATERIAL_DEFAULT_EDITOR_SELECTION] = "materials/engine/selection.mat.n",

	                [APE_MATERIAL_DEFAULT_DEBUG_NORMALS] = "materials/debug/debug_normals.mat.n",
	        };
	for ( uint i = 0; i < APE_MAX_DEFAULT_MATERIALS; ++i )
	{
		assert( *defaultMaterialPaths[ i ] != '\0' );
		defaultMaterials[ i ] = ape_material_cache( defaultMaterialPaths[ i ], APE_CACHE_GROUP_WORLD, false );
		if ( defaultMaterials[ i ] == NULL )
		{
			ape_error_( true, "Failed to cache default material: %s\n", defaultMaterialPaths[ i ] );
		}
	}
}

void ape_shutdown_materials_( void )
{
	/* Flush any objects pending deletion in case they are holding a material handle. */
	ape_memory_flush_unreferenced_resources();

	uint totalCachedMaterials = 0;
	uint orphanedCaches       = 0;

	for ( uint i = 0; i < APE_MAX_CACHE_GROUPS; ++i )
	{
		uint cached_materials = PlGetNumLinkedListNodes( materials[ i ] );
		totalCachedMaterials += cached_materials;

		if ( cached_materials == 0 )
		{
			/* and now destroy the list */
			PlDestroyLinkedList( materials[ i ] );
			materials[ i ] = nullptr;
		}
		else
		{
			++orphanedCaches;
		}
	}

	if ( totalCachedMaterials > 0 )
	{
		ape_warning_( "Shutting down material system with %u active materials, orphaned %u caches!\n",
		              totalCachedMaterials, orphanedCaches );
	}
}

const char *ape_material_get_path( const ApeMaterial *material )
{
	return material->path;
}

uint ape_material_get_flags( const ApeMaterial *self )
{
	return self->flags;
}

/**
 * Convert the given tag into a blend mode type.
 */
static int get_blend_mode_by_tag( const char *tag )
{
	static const char *blendModeTags[] = {
	        [PLG_BLEND_NONE]                = "none",
	        [PLG_BLEND_ZERO]                = "zero",
	        [PLG_BLEND_ONE]                 = "one",
	        [PLG_BLEND_SRC_COLOR]           = "src_color",
	        [PLG_BLEND_ONE_MINUS_SRC_COLOR] = "one_minus_src_color",
	        [PLG_BLEND_SRC_ALPHA]           = "src_alpha",
	        [PLG_BLEND_ONE_MINUS_SRC_ALPHA] = "one_minus_src_alpha",
	        [PLG_BLEND_DST_ALPHA]           = "dst_alpha",
	        [PLG_BLEND_ONE_MINUS_DST_ALPHA] = "one_minus_dst_alpha",
	        [PLG_BLEND_DST_COLOR]           = "dst_color",
	        [PLG_BLEND_ONE_MINUS_DST_COLOR] = "one_minus_dst_color",
	        [PLG_BLEND_SRC_ALPHA_SATURATE]  = "src_alpha_saturate",
	};
	PL_STATIC_ASSERT( PL_ARRAY_ELEMENTS( blendModeTags ) == PLG_MAX_BLEND_MODES, "" );

	for ( int i = 0; i < PLG_MAX_BLEND_MODES; ++i )
	{
		if ( strcmp( tag, blendModeTags[ i ] ) != 0 )
		{
			continue;
		}

		return i;
	}

	ape_warning_( "Invalid blend mode specified, \"%s\", defaulting to \"none\"!\n", tag );
	return PLG_BLEND_NONE;
}

/**
 * Convert the given tag into it's built-in type.
 */
static ApeMaterialBuiltinVar get_built_in_by_tag( const char *tag )
{
	static const char *builtInTags[] = {
	        [APE_MATERIAL_BUILTIN_TIME]          = "time",
	        [APE_MATERIAL_BUILTIN_DEPTH]         = "depth",
	        [APE_MATERIAL_BUILTIN_VIEWPORT_SIZE] = "vpsize",
	        [APE_MATERIAL_BUILTIN_FALLBACK]      = "proc_fallback",

	        [APE_MATERIAL_BUILTIN_RT_SPHERE] = "rt_sphere",
	};
	PL_STATIC_ASSERT( PL_ARRAY_ELEMENTS( builtInTags ) == SS_ARL_MAX_MATERIAL_BUILTINS, "" );

	for ( int i = 0; i < SS_ARL_MAX_MATERIAL_BUILTINS; ++i )
	{
		if ( strcmp( tag, builtInTags[ i ] ) != 0 )
		{
			continue;
		}

		return i;
	}

	return APE_MATERIAL_BUILTIN_INVALID;
}

/**
 * Ensures that the material variable is set up in such a way it can
 * actually be applied for the uniform it's pointing to. Also known
 * as a shit block of code.
 */
static bool validate_material_variable( ApeMaterialVariable *variable, PLGShaderUniformType uniformType )
{
	switch ( variable->type )
	{
		default:
			break;

		case SS_ARL_MATERIAL_VAR_FLOAT:
			return ( uniformType == PLG_UNIFORM_FLOAT );
		case SS_ARL_MATERIAL_VAR_DOUBLE:
			return ( uniformType == PLG_UNIFORM_DOUBLE );

		case SS_ARL_MATERIAL_VAR_INT:
			return ( uniformType == PLG_UNIFORM_INT );
		case SS_ARL_MATERIAL_VAR_UINT:
			return ( uniformType == PLG_UNIFORM_UINT );

		case SS_ARL_MATERIAL_VAR_BOOL:
			return ( uniformType == PLG_UNIFORM_BOOL );

		case SS_ARL_MATERIAL_VAR_VEC2:
			return ( uniformType == PLG_UNIFORM_VEC2 );
		case SS_ARL_MATERIAL_VAR_VEC3:
			return ( uniformType == PLG_UNIFORM_VEC3 );
		case SS_ARL_MATERIAL_VAR_VEC4:
			return ( uniformType == PLG_UNIFORM_VEC4 );

		case SS_ARL_MATERIAL_VAR_MAT3:
			return ( uniformType == PLG_UNIFORM_MAT3 );
		case SS_ARL_MATERIAL_VAR_MAT4:
			return ( uniformType == PLG_UNIFORM_MAT4 );

			/* special types */
		case APE_MATERIAL_VAR_RENDERTARGET:
		case APE_MATERIAL_VAR_TEXTURE:
		{
			return ( ( uniformType == PLG_UNIFORM_SAMPLER1D ) ||
			         ( uniformType == PLG_UNIFORM_SAMPLER1DSHADOW ) ||
			         ( uniformType == PLG_UNIFORM_SAMPLER2D ) ||
			         ( uniformType == PLG_UNIFORM_SAMPLER2DSHADOW ) ||
			         ( uniformType == PLG_UNIFORM_SAMPLERCUBE ) );
		}
		case APE_MATERIAL_VAR_BUILTIN:
			return true;
	}

	return false;
}

/**
 * Iterate through each of the parameters provided in the 'shaderParameters'
 * block of the material.
 */
static void parse_shader_parameters( ApeMaterial *material, ApeMaterialPass *materialPass, AcmBranch *root )
{
	AcmBranch *node = acm_get_first_child( root );
	while ( node != NULL )
	{
		/* fetch the next node, so we can roll onto the next element early */
		AcmBranch *next = acm_get_next_child( node );

		ApeMaterialVariable *materialVariable = &materialPass->variables[ materialPass->numVariables ];

		/* validate that the property actually exists or is at least exposed by the shader.
		 * in the long-term we'll be doing this against our own shader program object, but
		 * for now, just do it directly against the shader itself */
		const char *propertyName      = acm_branch_get_name( node );
		materialVariable->programSlot = PlgGetShaderUniformSlot( materialPass->program->internal, propertyName );
		if ( materialVariable->programSlot == -1 )
		{
			ape_warning_( "Failed to fetch uniform slot for variable \"%s\"!\n", propertyName );
			node = next;
			continue;
		}

		materialVariable->numElements = PlgGetNumShaderUniformElements( materialPass->program->internal, materialVariable->programSlot );
		if ( materialVariable->numElements == 0 )
		{
			ape_warning_( "Failed to fetch number of uniform elements for variable (%s/%u)!\n", propertyName, materialVariable->programSlot );
			node = next;
			continue;
		}

		snprintf( materialVariable->name, sizeof( materialVariable->name ), "%s", propertyName );

		PLGShaderUniformType uniformType = PlgGetShaderUniformType( materialPass->program->internal, materialVariable->programSlot );

		/* if it's a string, it *could* be a built-in type */
		if ( acm_branch_get_type( node ) == ACM_PROPERTY_TYPE_STRING )
		{
			PLPath value;
			acm_branch_get_string( node, value, sizeof( value ) );
			if ( *value == '_' )
			{
				const char *p = ( value + 1 );
				// Render targets are "special" in the sense that we can specify what we want
				if ( strncmp( p, "rt_", 3 ) == 0 )
				{
					p += 3;
					ApeRenderTarget *renderTarget = ape_render_target_get_by_key( p );
					if ( renderTarget == NULL )
					{// Passing flag of 0 to create a placeholder
						renderTarget = ape_render_target_create( p, 64, 64, 0, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR, false );
					}

					materialVariable->type     = APE_MATERIAL_VAR_RENDERTARGET;
					materialVariable->data.ptr = renderTarget;
				}
				else
				{
					/* lookup what it actually is */
					ApeMaterialBuiltinVar materialBuiltinVar = get_built_in_by_tag( p );
					if ( materialBuiltinVar == APE_MATERIAL_BUILTIN_INVALID )
					{
						ape_warning_( "Invalid built-in variable, \"%s\", specified!\n", value );
						node = next;
						continue;
					}

					/* todo: consider validating the built-in type here, but for now, we won't bother... */
					materialVariable->type            = APE_MATERIAL_VAR_BUILTIN;
					materialVariable->data.builtinVar = materialBuiltinVar;
				}
			}
		}

		/* otherwise, we need to handle it as a traditional var */
		if ( materialVariable->type == SS_ARL_MATERIAL_VAR_INVALID )
		{
			/* now we need to convert from the node type to our internal
		 	 * material type, which is gross and sloppy and crap, but oh
		 	 * well! */
			switch ( uniformType )
			{
				default:
					break;

				case PLG_UNIFORM_BOOL:
				{
					materialVariable->data.ptr = PL_NEW_( bool, materialVariable->numElements );

					AcmErrorCode status;
					if ( materialVariable->numElements > 1 )
					{
						status = acm_branch_get_bool_array( node, materialVariable->data.ptr, materialVariable->numElements );
					}
					else
					{
						status = acm_branch_get_bool( node, materialVariable->data.ptr );
					}

					if ( status != ND_ERROR_SUCCESS )
					{
						break;
					}

					materialVariable->type = SS_ARL_MATERIAL_VAR_BOOL;
					break;
				}

				case PLG_UNIFORM_FLOAT:
				{
					materialVariable->data.ptr = PL_NEW_( float, materialVariable->numElements );

					AcmErrorCode status;
					if ( materialVariable->numElements > 1 )
					{
						status = acm_branch_get_float32_array( node, materialVariable->data.ptr, materialVariable->numElements );
					}
					else
					{
						status = acm_branch_get_float32( node, materialVariable->data.ptr );
					}

					if ( status != ND_ERROR_SUCCESS )
					{
						break;
					}

					materialVariable->type = SS_ARL_MATERIAL_VAR_FLOAT;
					break;
				}
				case PLG_UNIFORM_DOUBLE:
				{
					materialVariable->data.ptr = PL_NEW_( double, materialVariable->numElements );

					AcmErrorCode status;
					if ( materialVariable->numElements > 1 )
					{
						status = acm_branch_get_float64_array( node, materialVariable->data.ptr, materialVariable->numElements );
					}
					else
					{
						status = acm_branch_get_float64( node, materialVariable->data.ptr );
					}

					if ( status != ND_ERROR_SUCCESS )
					{
						break;
					}

					materialVariable->type = SS_ARL_MATERIAL_VAR_DOUBLE;
					break;
				}

				case PLG_UNIFORM_UINT:
				{
					materialVariable->data.ptr = PL_NEW_( uint32_t, materialVariable->numElements );

					AcmErrorCode status;
					if ( materialVariable->numElements > 1 )
					{
						status = acm_branch_get_uint32_array( node, materialVariable->data.ptr, materialVariable->numElements );
					}
					else
					{
						status = acm_branch_get_uint32( node, materialVariable->data.ptr );
					}

					if ( status != ND_ERROR_SUCCESS )
					{
						break;
					}

					materialVariable->type = SS_ARL_MATERIAL_VAR_UINT;
					break;
				}
				case PLG_UNIFORM_INT:
				{
					materialVariable->data.ptr = PL_NEW_( int32_t, materialVariable->numElements );

					AcmErrorCode status;
					if ( materialVariable->numElements > 1 )
					{
						status = acm_branch_get_int32_array( node, materialVariable->data.ptr, materialVariable->numElements );
					}
					else
					{
						status = acm_branch_get_int32( node, materialVariable->data.ptr );
					}

					if ( status != ND_ERROR_SUCCESS )
					{
						break;
					}

					materialVariable->type = SS_ARL_MATERIAL_VAR_INT;
					break;
				}

				case PLG_UNIFORM_VEC2:
				{
					materialVariable->data.ptr = PL_NEW_( PLVector2, materialVariable->numElements );
					if ( acm_branch_get_float32_array( node, materialVariable->data.ptr, 2 * materialVariable->numElements ) != ND_ERROR_SUCCESS )
					{
						break;
					}

					materialVariable->type = SS_ARL_MATERIAL_VAR_VEC2;
					break;
				}

				case PLG_UNIFORM_SAMPLER1D:
				case PLG_UNIFORM_SAMPLER2D:
				case PLG_UNIFORM_SAMPLER3D:
				case PLG_UNIFORM_SAMPLERCUBE:
				case PLG_UNIFORM_SAMPLER1DSHADOW:
				case PLG_UNIFORM_SAMPLER2DSHADOW:
				{
					PLPath texturePath;
					if ( acm_branch_get_string( node, texturePath, sizeof( PLPath ) ) != ND_ERROR_SUCCESS )
					{
						break;
					}

					if ( pl_strcasecmp( materialVariable->name, "diffuseMap" ) == 0 )
					{
						materialVariable->hint = SS_ARL_MATERIAL_VAR_HINT_DIFFUSE;
					}
					else if ( pl_strcasecmp( materialVariable->name, "normalMap" ) == 0 )
					{
						materialVariable->hint = SS_ARL_MATERIAL_VAR_HINT_NORMAL;
					}
					else if ( pl_strcasecmp( materialVariable->name, "specularMap" ) == 0 )
					{
						materialVariable->hint = SS_ARL_MATERIAL_VAR_HINT_SPECULAR;
					}

					PLGTexture *texture = ape_texture_load_direct_( texturePath, materialPass->textureFilter );
					/* this sucks, but shaders only deal with a default "pass" rather than a whole material, so
					 * it needs to be able to pass a null material... probably revisit this later. */
					if ( material != nullptr )
					{
						if ( material->width < texture->w )
						{
							material->width = texture->w;
						}
						if ( material->height < texture->h )
						{
							material->height = texture->h;
						}
					}

					materialVariable->data.ptr = texture;
					materialVariable->type     = APE_MATERIAL_VAR_TEXTURE;
					break;
				}
			}

			if ( materialVariable->type == SS_ARL_MATERIAL_VAR_INVALID )
			{
				ape_warning_( "Invalid property type for shader variable \"%s\"!\n", propertyName );
				node = next;
				continue;
			}
		}

		if ( !validate_material_variable( materialVariable, uniformType ) )
		{
			ape_warning_( "Mismatch between material variable type and uniform type!\n" );
			node = next;
			continue;
		}

		materialPass->numVariables++;

		node = next;
	}
}

void ape_parse_material_pass_( ApeMaterial *material, struct AcmBranch *root, ApeMaterialPass *materialPass )
{
	/* fetch the blend mode we'll use for the pass */
	AcmBranch *subNode;
	if ( ( subNode = acm_get_child_by_name( root, "blendMode" ) ) != NULL )
	{
		char *blendModesArray[ 2 ];
		if ( acm_branch_get_string_array( subNode, blendModesArray, 2 ) == ND_ERROR_SUCCESS )
		{
			materialPass->blendMode[ 0 ] = get_blend_mode_by_tag( blendModesArray[ 0 ] );
			PL_DELETE( blendModesArray[ 0 ] );
			materialPass->blendMode[ 1 ] = get_blend_mode_by_tag( blendModesArray[ 1 ] );
			PL_DELETE( blendModesArray[ 1 ] );
		}
		else
		{
			ape_warning_( "Invalid blend mode array in material!\n" );
		}
	}
	else
	{
		materialPass->blendMode[ 0 ] = PLG_BLEND_NONE;
		materialPass->blendMode[ 1 ] = PLG_BLEND_NONE;
	}

	materialPass->depthTest = acm_get_bool( root, "depthTest", materialPass->depthTest );
	materialPass->cullMode  = ACM_GET_UINT( materialPass->cullMode, root, "cullMode", materialPass->cullMode );

	const char *textureFilterPtr = acm_get_string( root, "textureFilterMode", nullptr );
	if ( textureFilterPtr != NULL )
	{
		if ( pl_strcasecmp( textureFilterPtr, "mipmap_nearest" ) == 0 )
		{
			materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_NEAREST;
		}
		else if ( pl_strcasecmp( textureFilterPtr, "mipmap_linear" ) == 0 )
		{
			materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
		}
		else if ( pl_strcasecmp( textureFilterPtr, "mipmap_linear_nearest" ) == 0 )
		{
			materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR_NEAREST;
		}
		else if ( pl_strcasecmp( textureFilterPtr, "mipmap_nearest_linear" ) == 0 )
		{
			materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_NEAREST_LINEAR;
		}
		else if ( pl_strcasecmp( textureFilterPtr, "nearest" ) == 0 )
		{
			materialPass->textureFilter = PLG_TEXTURE_FILTER_NEAREST;
		}
		else if ( pl_strcasecmp( textureFilterPtr, "linear" ) == 0 )
		{
			materialPass->textureFilter = PLG_TEXTURE_FILTER_LINEAR;
		}
	}

	/* now handle any specific parameters the material provides */
	materialPass->textureScroll = acm_get_vector2( root, "textureScroll", &PL_VECTOR2( 0.0f, 0.0f ) );
	materialPass->textureOffset = acm_get_vector2( root, "textureOffset", &PL_VECTOR2( 0.0f, 0.0f ) );
	materialPass->textureScale  = acm_get_vector2( root, "textureScale", &PL_VECTOR2( 1.0f, 1.0f ) );
	if ( materialPass->textureScale.x == 0.0f || materialPass->textureScale.y == 0.0f )
	{
		ape_warning_( "Encountered material pass with invalid texture scale (%s)!\n", PlPrintVector2( &materialPass->textureScale, PL_VAR_F32 ) );
	}

	if ( ( subNode = acm_get_child_by_name( root, "shaderParameters" ) ) != NULL )
	{
		/* there's some extra complexity when parsing in parameters, so we'll defer that
		 * to another function */
		parse_shader_parameters( material, materialPass, subNode );
	}

	/* not sure whether the above section should be required yet, there might be
	 * a case where we only want to use the shader defaults? */
}

PLImage *ape_material_load_preview( const char *path )
{
	AcmBranch *root = acm_load_file( path, "material" );
	if ( root == nullptr )
	{
		ape_warning_( "Failed to load material (%s) for preview!\n", path );
		return nullptr;
	}

	PLImage    *preview;
	const char *previewPath = acm_get_string( root, "previewTexture", nullptr );
	if ( previewPath != NULL )
	{
		preview = PlLoadImage( previewPath );
	}
	else
	{
		// the painful way...
		AcmBranch *diffuseNode = acm_linear_lookup( root, "diffuseMap" );
		if ( diffuseNode == nullptr )
		{
			ape_warning_( "Failed to find preview texture to use under material (%s)!\n", path );
			return nullptr;
		}

		PLPath buf;
		if ( acm_branch_get_string( diffuseNode, buf, sizeof( buf ) ) != ND_ERROR_SUCCESS )
		{
			ape_warning_( "Diffuse texture under material (%s) was not a valid string!\n", path );
			return nullptr;
		}

		preview = PlLoadImage( buf );
	}

	if ( preview == nullptr )
	{
		ape_warning_( "Failed to load preview image for material (%s): %s\n", path, PlGetError() );
		return nullptr;
	}

	PLImage *newPreview = PlResizeImage( preview, 128, 128 );
	if ( newPreview != nullptr )
	{
		PlDestroyImage( preview );
		preview = newPreview;
	}
	else
	{
		ape_warning_( "Failed to resize preview for material (%s): %s\n", path, PlGetError() );
	}

	return preview;
}

static ApeMaterial *parse_material( ApeMaterial *material, AcmBranch *root )
{
	/* each pass specifies how the object should be drawn before
	 * drawing it again and again for each child */
	AcmBranch *node;
	if ( ( node = acm_get_child_by_name( root, "passes" ) ) != NULL )
	{
		node = acm_get_first_child( node );
		while ( node != NULL )
		{
			ApeMaterialPass *currentPass = &material->passes[ material->numPasses++ ];
			/* current pass should've already been cleared by prior memset,
			 * so no need to reset the state for some crap */

			/* fetch the shader program we need to use for this pass */
			const char       *programName  = acm_get_string( node, "shaderProgram", "default" );
			ApeShaderProgram *programIndex = ape_get_shader_by_name( programName, APE_SHADER_DEFAULT );
			*currentPass                   = programIndex->defaultPass;
			currentPass->program           = programIndex;

			ape_parse_material_pass_( material, node, currentPass );
			if ( currentPass->blendMode[ 0 ] != PLG_BLEND_NONE || currentPass->blendMode[ 1 ] != PLG_BLEND_NONE )
			{
				material->flags |= APE_MATERIAL_FLAG_BLENDED;
			}

			node = acm_get_next_child( node );
		}
	}

	material->surfaceType = ACM_GET_UINT( material->surfaceType, root, "surfaceType", 0 );
	if ( acm_get_bool( root, "enableShadows", true ) )
	{
		material->flags |= APE_MATERIAL_FLAG_CAST_SHADOWS | APE_MATERIAL_FLAG_RECEIVE_SHADOWS;
	}
	if ( !acm_get_bool( root, "receiveShadows", ( material->flags & APE_MATERIAL_FLAG_RECEIVE_SHADOWS ) ) )
	{
		material->flags &= ~APE_MATERIAL_FLAG_RECEIVE_SHADOWS;
	}
	if ( !acm_get_bool( root, "castShadows", ( material->flags & APE_MATERIAL_FLAG_CAST_SHADOWS ) ) )
	{
		material->flags &= ~APE_MATERIAL_FLAG_CAST_SHADOWS;
	}
	if ( acm_get_bool( root, "mirror", false ) )
	{
		material->flags |= APE_MATERIAL_FLAG_MIRROR;
	}

	if ( material->numPasses == 0 )
	{
		ape_warning_( "No passes specified for material!\n" );
	}

	return material;
}

static ApeMaterial *get_material( const char *path, ApeCacheGroup group )
{
	PLLinkedListNode *node = PlGetFirstNode( materials[ group ] );
	while ( node != NULL )
	{
		ApeMaterial *material = PlGetLinkedListNodeUserData( node );
		if ( strcmp( material->path, path ) == 0 )
		{
			return material;
		}

		node = PlGetNextLinkedListNode( node );
	}

	return nullptr;
}

static void destroy_material( ApeMaterial *material )
{
	if ( material == NULL )
	{
		return;
	}

	for ( uint i = 0; i < material->numPasses; ++i )
	{
		for ( uint j = 0; j < material->passes[ i ].numVariables; ++j )
		{
			switch ( material->passes[ i ].variables[ j ].type )
			{
				case APE_MATERIAL_VAR_BUILTIN:
				case APE_MATERIAL_VAR_TEXTURE:
					//TODO: right now this is all using the plgtexture crap directly, so... waaaahh!!!
					break;
				case APE_MATERIAL_VAR_RENDERTARGET:
					ape_render_target_release( ( ApeRenderTarget * ) material->passes[ i ].variables[ j ].data.ptr );
					break;
				default:
					PL_DELETE( material->passes[ i ].variables[ j ].data.ptr );
					break;
			}
		}
	}

	PLLinkedList *container = PlGetLinkedListNodeContainer( material->node );
	if ( container != NULL )
	{
		PlDestroyLinkedListNode( material->node );
	}

	PL_DELETE( material );
}

static void destroy_material_callback( void *userData )
{
	destroy_material( ( ApeMaterial * ) userData );
}

/**
 * This will attempt to draw from the perspective of the sphere.
 * This is also really *really* expensive right now, so uh, use sparingly.
 *
 * @param material 	Pointer to the material.
 * @param mesh		Pointer to the origin mesh.
 */
static void draw_rt_sphere( ApeMaterial *material, PLGMesh *mesh )
{
#pragma message "TODO: draw_rt_sphere"

	// make sure we don't draw this again during pass
	static ApeMaterial *reject;
	if ( reject == material )
	{
		return;
	}

	ApeCamera *camera = ape_renderer_get_current_camera_();
	if ( camera == nullptr )
	{
		// no camera bound...
		return;
	}

	reject = material;

	PLCollisionAABB bounds = PlgGenerateAabbFromMesh( mesh, true );
	PLVector3       origin = bounds.absOrigin;
	PLVector3       dir    = PlSubtractVector3( APE_SG_NODE_GET_POSITION( camera ), origin );

	// save some of the camera state, so we can do dumb shit

	ape_camera_build_pvs_( camera );

	reject = nullptr;
}

static void set_built_in_variable( ApeMaterial *material, ApeMaterialPass *pass, PLGMesh *mesh, int uniformSlot, int variable, uint *curUnit )
{
	if ( variable == -1 )
	{
		return;
	}

	PLGShaderProgram *program = pass->program->internal;
	switch ( variable )
	{
		case APE_MATERIAL_BUILTIN_TIME:
		{
			uint numTicks = ape_get_num_ticks();
			PlgSetShaderUniformValueByIndex( program, uniformSlot, &numTicks, false );
			break;
		}

		case APE_MATERIAL_BUILTIN_FALLBACK:
		case APE_MATERIAL_BUILTIN_DEPTH:
		{
			PLGTexture *texture = NULL;
			if ( variable == APE_MATERIAL_BUILTIN_FALLBACK )
			{
				texture = ape_texture_get_fallback();
			}
#if 0//TODO
			else if ( variable == APE_MATERIAL_BUILTIN_DEPTH )
				texture = apeGetPrimaryDepthAttachment();
#endif

			if ( texture == NULL )
			{
				break;
			}

			PlgSetTexture( texture, *curUnit );
			PlgSetShaderUniformValueByIndex( program, uniformSlot, curUnit, false );
			( *curUnit )++;
			break;
		}

		case APE_MATERIAL_BUILTIN_VIEWPORT_SIZE:
		{
			int w, h;
			ape_get_2d_viewport_size_( &w, &h );
			PlgSetShaderUniformValueByIndex( program, uniformSlot, &PL_VECTOR2( ( float ) w, ( float ) h ), false );
			break;
		}

		case APE_MATERIAL_BUILTIN_RT_SPHERE:
		{
			draw_rt_sphere( material, mesh );
			break;
		}

		default:
			break;
	}
}

static void set_global_uniforms( ApeShaderProgram *program, const ApeMaterialPass *pass, const ApeLight *light )
{
	ApeWorld *world = ss_game_get_current_world();

	if ( program->globalUniforms[ APE_SHADER_UNIFORM_FOG_COLOUR ] >= 0 )
	{
		PLColourF32 fogColour = ( light == NULL && world != NULL ) ? world->fogColour : ( PLColourF32 ) { 0.0f, 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_FOG_COLOUR ], &fogColour, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_FOG_NEAR ] >= 0 )
	{
		float fogNear = PlClamp( 0.0f, ( ( ape_config_.renderer.fogNearOverride > -1.f ) || world == NULL ) ? ape_config_.renderer.fogNearOverride : world->fogNear, FLT_MAX );
		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_FOG_NEAR ], &fogNear, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_FOG_FAR ] >= 0 )
	{
		float fogFar = ( ( ape_config_.renderer.fogFarOverride > -1.f ) || world == NULL ) ? ape_config_.renderer.fogFarOverride : world->fogFar;
		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_FOG_FAR ], &fogFar, false );
	}

	if ( program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_COLOUR ] >= 0 )
	{
		PLColourF32 lightColour = ( light != NULL ) ? light->colour : ( PLColourF32 ) { 0.0f, 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_COLOUR ], &lightColour, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_POSITION ] >= 0 )
	{
		PLVector3 lightPosition = ( light != NULL ) ? light->base.position : ( PLVector3 ) { 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_POSITION ], &lightPosition, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_RADIUS ] >= 0 )
	{
		float lightRadius = ( light != NULL ) ? light->radius : 0.0f;
		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_RADIUS ], &lightRadius, false );
	}

	if ( program->globalUniforms[ APE_SHADER_UNIFORM_SUN_COLOUR ] >= 0 )
	{
		PLColourF32 sunColour = ( light != NULL && light->type == APE_LIGHT_TYPE_SUN ) ? light->colour : ( PLColourF32 ) { 0.0f, 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_SUN_COLOUR ], &sunColour, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_SUN_POSITION ] >= 0 )
	{
		PLVector3 lightPosition = ( light != NULL && light->type == APE_LIGHT_TYPE_SUN ) ? light->base.position : ( PLVector3 ) { 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_SUN_POSITION ], &lightPosition, false );
	}

	if ( program->globalUniforms[ APE_SHADER_UNIFORM_AMBIENCE ] >= 0 )
	{
		PLColourF32 sunAmbience;
		if ( ape_rendererState_.camera != NULL && ( ape_rendererState_.camera->drawMode == APE_CAMERA_DRAW_MODE_TEXTURED ) )
		{
			sunAmbience = ( PLColourF32 ) { 1.0f, 1.0f, 1.0f, 1.0f };
		}
		else
		{
			sunAmbience = ape_rendererState_.ambience;
		}

		PlgSetShaderUniformValueByIndex( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_AMBIENCE ], &sunAmbience, false );
	}

	if ( program->globalUniforms[ APE_SHADER_UNIFORM_TEXTURE_MATRIX ] >= 0 )
	{
		PLMatrixMode oldMode = PlGetMatrixMode();
		PlMatrixMode( PL_TEXTURE_MATRIX );

		PlPushMatrix();

		PlTranslateMatrix( PL_VECTOR3( pass->textureOffset.x, pass->textureOffset.y, 0.0f ) );

		float scaleX = ( pass->textureScale.x != 0.0f ) ? 1.0f / pass->textureScale.x : 1.0f;
		float scaleY = ( pass->textureScale.y != 0.0f ) ? 1.0f / pass->textureScale.y : 1.0f;
		PlScaleMatrix( PL_VECTOR3( scaleX, scaleY, 1.0f ) );

		PlgSetShaderUniformValueByIndex( program->internal,
		                                 program->globalUniforms[ APE_SHADER_UNIFORM_TEXTURE_MATRIX ],
		                                 PlGetMatrix( PL_TEXTURE_MATRIX ), false );

		PlPopMatrix();

		PlMatrixMode( oldMode );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_MODEL_MATRIX ] >= 0 )
	{
		PlgSetShaderUniformValueByIndex( program->internal,
		                                 program->globalUniforms[ APE_SHADER_UNIFORM_MODEL_MATRIX ],
		                                 PlGetMatrix( PL_MODELVIEW_MATRIX ), false );
	}
}

ApeMaterial *ape_material_cache( const char *path, ApeCacheGroup group, bool useFallback )
{
	/* check if it's already cached */
	ApeMaterial *material = get_material( path, group );
	if ( material != NULL )
	{
		ape_memory_add_reference( &material->mem );
		return material;
	}

	/* fallback should be optional, as in some cases we might actually care */
	ApeMaterial *fallbackPtr = useFallback ? defaultMaterials[ APE_MATERIAL_DEFAULT_FALLBACK ] : nullptr;

	AcmBranch *root = acm_load_file( path, "material" );
	if ( root == NULL )
	{
		ape_warning_( "Failed to load material, \"%s\" (%s)!\n", path, acm_get_error_message() );
		return fallbackPtr;
	}

	material = PL_NEW( ApeMaterial );
	snprintf( material->path, sizeof( material->path ), "%s", path );

	parse_material( material, root );

	acm_branch_destroy( root );

	material->node = PlInsertLinkedListNode( materials[ group ], material );

	ape_memory_setup_reference( material->path, APE_CACHE_POOL_MATERIALS, &material->mem, destroy_material_callback, material );
	ape_memory_add_reference( &material->mem );

	return material;
}

void ape_material_release( ApeMaterial *material )
{
	if ( material == NULL )
	{
		return;
	}

	// don't flush default materials...
	for ( uint i = 0; i < APE_MAX_DEFAULT_MATERIALS; ++i )
	{
		if ( material != defaultMaterials[ i ] )
		{
			continue;
		}

		return;
	}

	ape_memory_release( &material->mem );
}

int8_t ape_material_get_surface_type( const ApeMaterial *material )
{
	return material->surfaceType;
}

bool ape_material_shadows_enabled( const ApeMaterial *self )
{
	return ( self->flags & APE_MATERIAL_FLAG_CAST_SHADOWS );
}

bool ape_material_is_blended( const ApeMaterial *self )
{
	return ( self->flags & APE_MATERIAL_FLAG_BLENDED );
}

void ape_material_draw( ApeMaterial *material, PLGMesh *mesh, ApeLight **lights )
{
	if ( ape_rendererState_.camera != NULL )
	{
		if ( ( ape_rendererState_.camera->drawMode == APE_CAMERA_DRAW_MODE_TEXTURED ) || ( ape_rendererState_.camera->drawMode == APE_CAMERA_DRAW_MODE_SOLID ) )
		{
			lights = nullptr;
		}
		if ( ape_rendererState_.camera->drawMode == APE_CAMERA_DRAW_MODE_SOLID )
		{
			material = defaultMaterials[ APE_MATERIAL_DEFAULT_VERTEX ];
		}
	}

	if ( ape_rendererState_.overrideBlendMode )
	{
		PlgEnableGraphicsState( PLG_GFX_STATE_BLEND );
		PlgSetBlendMode( ape_rendererState_.blendModeA, ape_rendererState_.blendModeB );
	}

	for ( uint i = 0; i < material->numPasses; ++i )
	{
		ApeMaterialPass *curPass = &material->passes[ i ];

		// Mirror mode requires flipping the matrix,
		// so we'll need to update the cull mode
		PLGCullMode cullMode;
		if ( ape_rendererState_.cullMode == APE_RENDERER_CULL_MODE_FRONT )
		{
			cullMode = PLG_CULL_POSITIVE;
		}
		else if ( ape_rendererState_.cullMode == APE_RENDERER_CULL_MODE_BACK )
		{
			cullMode = PLG_CULL_NEGATIVE;
		}
		else if ( ape_rendererState_.cullMode == APE_RENDERER_CULL_MODE_NONE )
		{
			cullMode = PLG_CULL_NONE;
		}
		else
		{
			cullMode = curPass->cullMode;
		}

		if ( ape_rendererState_.mirror && ( ape_rendererState_.depth % 2 ) )
		{
			if ( cullMode == PLG_CULL_NEGATIVE )
			{
				cullMode = PLG_CULL_POSITIVE;
			}
			else if ( cullMode == PLG_CULL_POSITIVE )
			{
				cullMode = PLG_CULL_NEGATIVE;
			}
		}

		PlgSetCullMode( cullMode );

		//TODO: breaks crap if called... :(
		//		need to keep track of global requested state, and override
		//PlgDepthMask( curPass->depthTest );

		// we have an awkward check for wireframe here because we don't want to just blindly handle it globally,
		// otherwise UI elements will be wireframe too, so instead we'll just check the plg state flag
		if ( PlgIsGraphicsStateEnabled( PLG_GFX_STATE_WIREFRAME ) )
		{
			ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT_VERTEX );
			ape_shader_set_active_( program );

			set_global_uniforms( curPass->program, curPass, nullptr );

			PlgSetTexture( nullptr, 0 );
		}
		else
		{
			PlgSetShaderProgram( curPass->program->internal );

			if ( !ape_rendererState_.overrideBlendMode )
			{
				PlgSetBlendMode( curPass->blendMode[ 0 ], curPass->blendMode[ 1 ] );
			}

			set_global_uniforms( curPass->program, curPass, lights != nullptr ? lights[ 0 ] : nullptr );

			uint curUnit = 0;
			for ( uint j = 0; j < curPass->numVariables; ++j )
			{
				if ( curPass->variables[ j ].type == APE_MATERIAL_VAR_BUILTIN )
				{
					set_built_in_variable( material, curPass, mesh, curPass->variables[ j ].programSlot, curPass->variables[ j ].data.builtinVar, &curUnit );
					continue;
				}
				// textures just need to be set per their respective unit
				else if ( curPass->variables[ j ].type == APE_MATERIAL_VAR_TEXTURE || curPass->variables[ j ].type == APE_MATERIAL_VAR_RENDERTARGET )
				{
					PL_GET_CVAR( "r/skipDiffuse", skipDiffuse );
					if ( skipDiffuse != nullptr && ( curPass->variables[ j ].hint == SS_ARL_MATERIAL_VAR_HINT_DIFFUSE && skipDiffuse->b_value ) )
					{
						continue;
					}

					PLGTexture *texture;
					if ( curPass->variables[ j ].type == APE_MATERIAL_VAR_RENDERTARGET )
					{
						texture = ape_render_target_get_texture( ( ApeRenderTarget * ) curPass->variables[ j ].data.ptr );
						if ( texture == NULL )
						{
							texture = ape_texture_get_fallback();
						}
					}
					else
					{
						texture = ( PLGTexture * ) curPass->variables[ j ].data.ptr;
					}

					assert( texture != NULL );

					PL_GET_CVAR( "r/skipNormal", skipNormal );
					if ( skipNormal != NULL && ( curPass->variables[ j ].hint == SS_ARL_MATERIAL_VAR_HINT_NORMAL && skipNormal->b_value ) )
					{
						texture = normalFallbackTexture;
					}
					PL_GET_CVAR( "r/skipSpecular", skipSpecular );
					if ( skipSpecular != NULL && ( curPass->variables[ j ].hint == SS_ARL_MATERIAL_VAR_HINT_SPECULAR && skipSpecular->b_value ) )
					{
						texture = specularFallbackTexture;
					}

					PLGTextureFilter textureFilter = curPass->textureFilter;
					if ( texture->flags & PLG_TEXTURE_FLAG_NOMIPS )
					{
						if ( textureFilter == PLG_TEXTURE_FILTER_MIPMAP_LINEAR )
						{
							textureFilter = PLG_TEXTURE_FILTER_LINEAR;
						}
						else
						{
							textureFilter = PLG_TEXTURE_FILTER_NEAREST;
						}
					}

					PlgSetTexture( texture, curUnit );
					PlgSetTextureFilter( texture, textureFilter );

					PlgSetShaderUniformValueByIndex( curPass->program->internal, curPass->variables[ j ].programSlot, &curUnit, false );
					curUnit++;
					continue;
				}

				PlgSetShaderUniformValueByIndex( curPass->program->internal, curPass->variables[ j ].programSlot, ( intmax_t * ) curPass->variables[ j ].data.ptr, false );
			}
		}

		PlgUploadMesh( mesh );
		PlgDrawMesh( mesh );

		ape_rendererPerformance_.numBatches++;
		if ( mesh->primitive == PLG_MESH_TRIANGLES )
		{
			ape_rendererPerformance_.numTriangles += mesh->num_triangles;
		}
		else
		{
			ape_rendererPerformance_.numTriangles += ( mesh->num_verts / 2 );
		}
	}

	PlgSetCullMode( PLG_CULL_POSITIVE );
	PlgSetBlendMode( PLG_BLEND_DISABLE );
}

void ape_tick_materials_( void )
{
	ape_hot_reload_shaders_();

	for ( uint i = 0; i < APE_MAX_CACHE_GROUPS; ++i )
	{
		ApeMaterial *material;
		COM_ITERATE_LINKED_LIST( material, materials[ i ], itr )
		{
			for ( uint j = 0; j < material->numPasses; ++j )
			{
				if ( fabsf( material->passes[ j ].textureScroll.x ) < PL_EPSILON &&
				     fabsf( material->passes[ j ].textureScroll.y ) < PL_EPSILON )
				{
					continue;
				}

				// before it was operating by the current pass texture
				// honestly this might not really be the best approach,
				// with the former being better, will see...
				float w = ( float ) material->width;
				float h = ( float ) material->height;

				PLVector2 scroll;
				scroll = PlDivideVector2( material->passes[ j ].textureScroll, PL_VECTOR2( w, h ) );
				scroll = PlScaleVector2( &scroll, &PL_VECTOR2( material->passes[ j ].textureScale.x, material->passes[ j ].textureScale.y ) );

				material->passes[ j ].textureOffset = PlAddVector2( material->passes[ j ].textureOffset, scroll );
			}
		}
	}
}

uint ape_material_get_width( const ApeMaterial *self )
{
	return self->width;
}

uint ape_material_get_height( const ApeMaterial *self )
{
	return self->height;
}

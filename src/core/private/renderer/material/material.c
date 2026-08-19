// Copyright © 2017-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Material system
// Author:  Mark E. Sowden

#include <plcore/pl_linkedlist.h>

#include "qmos/public/qm_os_string.h"

#include "ape_private.h"

#include "renderer/renderer_render_target.h"
#include "renderer/renderer_texture.h"

#include "material.h"

static PLLinkedList *materials[ APE_MAX_CACHE_GROUPS ];

static ApeTexture *normalFallbackTexture;

static ApeConsoleVarString materialTextureFilter;
static int32_t             materialTextureAnisotropy = 16;
static bool                materialSkipDiffuse;
static bool                materialSkipNormal;
static bool                materialSkipSpecular;
static bool                materialSkipLightmap;

typedef struct ApeMaterial
{
	char              path[ PL_SYSTEM_MAX_PATH ];
	ApeMaterialPass   passes[ SS_ARL_MAX_MATERIAL_PASSES ];
	uint8_t           numPasses;
	PLLinkedListNode *node;

	uint16_t width;
	uint16_t height;

	QmMathColour4f lightEmission;

	int8_t surfaceType;

	unsigned int flags;

	ApeMemoryReference mem;
} ApeMaterial;

static ApeMaterial *defaultMaterials[ APE_MAX_DEFAULT_MATERIALS ];

static void material_var_free( ApeMaterialVariable *var );

ApeMaterial *ape_material_get_default( ApeDefaultMaterial defaultMaterial )
{
	assert( defaultMaterial != APE_MAX_DEFAULT_MATERIALS );
	return defaultMaterials[ defaultMaterial ];
}

ApeTexture *ape_material_get_texture_( ApeMaterial *self, unsigned int pass, const char *hint )
{
	if ( pass >= self->numPasses )
	{
		ape_console_warning_( "Invalid material pass (%u >= %u)!\n", pass, self->numPasses );
		return nullptr;
	}

	ApeMaterialPass *materialPass = &self->passes[ pass ];
	for ( unsigned int i = 0; i < materialPass->numVariables; ++i )
	{
		if ( materialPass->variables[ i ].type != APE_MATERIAL_VAR_TEXTURE )
		{
			continue;
		}

		if ( strcmp( materialPass->variables[ i ].name, hint ) != 0 )
		{
			continue;
		}

		return materialPass->variables[ i ].data.ptr;
	}

	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_material_register_console_variables_()
{
	ape_console_var_register( "material.forceTextureFilter", "Force a specific texture filtering mode.", "", PL_VAR_STRING, materialTextureFilter, nullptr, 0 );
	ape_console_var_register( "material.textureAnistropy", "", "16", PL_VAR_I32, &materialTextureAnisotropy, nullptr, 0 );

	ape_console_var_register( "material.skipDiffuse", "Skip diffuse map.", "0", PL_VAR_BOOL, &materialSkipDiffuse, nullptr, 0 );
	ape_console_var_register( "material.skipNormal", "Skip normal map.", "0", PL_VAR_BOOL, &materialSkipNormal, nullptr, 0 );
	ape_console_var_register( "material.skipSpecular", "Skip specular map.", "0", PL_VAR_BOOL, &materialSkipSpecular, nullptr, 0 );
	ape_console_var_register( "material.skipLightmap", "Skip lightmap.", "0", PL_VAR_BOOL, &materialSkipLightmap, nullptr, 0 );
}

void ape_initialize_materials_()
{
	ape_console_print_( "Initializing material system\n" );

	for ( unsigned int i = 0; i < APE_MAX_CACHE_GROUPS; ++i )
	{
		materials[ i ] = PlCreateLinkedList();
		if ( materials[ i ] == NULL )
		{
			ape_console_error_( true, "Failed to create materials list: %s\n", PlGetError() );
		}
	}

	normalFallbackTexture = ape_texture_cache_( "materials/shaders/textures/normal.tga", PLG_TEXTURE_FILTER_LINEAR, true );

	// cache default materials we need
	static const char *defaultMaterialPaths[ APE_MAX_DEFAULT_MATERIALS ] =
	        {
	                [APE_MATERIAL_DEFAULT_FALLBACK]     = "materials/engine/fallback.mat.n",
	                [APE_MATERIAL_DEFAULT_VERTEX]       = "materials/engine/vertex.mat.n",
	                [APE_MATERIAL_DEFAULT_VERTEX_ALPHA] = "materials/engine/vertex_alpha.mat.n",
	                [APE_MATERIAL_DEFAULT_SHADOW]       = "materials/engine/shadow.mat.n",
	                [APE_MATERIAL_DEFAULT_HIDDEN]       = "materials/editor/hidden.mat.n",

	                [APE_MATERIAL_DEFAULT_EDITOR]           = "materials/world/dev/dev_tile_generic_00.mat.n",//TODO: get rid of and make this configurable
	                [APE_MATERIAL_DEFAULT_EDITOR_SELECTION] = "materials/engine/selection.mat.n",

	                [APE_MATERIAL_DEFAULT_DEBUG_NORMALS] = "materials/debug/debug_normals.mat.n",
	        };
	for ( unsigned int i = 0; i < APE_MAX_DEFAULT_MATERIALS; ++i )
	{
		assert( *defaultMaterialPaths[ i ] != '\0' );
		defaultMaterials[ i ] = ape_material_cache( defaultMaterialPaths[ i ], APE_CACHE_GROUP_WORLD, false );
		if ( defaultMaterials[ i ] == NULL )
		{
			ape_console_error_( true, "Failed to cache default material: %s\n", defaultMaterialPaths[ i ] );
		}
	}
}

void ape_shutdown_materials_()
{
	// release defaults
	for ( unsigned int i = 0; i < APE_MAX_DEFAULT_MATERIALS; ++i )
	{
		ape_material_release_reference( defaultMaterials[ i ] );
		defaultMaterials[ i ] = nullptr;
	}

	/* Flush any objects pending deletion in case they are holding a material handle. */
	ape_memory_flush_unreferenced_resources();

	unsigned int totalCachedMaterials = 0;
	unsigned int orphanedCaches       = 0;

	for ( unsigned int i = 0; i < APE_MAX_CACHE_GROUPS; ++i )
	{
		unsigned int cached_materials = PlGetNumLinkedListNodes( materials[ i ] );
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
		ape_console_warning_( "Shutting down material system with %u active materials, orphaned %u caches!\n",
		                      totalCachedMaterials, orphanedCaches );
	}
}

const char *ape_material_get_path( const ApeMaterial *material )
{
	return material->path;
}

ApeMaterialPass *ape_material_get_pass( ApeMaterial *self, const unsigned int pass )
{
	if ( pass >= self->numPasses )
	{
		ape_console_warning_( "Invalid pass index specified (%u >= %u)!\n", pass, self->numPasses );
		return nullptr;
	}

	return &self->passes[ pass ];
}

ApeShaderProgram *ape_material_pass_get_shader_program( const ApeMaterialPass *self )
{
	return self->program;
}

ApeMaterialVariable *ape_material_pass_get_variable_( ApeMaterialPass *self, const char *name )
{
	// for now we just have a linear lookup here, which will *probably* be okay...
	for ( unsigned int i = 0; i < self->numVariables; ++i )
	{
		if ( strcmp( self->variables[ i ].name, name ) == 0 )
		{
			return &self->variables[ i ];
		}
	}

	return nullptr;
}

/**
 * Convert the given tag into a compare mode type.
 */
static PLGCompareFunction get_compare_mode_by_tag( const char *tag )
{
	static const char *compareModeTags[] = {
	        [PLG_COMPARE_NEVER]    = "never",
	        [PLG_COMPARE_LESS]     = "less",
	        [PLG_COMPARE_EQUAL]    = "equal",
	        [PLG_COMPARE_LEQUAL]   = "lequal",
	        [PLG_COMPARE_GREATER]  = "greater",
	        [PLG_COMPARE_NOTEQUAL] = "notequal",
	        [PLG_COMPARE_GEQUAL]   = "gequal",
	        [PLG_COMPARE_ALWAYS]   = "always",
	};
	PL_STATIC_ASSERT( QM_OS_ARRAY_ELEMENTS( compareModeTags ) == PLG_MAX_COMPARE_FUNCTIONS, "" );

	for ( int i = 0; i < PLG_MAX_COMPARE_FUNCTIONS; ++i )
	{
		if ( strcmp( tag, compareModeTags[ i ] ) != 0 )
		{
			continue;
		}

		return i;
	}

	ape_console_warning_( "Invalid compare mode specified, \"%s\", defaulting to \"less\"!\n", tag );
	return PLG_COMPARE_LESS;
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
	PL_STATIC_ASSERT( QM_OS_ARRAY_ELEMENTS( blendModeTags ) == PLG_MAX_BLEND_MODES, "" );

	for ( int i = 0; i < PLG_MAX_BLEND_MODES; ++i )
	{
		if ( strcmp( tag, blendModeTags[ i ] ) != 0 )
		{
			continue;
		}

		return i;
	}

	ape_console_warning_( "Invalid blend mode specified, \"%s\", defaulting to \"none\"!\n", tag );
	return PLG_BLEND_NONE;
}

/**
 * Convert the given tag into it's built-in type.
 */
static ApeMaterialBuiltinVar get_built_in_by_tag( const char *tag )
{
	static const char *builtInTags[] = {
	        [APE_MATERIAL_BUILTIN_TIME]          = "time",
	        [APE_MATERIAL_BUILTIN_VIEWPORT_SIZE] = "vpsize",
	        [APE_MATERIAL_BUILTIN_FALLBACK]      = "proc_fallback",
	        [APE_MATERIAL_BUILTIN_LIGHTMAP]      = "lightmap",

	        [APE_MATERIAL_BUILTIN_RT_SPHERE] = "rt_sphere",
	};
	PL_STATIC_ASSERT( QM_OS_ARRAY_ELEMENTS( builtInTags ) == APE_MATERIAL_MAX_BUILTINS, "" );

	for ( int i = 0; i < APE_MATERIAL_MAX_BUILTINS; ++i )
	{
		if ( strncmp( tag, builtInTags[ i ], strlen( builtInTags[ i ] ) ) != 0 )
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
static bool validate_material_variable( ApeMaterialVariable *variable, QmGfxShaderUniformType uniformType )
{
	switch ( variable->type )
	{
		default:
			break;

		case SS_ARL_MATERIAL_VAR_FLOAT:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_FLOAT );
		case SS_ARL_MATERIAL_VAR_DOUBLE:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE );

		case SS_ARL_MATERIAL_VAR_INT:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_INT );
		case SS_ARL_MATERIAL_VAR_UINT:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_UINT );

		case SS_ARL_MATERIAL_VAR_BOOL:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_BOOL );

		case SS_ARL_MATERIAL_VAR_VEC2:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_VEC2 );
		case SS_ARL_MATERIAL_VAR_VEC3:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_VEC3 );
		case SS_ARL_MATERIAL_VAR_VEC4:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_VEC4 );

		case SS_ARL_MATERIAL_VAR_MAT3:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_MAT3 );
		case SS_ARL_MATERIAL_VAR_MAT4:
			return ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_MAT4 );

			/* special types */
		case APE_MATERIAL_VAR_RENDERTARGET:
		case APE_MATERIAL_VARIABLE_TYPE_DEPTHMAP:
		case APE_MATERIAL_VAR_TEXTURE:
		{
			return ( ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1D ) ||
			         ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1DSHADOW ) ||
			         ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D ) ||
			         ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2DSHADOW ) ||
			         ( uniformType == QM_GFX_SHADER_UNIFORM_TYPE_SAMPLERCUBE ) );
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
	ACM_ITERATE_BRANCH( root, node )
	{
		ApeMaterialVariable *materialVariable = &materialPass->variables[ materialPass->numVariables ];

		/* validate that the property actually exists or is at least exposed by the shader.
		 * in the long-term we'll be doing this against our own shader program object, but
		 * for now, just do it directly against the shader itself */
		const char *propertyName = acm_branch_get_name( node );

		materialVariable->programSlot = qm_gfx_shader_program_get_uniform_slot( materialPass->program->internal, propertyName );
		if ( materialVariable->programSlot == -1 )
		{
			ape_console_warning_( "Failed to fetch uniform slot for variable \"%s\"!\n", propertyName );
			continue;
		}

		materialVariable->numElements = qm_gfx_shader_program_get_num_uniform_elements( materialPass->program->internal, materialVariable->programSlot );
		if ( materialVariable->numElements == 0 )
		{
			ape_console_warning_( "Failed to fetch number of uniform elements for variable (%s/%u)!\n", propertyName, materialVariable->programSlot );
			continue;
		}

		qm_os_string_copy( materialVariable->name, propertyName, sizeof( materialVariable->name ) );

		/* if it's a string, it *could* be a built-in type */
		if ( acm_branch_get_type( node ) == ACM_PROPERTY_TYPE_STRING )
		{
			PLPath value;
			acm_branch_get_string( node, value, sizeof( value ) );
			if ( *value == '_' || *value == '%' )
			{
				const char *p = value + 1;
				// Render targets are "special" in the sense that we can specify what we want
				if ( strncmp( p, "rt_", 3 ) == 0 )
				{
					p += 3;
					materialVariable->type     = APE_MATERIAL_VAR_RENDERTARGET;
					materialVariable->data.ptr = qm_os_string_alloc( "%s", p );
				}
				else if ( strncmp( p, "depth_", 6 ) == 0 )
				{
					p += 6;
					materialVariable->type     = APE_MATERIAL_VARIABLE_TYPE_DEPTHMAP;
					materialVariable->data.ptr = qm_os_string_alloc( "%s", p );
				}
				else
				{
					/* lookup what it actually is */
					ApeMaterialBuiltinVar materialBuiltinVar = get_built_in_by_tag( p );
					if ( materialBuiltinVar == APE_MATERIAL_BUILTIN_INVALID )
					{
						ape_console_warning_( "Invalid built-in variable, \"%s\", specified!\n", value );
						continue;
					}

					/* todo: consider validating the built-in type here, but for now, we won't bother... */
					materialVariable->type            = APE_MATERIAL_VAR_BUILTIN;
					materialVariable->data.builtinVar = materialBuiltinVar;
				}
			}
		}

		QmGfxShaderUniformType uniformType = qm_gfx_shader_program_get_uniform_type( materialPass->program->internal, materialVariable->programSlot );

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

				case QM_GFX_SHADER_UNIFORM_TYPE_BOOL:
				{
					materialVariable->data.ptr = QM_OS_MEMORY_NEW_( bool, materialVariable->numElements );

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

				case QM_GFX_SHADER_UNIFORM_TYPE_FLOAT:
				{
					materialVariable->data.ptr = QM_OS_MEMORY_NEW_( float, materialVariable->numElements );

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
				case QM_GFX_SHADER_UNIFORM_TYPE_DOUBLE:
				{
					materialVariable->data.ptr = QM_OS_MEMORY_NEW_( double, materialVariable->numElements );

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

				case QM_GFX_SHADER_UNIFORM_TYPE_UINT:
				{
					materialVariable->data.ptr = QM_OS_MEMORY_NEW_( uint32_t, materialVariable->numElements );

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
				case QM_GFX_SHADER_UNIFORM_TYPE_INT:
				{
					materialVariable->data.ptr = QM_OS_MEMORY_NEW_( int32_t, materialVariable->numElements );

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

				case QM_GFX_SHADER_UNIFORM_TYPE_VEC2:
				{
					materialVariable->data.ptr = QM_OS_MEMORY_NEW_( QmMathVector2f, materialVariable->numElements );
					if ( acm_branch_get_float32_array( node, materialVariable->data.ptr, 2 * materialVariable->numElements ) != ND_ERROR_SUCCESS )
					{
						break;
					}

					materialVariable->type = SS_ARL_MATERIAL_VAR_VEC2;
					break;
				}

				case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLERCUBE:
				{
					if ( acm_get_num_of_children( node ) != QM_GFX_TEXTURE_MAX_CUBEMAP_FACES )
					{
						ape_console_warning_( "Invalid number of faces for cubemap!\n" );
						break;
					}

					char *paths[ QM_GFX_TEXTURE_MAX_CUBEMAP_FACES ] = {};
					if ( acm_branch_get_string_array( node, paths, QM_GFX_TEXTURE_MAX_CUBEMAP_FACES ) != ND_ERROR_SUCCESS )
					{
						ape_console_warning_( "Invalid cubemap variable setup!\n" );
						break;
					}

					ApeTexture *texture = ape_texture_cache_cubemap_( paths, materialPass->textureFilter );
					if ( texture != nullptr )
					{
						materialVariable->hint = APE_MATERIAL_VAR_HINT_DIFFUSE;

						QmGfxTexture *internal = texture->internal;
						if ( material != nullptr )
						{
							if ( material->width < internal->w )
							{
								material->width = internal->w;
							}
							if ( material->height < internal->h )
							{
								material->height = internal->h;
							}
						}

						materialVariable->data.ptr = texture;
						materialVariable->type     = APE_MATERIAL_VAR_TEXTURE;
					}

					for ( unsigned int i = 0; i < QM_GFX_TEXTURE_MAX_CUBEMAP_FACES; ++i )
					{
						ACM_DELETE( paths[ i ] );
					}

					break;
				}

				case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1D:
				case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2D:
				case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER3D:
				case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER1DSHADOW:
				case QM_GFX_SHADER_UNIFORM_TYPE_SAMPLER2DSHADOW:
				{
					PLPath texturePath;
					if ( acm_branch_get_string( node, texturePath, sizeof( PLPath ) ) != ND_ERROR_SUCCESS )
					{
						break;
					}

					ApeTexture *texture = ape_texture_cache_( texturePath, materialPass->textureFilter, true );
					if ( pl_strcasecmp( materialVariable->name, "diffuseMap" ) == 0 )
					{
						materialVariable->hint = APE_MATERIAL_VAR_HINT_DIFFUSE;

						/* this sucks, but shaders only deal with a default "pass" rather than a whole material, so
						 * it needs to be able to pass a null material... probably revisit this later. */
						// this now doubly sucks because we're assuming the diffuse map is representative of the "size" we care about,
						// which isn't necessarily always going to be the case... *sigh*
						QmGfxTexture *internal = texture->internal;
						if ( material != nullptr )
						{
							if ( material->width < internal->w )
							{
								material->width = internal->w;
							}
							if ( material->height < internal->h )
							{
								material->height = internal->h;
							}
						}
					}
					else if ( pl_strcasecmp( materialVariable->name, "normalMap" ) == 0 )
					{
						materialVariable->hint = APE_MATERIAL_VAR_HINT_NORMAL;
					}
					else if ( pl_strcasecmp( materialVariable->name, "specularMap" ) == 0 )
					{
						materialVariable->hint = APE_MATERIAL_VAR_HINT_SPECULAR;
					}
					else if ( pl_strcasecmp( materialVariable->name, "lightMap" ) == 0 )
					{
						materialVariable->hint = APE_MATERIAL_VAR_HINT_LIGHTMAP;
					}

					materialVariable->data.ptr = texture;
					materialVariable->type     = APE_MATERIAL_VAR_TEXTURE;
					break;
				}
			}

			if ( materialVariable->type == SS_ARL_MATERIAL_VAR_INVALID )
			{
				ape_console_warning_( "Invalid property type for shader variable \"%s\"!\n", propertyName );
				continue;
			}
		}

		if ( !validate_material_variable( materialVariable, uniformType ) )
		{
			ape_console_warning_( "Mismatch between material variable type and uniform type!\n" );
			continue;
		}

		// check if we've already got that property already urgh
		// good ol' linear search... :(
		unsigned int i;
		for ( i = 0; i < materialPass->numVariables; ++i )
		{
			if ( strcmp( propertyName, materialPass->variables[ i ].name ) == 0 )
			{
				//material_var_free( &materialPass->variables[ i ] );
				// commented out the above; I'm not sure what my intention was here?
				// we pull the variables initially from the shader default pass which
				// we wouldn't want to free yet??
				materialPass->variables[ i ] = *materialVariable;
				*materialVariable            = ( ApeMaterialVariable ) {};
				break;
			}
		}

		if ( i >= materialPass->numVariables )
		{
			materialPass->numVariables++;
		}
	}
}

static QmGfxTextureFilter get_texture_filter_by_name( const char *name )
{
	QmGfxTextureFilter textureFilter;
	if ( pl_strcasecmp( name, "mipmap_nearest" ) == 0 )
	{
		textureFilter = PLG_TEXTURE_FILTER_MIPMAP_NEAREST;
	}
	else if ( pl_strcasecmp( name, "mipmap_linear" ) == 0 )
	{
		textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
	}
	else if ( pl_strcasecmp( name, "mipmap_linear_nearest" ) == 0 )
	{
		textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR_NEAREST;
	}
	else if ( pl_strcasecmp( name, "mipmap_nearest_linear" ) == 0 )
	{
		textureFilter = PLG_TEXTURE_FILTER_MIPMAP_NEAREST_LINEAR;
	}
	else if ( pl_strcasecmp( name, "nearest" ) == 0 )
	{
		textureFilter = PLG_TEXTURE_FILTER_NEAREST;
	}
	else if ( pl_strcasecmp( name, "linear" ) == 0 )
	{
		textureFilter = PLG_TEXTURE_FILTER_LINEAR;
	}
	else
	{
		textureFilter = PLG_TEXTURE_FILTER_LINEAR;
		ape_console_warning_( "Encountered an invalid texture filter type (%s), reverting to linear!\n", name );
	}

	return textureFilter;
}

void ape_parse_material_pass_( ApeMaterial *material, AcmBranch *root, ApeMaterialPass *materialPass )
{
	/* fetch the blend mode we'll use for the pass */
	AcmBranch *subNode;
	if ( ( subNode = acm_get_child_by_name( root, "blendMode" ) ) != NULL )
	{
		char *blendModesArray[ 2 ];
		if ( acm_branch_get_string_array( subNode, blendModesArray, 2 ) == ND_ERROR_SUCCESS )
		{
			materialPass->blendMode[ 0 ] = get_blend_mode_by_tag( blendModesArray[ 0 ] );
			ACM_DELETE( blendModesArray[ 0 ] );
			materialPass->blendMode[ 1 ] = get_blend_mode_by_tag( blendModesArray[ 1 ] );
			ACM_DELETE( blendModesArray[ 1 ] );
		}
		else
		{
			ape_console_warning_( "Invalid blend mode array in material!\n" );
		}
	}
	else
	{
		materialPass->blendMode[ 0 ] = PLG_BLEND_NONE;
		materialPass->blendMode[ 1 ] = PLG_BLEND_NONE;
	}

	const char *tmp;
	if ( ( tmp = acm_get_string( root, "depthMode", "lequal" ) ) != nullptr )
	{
		materialPass->depthMode = get_compare_mode_by_tag( tmp );
	}

	materialPass->depthMask = acm_get_bool( root, "depthMask", materialPass->depthMask );
	materialPass->cullMode  = ACM_GET_UINT( materialPass->cullMode, root, "cullMode", materialPass->cullMode );
	if ( material != nullptr && materialPass->cullMode != 1 )
	{
		material->flags |= APE_MATERIAL_FLAG_NO_CULL;
	}

	if ( ( tmp = acm_get_string( root, "textureFilterMode", nullptr ) ) != NULL )
	{
		materialPass->textureFilter = get_texture_filter_by_name( tmp );
	}

	// now handle any specific parameters the material provides
	materialPass->textureScroll = com_acm_get_vector2( root, "textureScroll", &QM_MATH_VECTOR2F( 0.0f, 0.0f ) );
	materialPass->textureOffset = com_acm_get_vector2( root, "textureOffset", &QM_MATH_VECTOR2F( 0.0f, 0.0f ) );
	materialPass->textureScale  = com_acm_get_vector2( root, "textureScale", &QM_MATH_VECTOR2F( 1.0f, 1.0f ) );

	if ( ( subNode = acm_get_child_by_name( root, "shaderParameters" ) ) != NULL )
	{
		/* there's some extra complexity when parsing in parameters, so we'll defer that
		 * to another function */
		parse_shader_parameters( material, materialPass, subNode );
	}

	// this should follow after the above, as it initialises the list of
	// material variables, which we might want to look up after
	if ( ( subNode = acm_get_child_by_name( root, "animators" ) ) != nullptr )
	{
		ape_material_animator_parse_array_( subNode, materialPass );
	}

	/* not sure whether the above section should be required yet, there might be
	 * a case where we only want to use the shader defaults? */
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

			if ( programIndex->flags & APE_SHADER_PROGRAM_FLAG_SUPPORTS_LIGHTMAP )
			{
				material->flags |= APE_MATERIAL_FLAG_LIGHTMAP;
			}

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
	if ( !acm_get_bool( root, "receiveShadows", material->flags & APE_MATERIAL_FLAG_RECEIVE_SHADOWS ) )
	{
		material->flags &= ~APE_MATERIAL_FLAG_RECEIVE_SHADOWS;
	}
	if ( !acm_get_bool( root, "castShadows", material->flags & APE_MATERIAL_FLAG_CAST_SHADOWS ) )
	{
		material->flags &= ~APE_MATERIAL_FLAG_CAST_SHADOWS;
	}
	if ( acm_get_bool( root, "mirror", false ) )
	{
		material->flags |= APE_MATERIAL_FLAG_MIRROR;
	}

	material->lightEmission = com_acm_get_colour_f32( root, "light", &QM_MATH_COLOUR4F_ZERO );
	if ( !qm_math_colour4f_compare( material->lightEmission, QM_MATH_COLOUR4F_ZERO ) )
	{
		material->flags |= APE_MATERIAL_FLAG_EMISSIVE;
	}

	if ( material->numPasses == 0 )
	{
		ape_console_warning_( "No passes specified for material!\n" );
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

static void material_var_free( ApeMaterialVariable *var )
{
	switch ( var->type )
	{
		case APE_MATERIAL_VAR_BUILTIN:
			//TODO!!!
			break;
		case APE_MATERIAL_VAR_TEXTURE:
			ape_texture_release_reference( var->data.ptr );
			break;
		default:
			qm_os_memory_free( var->data.ptr );
			break;
	}

	// zero it
	*var = ( ApeMaterialVariable ) {};
}

static void ape_material_pass_free_( ApeMaterialPass *pass )
{
	for ( unsigned int i = 0; i < pass->numVariables; ++i )
	{
		material_var_free( &pass->variables[ i ] );
	}

	if ( pass->animators != nullptr )
	{
		for ( unsigned int i = 0; i < pass->numAnimators; ++i )
		{
			ape_material_animator_free_( &pass->animators[ i ] );
		}

		qm_os_memory_free( pass->animators );

		pass->animators    = nullptr;
		pass->numAnimators = 0;
	}
}

static void destroy_material( ApeMaterial *material )
{
	if ( material == NULL )
	{
		return;
	}

	for ( unsigned int i = 0; i < material->numPasses; ++i )
	{
		ape_material_pass_free_( &material->passes[ i ] );
	}

	PLLinkedList *container = PlGetLinkedListNodeContainer( material->node );
	if ( container != NULL )
	{
		PlDestroyLinkedListNode( material->node );
	}

	qm_os_memory_free( material );
}

static void destroy_material_callback( void *userData )
{
	destroy_material( userData );
}

/**
 * This will attempt to draw from the perspective of the sphere.
 * This is also really *really* expensive right now, so uh, use sparingly.
 *
 * @param material 	Pointer to the material.
 * @param mesh		Pointer to the origin mesh.
 */
static void draw_rt_sphere( ApeMaterial *material, QmGfxMesh *mesh )
{
	//TODO: draw_rt_sphere

#if 0
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
	QmMathVector3f       origin = bounds.absOrigin;
	QmMathVector3f       dir    = qm_math_vector3f_sub( APE_SG_NODE_GET_POSITION( camera ), origin );

	// save some of the camera state, so we can do dumb shit

	ape_camera_build_pvs_( camera );

	reject = nullptr;
#endif
}

static void set_built_in_variable( ApeMaterial *material, const ApeMaterialPass *pass, QmGfxMesh *mesh, int uniformSlot, int variable, unsigned int *curUnit )
{
	if ( variable == -1 )
	{
		return;
	}

	QmGfxShaderProgram *program = pass->program->internal;
	switch ( variable )
	{
		case APE_MATERIAL_BUILTIN_TIME:
		{
			unsigned int numTicks = ape_get_num_ticks();
			qm_gfx_shader_program_set_uniform( program, uniformSlot, &numTicks, false );
			break;
		}

		case APE_MATERIAL_BUILTIN_FALLBACK:
		{
			QmGfxTexture *texture = ape_get_default_texture_( APE_TEXTURE_FALLBACK )->internal;
			assert( texture != nullptr );
			qm_gfx_texture_set( texture, *curUnit );
			qm_gfx_shader_program_set_uniform( program, uniformSlot, curUnit, false );
			( *curUnit )++;
			break;
		}
		case APE_MATERIAL_BUILTIN_LIGHTMAP:
		{
			QmGfxTexture *texture = ape_rendererState_.lightmapTexture;
			if ( texture == nullptr )
			{
				texture = ape_get_default_texture_( APE_TEXTURE_WHITE )->internal;
			}

			qm_gfx_texture_set( texture, *curUnit );
			qm_gfx_shader_program_set_uniform( program, uniformSlot, curUnit, false );
			( *curUnit )++;
			break;
		}

		case APE_MATERIAL_BUILTIN_VIEWPORT_SIZE:
		{
			int w, h;
			ape_get_2d_viewport_size_( &w, &h );
			qm_gfx_shader_program_set_uniform( program, uniformSlot, &QM_MATH_VECTOR2F( ( float ) w, ( float ) h ), false );
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

static void set_global_uniforms( const ApeShaderProgram *program, const ApeMaterialPass *pass, const ApeRendererPassState *state )
{
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_NUM_TICKS ] >= 0 )
	{
		double numTicks = ( double ) ape_get_num_ticks();
		qm_gfx_shader_program_set_uniform( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_NUM_TICKS ], &numTicks, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_VIEW_SIZE ] >= 0 )
	{
		int w, h;
		ape_get_2d_viewport_size_( &w, &h );

		QmMathVector2f viewSize = qm_math_vector2f( ( float ) w, ( float ) h );
		qm_gfx_shader_program_set_uniform( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_VIEW_SIZE ], &viewSize, false );
	}

	if ( program->globalUniforms[ APE_SHADER_UNIFORM_FOG_COLOUR ] >= 0 )
	{
		qm_gfx_shader_program_set_uniform( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_FOG_COLOUR ], &ape_rendererState_.fogColour, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_FOG_NEAR ] >= 0 )
	{
		qm_gfx_shader_program_set_uniform( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_FOG_NEAR ], &ape_rendererState_.fogNear, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_FOG_FAR ] >= 0 )
	{
		qm_gfx_shader_program_set_uniform( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_FOG_FAR ], &ape_rendererState_.fogFar, false );
	}

	if ( program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_AMBIENCE ] >= 0 )
	{
		qm_gfx_shader_program_set_uniform( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_AMBIENCE ], &state->lighting.ambience, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_COLOUR ] >= 0 )
	{
		QmMathColour3f lightColour = QM_MATH_COLOUR3F16_TO_3F( state->lighting.colour );
		qm_gfx_shader_program_set_uniform( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_COLOUR ], &lightColour, false );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_DIRECTION ] >= 0 )
	{
		qm_gfx_shader_program_set_uniform( program->internal, program->globalUniforms[ APE_SHADER_UNIFORM_LIGHT_DIRECTION ], &state->lighting.dir, false );
	}

	if ( program->globalUniforms[ APE_SHADER_UNIFORM_TEXTURE_MATRIX ] >= 0 )
	{
		PLMatrixMode oldMode = PlGetMatrixMode();
		PlMatrixMode( PL_TEXTURE_MATRIX );

		PlPushMatrix();

		PlTranslateMatrix( qm_math_vector3f( pass->textureOffset.x, pass->textureOffset.y, 0.0f ) );

		float scaleX = pass->textureScale.x != 0.0f ? 1.0f / pass->textureScale.x : 1.0f;
		float scaleY = pass->textureScale.y != 0.0f ? 1.0f / pass->textureScale.y : 1.0f;
		PlScaleMatrix( qm_math_vector3f( scaleX, scaleY, 1.0f ) );

		qm_gfx_shader_program_set_uniform( program->internal,
		                                   program->globalUniforms[ APE_SHADER_UNIFORM_TEXTURE_MATRIX ],
		                                   PlGetMatrix( PL_TEXTURE_MATRIX ), false );

		PlPopMatrix();

		PlMatrixMode( oldMode );
	}
	if ( program->globalUniforms[ APE_SHADER_UNIFORM_MODEL_MATRIX ] >= 0 )
	{
		qm_gfx_shader_program_set_uniform( program->internal,
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
		ape_memory_reference_add( &material->mem );
		return material;
	}

	/* fallback should be optional, as in some cases we might actually care */
	ApeMaterial *fallbackPtr = useFallback ? defaultMaterials[ APE_MATERIAL_DEFAULT_FALLBACK ] : nullptr;

	AcmBranch *root = com_acm_load_file( path, "material" );
	if ( root == NULL )
	{
		ape_console_warning_( "Failed to load material, \"%s\" (%s)!\n", path, acm_get_error_message() );
		return fallbackPtr;
	}

	material = QM_OS_MEMORY_NEW( ApeMaterial );
	qm_os_string_copy( material->path, path, sizeof( material->path ) );

	parse_material( material, root );

	acm_branch_destroy( root );

	material->node = PlInsertLinkedListNode( materials[ group ], material );

	ape_memory_setup_reference( material->path, APE_CACHE_POOL_MATERIALS, &material->mem, destroy_material_callback, material );
	ape_memory_reference_add( &material->mem );

	return material;
}

APE_MEMORY_IMPLEMENT_INTERFACE( ape_material, ApeMaterial, mem )

int8_t ape_material_get_surface_type( const ApeMaterial *material )
{
	return material->surfaceType;
}

bool ape_material_can_cast_shadows( const ApeMaterial *self )
{
	return self->flags & APE_MATERIAL_FLAG_CAST_SHADOWS;
}

bool ape_material_can_receive_shadows( const ApeMaterial *self )
{
	return self->flags & APE_MATERIAL_FLAG_RECEIVE_SHADOWS;
}

bool ape_material_is_blended( const ApeMaterial *self )
{
	return self->flags & APE_MATERIAL_FLAG_BLENDED;
}

bool ape_material_is_cull_enabled_( const ApeMaterial *self )
{
	return !( self->flags & APE_MATERIAL_FLAG_NO_CULL );
}

bool ape_material_is_emissive_( const ApeMaterial *self )
{
	return self->flags & APE_MATERIAL_FLAG_EMISSIVE;
}

QmMathColour4f ape_material_get_emission_( const ApeMaterial *self )
{
	return self->lightEmission;
}

unsigned int ape_material_get_flags_( const ApeMaterial *self )
{
	return self->flags;
}

static QmGfxTexture *ape_material_var_get_texture_( ApeMaterialVariable *var )
{
	QmGfxTexture *texture = nullptr;
	if ( ( var->hint == APE_MATERIAL_VAR_HINT_DIFFUSE && materialSkipDiffuse ) ||
	     ( var->hint == APE_MATERIAL_VAR_HINT_LIGHTMAP && materialSkipLightmap ) )
	{
		texture = ape_get_default_texture_( APE_TEXTURE_WHITE )->internal;
	}
	else if ( var->hint == APE_MATERIAL_VAR_HINT_NORMAL && materialSkipNormal )
	{
		texture = normalFallbackTexture->internal;
	}
	else if ( var->hint == APE_MATERIAL_VAR_HINT_SPECULAR && materialSkipSpecular )
	{
		texture = ape_get_default_texture_( APE_TEXTURE_BLACK )->internal;
	}
	else if ( var->type == APE_MATERIAL_VAR_RENDERTARGET )
	{
		ApeRenderTarget *renderTarget = ape_render_target_get_by_key_( var->data.ptr );
		if ( renderTarget != nullptr )
		{
			texture = ape_render_target_get_texture_( renderTarget, APE_RENDER_TARGET_ATTACHMENT_TYPE_COLOUR );
		}
	}
	else if ( var->type == APE_MATERIAL_VARIABLE_TYPE_DEPTHMAP )
	{
		ApeRenderTarget *renderTarget = ape_render_target_get_by_key_( var->data.ptr );
		if ( renderTarget != nullptr )
		{
			texture = ape_render_target_get_texture_( renderTarget, APE_RENDER_TARGET_ATTACHMENT_TYPE_DEPTH );
		}
	}
	else
	{
		if ( var->animator != nullptr )
		{
			ApeMaterialAnimator *animator = var->animator;
			//TODO: should we validate the type here?? everything should've been validated at load

			unsigned int frame = animator->state.frame;
			if ( frame >= animator->numFrames )
			{
				frame = animator->numFrames - 1;
			}

			texture = animator->texture.frames[ frame ]->internal;
		}
		else
		{
			texture = ( ( ApeTexture * ) var->data.ptr )->internal;
		}
	}

	if ( texture == nullptr )
	{
		texture = ape_get_default_texture_( APE_TEXTURE_FALLBACK )->internal;
		assert( texture != nullptr );
	}

	return texture;
}

void ape_material_draw( ApeMaterial *material, QmGfxMesh *mesh, const ApeRendererPassState *state )
{
	if ( state == nullptr )
	{
		state = &ape_rendererState_;
	}

	ApeCamera *camera = state->camera;
	if ( camera != nullptr )
	{
#if 0
		if ( camera->drawMode == APE_CAMERA_DRAW_MODE_TEXTURED || camera->drawMode == APE_CAMERA_DRAW_MODE_SOLID )
		{
			state->lighting.ambience = QM_MATH_COLOUR3F( 1.0f, 1.0f, 1.0f );
		}
#endif

		if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SOLID )
		{
			material = defaultMaterials[ APE_MATERIAL_DEFAULT_VERTEX ];
		}
	}

	if ( state->overrideBlendMode )
	{
		PlgEnableGraphicsState( PLG_GFX_STATE_BLEND );
		PlgSetBlendMode( ape_rendererState_.blendModeA, ape_rendererState_.blendModeB );
	}

	if ( ape_rendererState_.overrideDepthMode )
	{
		PlgDepthBufferFunction( ape_rendererState_.overrideDepthMode );
	}

	for ( unsigned int i = 0; i < material->numPasses; ++i )
	{
		ApeMaterialPass *curPass = &material->passes[ i ];

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

		// Mirror mode requires flipping the matrix,
		// so we'll need to update the cull mode
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

		PlgDepthMask( ape_rendererState_.overrideDepthMask ? ape_rendererState_.depthMask : curPass->depthMask );

		// we have an awkward check for wireframe here because we don't want to just blindly handle it globally,
		// otherwise UI elements will be wireframe too, so instead we'll just check the plg state flag
		if ( PlgIsGraphicsStateEnabled( PLG_GFX_STATE_WIREFRAME ) )
		{
			ApeShaderProgram *program = ape_get_default_shader( APE_SHADER_DEFAULT_VERTEX );
			ape_shader_set_active_( program );

			set_global_uniforms( curPass->program, curPass, nullptr );

			qm_gfx_texture_set( nullptr, 0 );
		}
		else
		{
			PlgSetShaderProgram( curPass->program->internal );

			if ( !ape_rendererState_.overrideBlendMode )
			{
				PlgSetBlendMode( curPass->blendMode[ 0 ], curPass->blendMode[ 1 ] );
			}
			if ( !ape_rendererState_.overrideDepthMode )
			{
				PlgDepthBufferFunction( curPass->depthMode );
			}

			set_global_uniforms( curPass->program, curPass, state );

			unsigned int curUnit = 0;
			for ( unsigned int j = 0; j < curPass->numVariables; ++j )
			{
				if ( curPass->variables[ j ].type == APE_MATERIAL_VAR_BUILTIN )
				{
					set_built_in_variable( material, curPass, mesh, curPass->variables[ j ].programSlot, curPass->variables[ j ].data.builtinVar, &curUnit );
					continue;
				}

				// textures just need to be set per their respective unit
				if ( curPass->variables[ j ].type == APE_MATERIAL_VAR_TEXTURE ||
				     //TODO: change how we handle special types here, should probably move this logic under set_built_in_variable
				     curPass->variables[ j ].type == APE_MATERIAL_VAR_RENDERTARGET ||
				     curPass->variables[ j ].type == APE_MATERIAL_VARIABLE_TYPE_DEPTHMAP )
				{
					QmGfxTexture *texture = ape_material_var_get_texture_( &curPass->variables[ j ] );

					qm_gfx_texture_set( texture, curUnit );

					// setup the texture filtering mode

					QmGfxTextureFilter textureFilter;
					// allow us to override the desired texture filter
					if ( *materialTextureFilter != '\0' )
					{
						textureFilter = get_texture_filter_by_name( materialTextureFilter );
					}
					else
					{
						textureFilter = curPass->textureFilter;
					}

					if ( texture->flags & PLG_TEXTURE_FLAG_NOMIPS )
					{
						// ensure that if the texture is flagged with no mips, the filter mode is valid!
						if ( textureFilter == PLG_TEXTURE_FILTER_MIPMAP_LINEAR || textureFilter == PLG_TEXTURE_FILTER_MIPMAP_LINEAR_NEAREST )
						{
							textureFilter = PLG_TEXTURE_FILTER_LINEAR;
						}
						else if ( textureFilter == PLG_TEXTURE_FILTER_MIPMAP_NEAREST || textureFilter == PLG_TEXTURE_FILTER_MIPMAP_NEAREST_LINEAR )
						{
							textureFilter = PLG_TEXTURE_FILTER_NEAREST;
						}
					}

					qm_gfx_texture_set_filter( texture, textureFilter );

					if ( !( texture->flags & PLG_TEXTURE_FLAG_NOMIPS ) )
					{
						qm_gfx_texture_set_anisotropy( texture, materialTextureAnisotropy );
					}

					qm_gfx_shader_program_set_uniform( curPass->program->internal, curPass->variables[ j ].programSlot, &curUnit, false );
					curUnit++;
					continue;
				}

				qm_gfx_shader_program_set_uniform( curPass->program->internal, curPass->variables[ j ].programSlot, curPass->variables[ j ].data.ptr, false );
			}
		}

		qm_gfx_mesh_upload( mesh, nullptr, nullptr );
		qm_gfx_mesh_draw( mesh );

		ape_rendererPerformance_.numBatches++;
		if ( mesh->primitive == QM_GFX_MESH_PRIMITIVE_TRIANGLES )
		{
			ape_rendererPerformance_.numTriangles += mesh->num_triangles;
		}
		else
		{
			ape_rendererPerformance_.numTriangles += mesh->num_verts / 2;
		}
	}

	// reset everything back before the next pass
	PlgDepthBufferFunction( APE_RENDERER_DEFAULT_DEPTH_FUNCTION );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	PlgDepthMask( true );

	PlgSetCullMode( APE_RENDERER_DEFAULT_CULL_FUNCTION );
}

static void ape_material_pass_tick_( ApeMaterialPass *self, const ApeMaterial *material, double delta )
{
	//TODO: wat???
	QmMathVector2f scroll = {};
	if ( !( fabsf( self->textureScroll.x ) < QM_MATH_EPSILON && fabsf( self->textureScroll.y ) < QM_MATH_EPSILON ) )
	{
		// before it was operating by the current pass texture
		// honestly this might not really be the best approach,
		// with the former being better, will see...
		float w = ( float ) material->width;
		float h = ( float ) material->height;

		scroll = qm_math_vector2f_div( self->textureScroll, qm_math_vector2f( w, h ) );
	}

	scroll              = qm_math_vector2f_scale( scroll, QM_MATH_VECTOR2F( self->textureScale.x, self->textureScale.y ) );
	self->textureOffset = qm_math_vector2f_add( self->textureOffset, scroll );

	for ( unsigned int i = 0; i < self->numAnimators; ++i )
	{
		ape_material_animator_tick_( &self->animators[ i ], delta );
	}
}

void ape_tick_materials_( double delta )
{
	for ( unsigned int i = 0; i < APE_MAX_CACHE_GROUPS; ++i )
	{
		ApeMaterial *material;
		COM_ITERATE_LINKED_LIST( material, materials[ i ], itr )
		{
			for ( unsigned int j = 0; j < material->numPasses; ++j )
			{
				ape_material_pass_tick_( &material->passes[ j ], material, delta );
			}
		}
	}
}

PLLinkedList *ape_material_get_group_( ApeCacheGroup group )
{
	return materials[ group ];
}

unsigned int ape_material_get_width( const ApeMaterial *self )
{
	return self->width;
}

unsigned int ape_material_get_height( const ApeMaterial *self )
{
	return self->height;
}

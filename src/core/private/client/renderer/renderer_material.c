// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_linkedlist.h>

#include "ape_private.h"
#include "renderer.h"
#include "renderer_material.h"
#include "renderer_render_target.h"
#include "world/world.h"

#include "../gui/gui_private.h"

static PLLinkedList *materials[ APE_MAX_CACHE_GROUPS ];

static PLGTexture *specularFallbackTexture;
static PLGTexture *normalFallbackTexture;
static PLGTexture *previewFallbackTexture;

typedef struct ApeMaterial
{
	char path[ PL_SYSTEM_MAX_PATH ];
	ApeMaterialPass passes[ ARL_MAX_MATERIAL_PASSES ];
	unsigned int numPasses;
	bool isCached;      // if false, it's just the preview
	PLGTexture *preview;// preview utilised for editor
	PLLinkedListNode *node;

	int8_t surfaceType;

	bool enableShadows;

	ApeMemoryReference mem;
} ApeMaterial;

static ApeMaterial *defaultMaterials[ APE_MAX_DEFAULT_MATERIALS ];

ApeMaterial *arl_material_get_default( ApeDefaultMaterial defaultMaterial )
{
	assert( defaultMaterial != APE_MAX_DEFAULT_MATERIALS );
	if ( defaultMaterial == APE_MAX_DEFAULT_MATERIALS )
		return defaultMaterials[ APE_MATERIAL_BUILTIN_FALLBACK ];

	return defaultMaterials[ defaultMaterial ];
}

ApeMaterial *ar_material_get_fallback( void ) { return arl_material_get_default( APE_MATERIAL_DEFAULT_FALLBACK ); }
ApeMaterial *ar_material_get_default_vertex( void ) { return arl_material_get_default( APE_MATERIAL_DEFAULT_VERTEX ); }

void arl_initialize_materials_( void )
{
	PRINT( "Initializing material system\n" );

	for ( unsigned int i = 0; i < APE_MAX_CACHE_GROUPS; ++i )
	{
		materials[ i ] = PlCreateLinkedList();
		if ( materials[ i ] == NULL )
			PRINT_ERROR( "Failed to create materials list: %s\n", PlGetError() );
	}

	normalFallbackTexture = arl_texture_load_direct_( "materials/shaders/textures/normal.tga", PLG_TEXTURE_FILTER_LINEAR );
	specularFallbackTexture = arl_texture_load_direct_( "materials/shaders/textures/black.png", PLG_TEXTURE_FILTER_LINEAR );
	previewFallbackTexture = arl_texture_load_direct_( "materials/editor/no_preview.png", PLG_TEXTURE_FILTER_NEAREST );

	// cache default materials we need
	static const char *defaultMaterialPaths[ APE_MAX_DEFAULT_MATERIALS ] =
	        {
	                "materials/engine/fallback.mat.n",
	                "materials/engine/vertex.mat.n",
	                "materials/engine/shadow.mat.n",
	                "materials/engine/depth.mat.n",
	        };
	for ( unsigned int i = 0; i < APE_MAX_DEFAULT_MATERIALS; ++i )
	{
		defaultMaterials[ i ] = apeCacheMaterial( defaultMaterialPaths[ i ], APE_CACHE_WORLD, false, false );
		if ( defaultMaterials[ i ] == NULL )
			PRINT_ERROR( "Failed to cache default material: %s\n", defaultMaterialPaths[ i ] );
	}
}

void arl_shutdown_materials_( void )
{
	/* Flush any objects pending deletion in case they are holding a material handle. */
	apeFlushUnreferencedResources();

	unsigned int totalCachedMaterials = 0;
	unsigned int orphanedCaches = 0;

	for ( unsigned int i = 0; i < APE_MAX_CACHE_GROUPS; ++i )
	{
		unsigned int cached_materials = PlGetNumLinkedListNodes( materials[ i ] );
		totalCachedMaterials += cached_materials;

		if ( cached_materials == 0 )
		{
			/* and now destroy the list */
			PlDestroyLinkedList( materials[ i ] );
			materials[ i ] = NULL;
		}
		else
		{
			++orphanedCaches;
		}
	}

	if ( totalCachedMaterials > 0 )
	{
		PRINT_WARNING( "Shutting down material system with %u active materials, orphaned %u caches!\n",
		               totalCachedMaterials, orphanedCaches );
	}
}

const char *ar_material_get_path( const ApeMaterial *material )
{
	return material->path;
}

PLGTexture *ar_material_get_preview_texture( ApeMaterial *material )
{
	return material->preview;
}

/**
 * Convert the given tag into a blend mode type.
 */
static int get_blend_mode_by_tag( const char *tag )
{
	static const char *blendModeTags[] = {
	        [PLG_BLEND_NONE] = "none",
	        [PLG_BLEND_ZERO] = "zero",
	        [PLG_BLEND_ONE] = "one",
	        [PLG_BLEND_SRC_COLOR] = "src_color",
	        [PLG_BLEND_ONE_MINUS_SRC_COLOR] = "one_minus_src_color",
	        [PLG_BLEND_SRC_ALPHA] = "src_alpha",
	        [PLG_BLEND_ONE_MINUS_SRC_ALPHA] = "one_minus_src_alpha",
	        [PLG_BLEND_DST_ALPHA] = "dst_alpha",
	        [PLG_BLEND_ONE_MINUS_DST_ALPHA] = "one_minus_dst_alpha",
	        [PLG_BLEND_DST_COLOR] = "dst_color",
	        [PLG_BLEND_ONE_MINUS_DST_COLOR] = "one_minus_dst_color",
	        [PLG_BLEND_SRC_ALPHA_SATURATE] = "src_alpha_saturate",
	};
	PL_STATIC_ASSERT( PL_ARRAY_ELEMENTS( blendModeTags ) == PLG_MAX_BLEND_MODES, "" );

	for ( int i = 0; i < PLG_MAX_BLEND_MODES; ++i )
	{
		if ( strcmp( tag, blendModeTags[ i ] ) != 0 )
			continue;

		return i;
	}

	PRINT_WARNING( "Invalid blend mode specified, \"%s\", defaulting to \"none\"!\n", tag );
	return PLG_BLEND_NONE;
}

/**
 * Convert the given tag into it's built-in type.
 */
static ApeMaterialBuiltinVar get_built_in_by_tag( const char *tag )
{
	static const char *builtInTags[] = {
	        [APE_MATERIAL_BUILTIN_TIME] = "time",
	        [APE_MATERIAL_BUILTIN_DEPTH] = "depth",
	        [APE_MATERIAL_BUILTIN_VIEWPORT_SIZE] = "vpsize",
	        [APE_MATERIAL_BUILTIN_FALLBACK] = "proc_fallback",
	};
	PL_STATIC_ASSERT( PL_ARRAY_ELEMENTS( builtInTags ) == ARL_MAX_MATERIAL_BUILTINS, "" );

	for ( int i = 0; i < ARL_MAX_MATERIAL_BUILTINS; ++i )
	{
		if ( strcmp( tag, builtInTags[ i ] ) != 0 )
		{
			continue;
		}

		return i;
	}

	return ARL_MATERIAL_BUILTIN_INVALID;
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

		case ARL_MATERIAL_VAR_FLOAT:
			return ( uniformType == PLG_UNIFORM_FLOAT );
		case MATERIAL_VAR_DOUBLE:
			return ( uniformType == PLG_UNIFORM_DOUBLE );

		case ARL_MATERIAL_VAR_INT:
			return ( uniformType == PLG_UNIFORM_INT );
		case ARL_MATERIAL_VAR_UINT:
			return ( uniformType == PLG_UNIFORM_UINT );

		case MATERIAL_VAR_BOOL:
			return ( uniformType == PLG_UNIFORM_BOOL );

		case MATERIAL_VAR_VEC2:
			return ( uniformType == PLG_UNIFORM_VEC2 );
		case MATERIAL_VAR_VEC3:
			return ( uniformType == PLG_UNIFORM_VEC3 );
		case MATERIAL_VAR_VEC4:
			return ( uniformType == PLG_UNIFORM_VEC4 );

		case MATERIAL_VAR_MAT3:
			return ( uniformType == PLG_UNIFORM_MAT3 );
		case MATERIAL_VAR_MAT4:
			return ( uniformType == PLG_UNIFORM_MAT4 );

			/* special types */
		case MATERIAL_VAR_RENDERTARGET:
		case MATERIAL_VAR_TEXTURE:
		{
			return ( ( uniformType == PLG_UNIFORM_SAMPLER1D ) ||
			         ( uniformType == PLG_UNIFORM_SAMPLER1DSHADOW ) ||
			         ( uniformType == PLG_UNIFORM_SAMPLER2D ) ||
			         ( uniformType == PLG_UNIFORM_SAMPLER2DSHADOW ) ||
			         ( uniformType == PLG_UNIFORM_SAMPLERCUBE ) );
		}
		case MATERIAL_VAR_BUILTIN:
			return true;
	}

	return false;
}

/**
 * Iterate through each of the parameters provided in the 'shaderParameters'
 * block of the material.
 */
static void parse_shader_parameters( ApeMaterialPass *materialPass, NdBranch *root )
{
	NdBranch *node = ndGetFirstChild( root );
	while ( node != NULL )
	{
		/* fetch the next node, so we can roll onto the next element early */
		NdBranch *next = ndGetNextChild( node );

		ApeMaterialVariable *materialVariable = &materialPass->variables[ materialPass->numVariables ];

		/* validate that the property actually exists or is at least exposed by the shader.
		 * in the long-term we'll be doing this against our own shader program object, but
		 * for now, just do it directly against the shader itself */
		const char *propertyName = ndGetName( node );
		materialVariable->programSlot = PlgGetShaderUniformSlot( materialPass->program, propertyName );
		if ( materialVariable->programSlot == -1 )
		{
			PRINT_WARNING( "Failed to fetch uniform slot for variable \"%s\"!\n", propertyName );
			node = next;
			continue;
		}

		snprintf( materialVariable->name, sizeof( materialVariable->name ), "%s", propertyName );

		PLGShaderUniformType uniformType = PlgGetShaderUniformType( materialPass->program, materialVariable->programSlot );

		/* if it's a string, it *could* be a built-in type */
		if ( ndGetType( node ) == ND_PROPERTY_STRING )
		{
			PLPath value;
			ndGetStr( node, value, sizeof( value ) );
			if ( *value == '_' )
			{
				const char *p = ( value + 1 );
				// Render targets are "special" in the sense that we can specify what we want
				if ( strncmp( p, "rt_", 3 ) == 0 )
				{
					p += 3;
					ArRenderTarget *renderTarget = ar_render_target_get_by_key( p );
					if ( renderTarget == NULL )
					{// Passing flag of 0 to create a placeholder
						renderTarget = arl_render_target_create( p, 64, 64, 0, PLG_BUFFER_COLOUR, PLG_TEXTURE_FILTER_LINEAR );
					}

					materialVariable->type = MATERIAL_VAR_RENDERTARGET;
					materialVariable->data.userPtr = renderTarget;
				}
				else
				{
					/* lookup what it actually is */
					ApeMaterialBuiltinVar materialBuiltinVar = get_built_in_by_tag( p );
					if ( materialBuiltinVar == ARL_MATERIAL_BUILTIN_INVALID )
					{
						PRINT_WARNING( "Invalid built-in variable, \"%s\", specified!\n", value );
						node = next;
						continue;
					}

					/* todo: consider validating the built-in type here, but for now, we won't bother... */
					materialVariable->type = MATERIAL_VAR_BUILTIN;
					materialVariable->data.i32 = materialBuiltinVar;
				}
			}
		}

		/* otherwise, we need to handle it as a traditional var */
		if ( materialVariable->type == ARL_MATERIAL_VAR_INVALID )
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
					if ( ndGetBool( node, &materialVariable->data.boolean ) != ND_ERROR_SUCCESS )
						break;

					materialVariable->type = MATERIAL_VAR_BOOL;
					break;
				}

				case PLG_UNIFORM_FLOAT:
				{
					if ( ndGetF32( node, &materialVariable->data.f32 ) != ND_ERROR_SUCCESS )
						break;

					materialVariable->type = ARL_MATERIAL_VAR_FLOAT;
					break;
				}
				case PLG_UNIFORM_DOUBLE:
				{
					if ( ndGetF64( node, &materialVariable->data.f64 ) != ND_ERROR_SUCCESS )
						break;

					materialVariable->type = MATERIAL_VAR_DOUBLE;
					break;
				}

				case PLG_UNIFORM_UINT:
				{
					if ( ndGetUI32( node, &materialVariable->data.ui32 ) != ND_ERROR_SUCCESS )
						break;

					materialVariable->type = ARL_MATERIAL_VAR_UINT;
					break;
				}
				case PLG_UNIFORM_INT:
				{
					if ( ndGetI32( node, &materialVariable->data.i32 ) != ND_ERROR_SUCCESS )
						break;

					materialVariable->type = ARL_MATERIAL_VAR_INT;
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
					if ( ndGetStr( node, texturePath, sizeof( PLPath ) ) != ND_ERROR_SUCCESS )
						break;

					if ( pl_strcasecmp( materialVariable->name, "diffuseMap" ) == 0 )
						materialVariable->hint = ARL_MATERIAL_VAR_HINT_DIFFUSE;
					else if ( pl_strcasecmp( materialVariable->name, "normalMap" ) == 0 )
						materialVariable->hint = ARL_MATERIAL_VAR_HINT_NORMAL;
					else if ( pl_strcasecmp( materialVariable->name, "specularMap" ) == 0 )
						materialVariable->hint = ARL_MATERIAL_VAR_HINT_SPECULAR;

					materialVariable->type = MATERIAL_VAR_TEXTURE;
					materialVariable->data.userPtr = arl_texture_load_direct_( texturePath, materialPass->textureFilter );
					break;
				}
			}

			if ( materialVariable->type == ARL_MATERIAL_VAR_INVALID )
			{
				PRINT_WARNING( "Invalid property type for shader variable \"%s\"!\n", propertyName );
				node = next;
				continue;
			}
		}

		if ( !validate_material_variable( materialVariable, uniformType ) )
		{
			PRINT_WARNING( "Mismatch between material variable type and uniform type!\n" );
			node = next;
			continue;
		}

		materialPass->numVariables++;

		node = next;
	}
}

void apeParseMaterialPass( struct NdBranch *root, ApeMaterialPass *materialPass )
{
	/* fetch the blend mode we'll use for the pass */
	NdBranch *subNode;
	if ( ( subNode = ndGetChildByName( root, "blendMode" ) ) != NULL )
	{
		char *blendModesArray[ 2 ];
		if ( ndGetStringArray( subNode, blendModesArray, 2 ) == ND_ERROR_SUCCESS )
		{
			materialPass->blendMode[ 0 ] = get_blend_mode_by_tag( blendModesArray[ 0 ] );
			PL_DELETE( blendModesArray[ 0 ] );
			materialPass->blendMode[ 1 ] = get_blend_mode_by_tag( blendModesArray[ 1 ] );
			PL_DELETE( blendModesArray[ 1 ] );
		}
		else
			PRINT_WARNING( "Invalid blend mode array in material!\n" );
	}
	else
	{
		materialPass->blendMode[ 0 ] = PLG_BLEND_NONE;
		materialPass->blendMode[ 1 ] = PLG_BLEND_NONE;
	}

	materialPass->depthTest = ndGetBoolByName( root, "depthTest", true );
	materialPass->cullMode = ndGetInt( root, "cullMode", materialPass->cullMode );

	const char *textureFilterPtr = ndGetStringByName( root, "textureFilterMode", NULL );
	if ( textureFilterPtr != NULL )
	{
		if ( pl_strcasecmp( textureFilterPtr, "mipmap_nearest" ) == 0 )
			materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_NEAREST;
		else if ( pl_strcasecmp( textureFilterPtr, "mipmap_linear" ) == 0 )
			materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;
		else if ( pl_strcasecmp( textureFilterPtr, "mipmap_linear_nearest" ) == 0 )
			materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR_NEAREST;
		else if ( pl_strcasecmp( textureFilterPtr, "mipmap_nearest_linear" ) == 0 )
			materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_NEAREST_LINEAR;
		else if ( pl_strcasecmp( textureFilterPtr, "nearest" ) == 0 )
			materialPass->textureFilter = PLG_TEXTURE_FILTER_NEAREST;
		else if ( pl_strcasecmp( textureFilterPtr, "linear" ) == 0 )
			materialPass->textureFilter = PLG_TEXTURE_FILTER_LINEAR;
	}
	else
		materialPass->textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;

	/* now handle any specific parameters the material provides */
	if ( ( subNode = ndGetChildByName( root, "shaderParameters" ) ) != NULL )
	{
		/* there's some extra complexity when parsing in parameters, so we'll defer that
		 * to another function */
		parse_shader_parameters( materialPass, subNode );
	}
	/* not sure whether the above section should be required yet, there might be
	 * a case where we only want to use the shader defaults? */
}

static ApeMaterial *parse_material( ApeMaterial *material, NdBranch *root, bool preview )
{
	// see if the preview texture is specified
	if ( material->preview == NULL )
	{
		material->preview = previewFallbackTexture;
		const char *previewTexture = ndGetStringByName( root, "previewTexture", NULL );
		if ( previewTexture != NULL )
		{
			material->preview = arl_texture_load_direct_( previewTexture, PLG_TEXTURE_FILTER_MIPMAP_LINEAR );
		}
	}

	// If it's just the preview we want, then stop here
	if ( preview )
	{
		return material;
	}

	/* each pass specifies how the object should be drawn before
	 * drawing it again and again for each child */
	NdBranch *node;
	if ( ( node = ndGetChildByName( root, "passes" ) ) != NULL )
	{
		node = ndGetFirstChild( node );
		while ( node != NULL )
		{
			ApeMaterialPass *currentPass = &material->passes[ material->numPasses++ ];
			/* current pass should've already been cleared by prior memset,
			 * so no need to reset the state for some crap */

			/* fetch the shader program we need to use for this pass */
			const char *programName = ndGetStringByName( node, "shaderProgram", "default" );
			ApeShaderProgramIndex *programIndex = arl_shader_get_by_name( programName );
			if ( programIndex == NULL )
			{
				currentPass->program = ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT ];
				PRINT_WARNING( "Failed to find program \"%s\", using fallback!\n", programName );
			}
			else
			{
				*currentPass = programIndex->defaultPass;
				currentPass->program = programIndex->internalPtr;
			}

			apeParseMaterialPass( node, currentPass );

			node = ndGetNextChild( node );
		}
	}

	material->surfaceType = ND_GETINT8( root, "surfaceType", 0 );
	material->enableShadows = ndGetBoolByName( root, "enableShadows", true );

	if ( material->numPasses == 0 )
	{
		PRINT_WARNING( "No passes specified for material!\n" );
	}

	material->isCached = true;

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

	return NULL;
}

static void destroy_material( ApeMaterial *material )
{
	if ( material == NULL )
	{
		return;
	}

	for ( unsigned int i = 0; i < material->numPasses; ++i )
	{
		for ( unsigned int j = 0; j < material->passes[ i ].numVariables; ++j )
		{
			switch ( material->passes[ i ].variables[ j ].type )
			{
				case MATERIAL_VAR_TEXTURE:
					//TODO: right now this is all using the plgtexture crap directly, so... waaaahh!!!
					break;
				case MATERIAL_VAR_RENDERTARGET:
					arl_render_target_release( ( ArRenderTarget * ) material->passes[ i ].variables[ j ].data.userPtr );
					break;
				default:
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

static void set_built_in_variable( PLGShaderProgram *program, int uniformSlot, int variable, unsigned int *curUnit )
{
	if ( variable == -1 )
		return;

	switch ( variable )
	{
		case APE_MATERIAL_BUILTIN_TIME:
		{
			unsigned int numTicks = apeGetNumTicks();
			PlgSetShaderUniformValueByIndex( program, uniformSlot, &numTicks, false );
			break;
		}

		case APE_MATERIAL_BUILTIN_FALLBACK:
		case APE_MATERIAL_BUILTIN_DEPTH:
		{
			PLGTexture *texture = NULL;
			if ( variable == APE_MATERIAL_BUILTIN_FALLBACK )
				texture = arl_texture_get_fallback();
#if 0//TODO
			else if ( variable == APE_MATERIAL_BUILTIN_DEPTH )
				texture = apeGetPrimaryDepthAttachment();
#endif

			if ( texture == NULL )
				break;

			PlgSetTexture( texture, *curUnit );
			PlgSetShaderUniformValueByIndex( program, uniformSlot, curUnit, false );
			( *curUnit )++;
			break;
		}

		case APE_MATERIAL_BUILTIN_VIEWPORT_SIZE:
		{
			int w, h;
			apeGet2DViewportSize( &w, &h );
			PlgSetShaderUniformValueByIndex( program, uniformSlot, &PLVector2( ( float ) w, ( float ) h ), false );
			break;
		}

		default:
			break;
	}
}

static void set_global_uniforms( PLGShaderProgram *program, const ApeLight *light )
{
	//TODO: we should be caching these slots rather than looking them up every time...

	ApeWorld *world = acl_level_get_current();

	int slot;
	if ( ( slot = PlgGetShaderUniformSlot( program, "fogColour" ) ) >= 0 )
	{
		PLColourF32 fogColour = ( light == NULL && world != NULL ) ? world->fogColour : ( PLColourF32 ){ 0.0f, 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program, slot, &fogColour, false );
	}
	if ( ( slot = PlgGetShaderUniformSlot( program, "fogNear" ) ) >= 0 )
	{
		PL_GET_CVAR( "ape/r/fogNear", fogNearVar );
		float fogNear = ( fogNearVar != NULL && fogNearVar->f_value > -1.f ) ? fogNearVar->f_value : world->fogNear;
		PlgSetShaderUniformValueByIndex( program, slot, &fogNear, false );
	}
	if ( ( slot = PlgGetShaderUniformSlot( program, "fogFar" ) ) >= 0 )
	{
		PL_GET_CVAR( "ape/r/fogFar", fogFarVar );
		float fogFar = ( fogFarVar != NULL && fogFarVar->f_value > -1.f ) ? fogFarVar->f_value : world->fogFar;
		PlgSetShaderUniformValueByIndex( program, slot, &fogFar, false );
	}

	if ( ( slot = PlgGetShaderUniformSlot( program, "light.colour" ) ) >= 0 )
	{
		PLColourF32 lightColour = ( light != NULL ) ? light->colour : ( PLColourF32 ){ 0.0f, 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program, slot, &lightColour, false );
	}
	if ( ( slot = PlgGetShaderUniformSlot( program, "light.position" ) ) >= 0 )
	{
		PLVector3 lightPosition = ( light != NULL ) ? light->position : ( PLVector3 ){ 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program, slot, &lightPosition, false );
	}
	if ( ( slot = PlgGetShaderUniformSlot( program, "light.radius" ) ) >= 0 )
	{
		float lightRadius = ( light != NULL ) ? light->radius : 0.0f;
		PlgSetShaderUniformValueByIndex( program, slot, &lightRadius, false );
	}

	if ( ( slot = PlgGetShaderUniformSlot( program, "sun.colour" ) ) >= 0 )
	{
		PLColourF32 sunColour = ( light != NULL && light->type == APE_LIGHT_TYPE_SUN ) ? light->colour : ( PLColourF32 ){ 0.0f, 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program, slot, &sunColour, false );
	}
	if ( ( slot = PlgGetShaderUniformSlot( program, "sun.position" ) ) >= 0 )
	{
		PLVector3 lightPosition = ( light != NULL && light->type == APE_LIGHT_TYPE_SUN ) ? light->position : ( PLVector3 ){ 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program, slot, &lightPosition, false );
	}
	if ( ( slot = PlgGetShaderUniformSlot( program, "sun.ambience" ) ) >= 0 )
	{
		PLColourF32 sunAmbience = ( light == NULL && world != NULL ) ? world->ambience : ( PLColourF32 ){ 0.0f, 0.0f, 0.0f, 0.0f };
		PlgSetShaderUniformValueByIndex( program, slot, &sunAmbience, false );
	}
}

ApeMaterial *apeCacheMaterial( const char *path, ApeCacheGroup group, bool useFallback, bool preview )
{
	/* check if it's already cached */
	ApeMaterial *material = get_material( path, group );
	if ( material != NULL )
	{
		// If it's not cached, and we're not asking for the preview, load the full thing
		if ( !material->isCached && !preview )
		{
			NdBranch *root = ndLoadFile( path, "material" );
			if ( root != NULL )
			{
				parse_material( material, root, false );
				ndDestroyBranch( root );
			}
			else
			{
				PRINT_WARNING( "Failed to cache material, \"%s\" (%s)!\n", path, ndGetErrorMessage() );
			}
		}
		apeAddReference( &material->mem );
		return material;
	}

	/* fallback should be optional, as in some cases we might actually care */
	ApeMaterial *fallbackPtr = useFallback ? defaultMaterials[ APE_MATERIAL_DEFAULT_FALLBACK ] : NULL;

	NdBranch *root = ndLoadFile( path, "material" );
	if ( root == NULL )
	{
		PRINT_WARNING( "Failed to load material, \"%s\" (%s)!\n", path, ndGetErrorMessage() );
		return fallbackPtr;
	}

	material = PL_NEW( ApeMaterial );
	parse_material( material, root, preview );

	ndDestroyBranch( root );

	snprintf( material->path, sizeof( material->path ), "%s", path );
	material->node = PlInsertLinkedListNode( materials[ group ], material );

	apeSetupReference( "material", APE_CACHE_POOL_MATERIALS, &material->mem, destroy_material_callback, material );
	apeAddReference( &material->mem );

	return material;
}

void ar_material_release( ApeMaterial *material )
{
	assert( material != NULL );
	if ( material == NULL )
	{
		return;
	}

	// don't flush default materials...
	for ( unsigned int i = 0; i < APE_MAX_DEFAULT_MATERIALS; ++i )
		if ( material == defaultMaterials[ i ] )
			return;

	apeReleaseReference( &material->mem );
}

int8_t ar_material_get_surface_type( const ApeMaterial *material )
{
	return material->surfaceType;
}

bool arl_material_shadows_enabled( const ApeMaterial *material ) { return material->enableShadows; }

void apeDrawMesh( ApeMaterial *material, PLGMesh *mesh, ApeLight **lights, unsigned int numLights )
{
	// If it's not had a full cache, use the fallback,
	// though ideally this shouldn't happen!
	assert( material->isCached );
	if ( !material->isCached )
		material = defaultMaterials[ APE_MATERIAL_DEFAULT_FALLBACK ];

	if ( arl_rendererState_.overrideBlendMode )
	{
		PlgEnableGraphicsState( PLG_GFX_STATE_BLEND );
		PlgSetBlendMode( arl_rendererState_.blendModeA, arl_rendererState_.blendModeB );
	}

	for ( unsigned int i = 0; i < material->numPasses; ++i )
	{
		ApeMaterialPass *curPass = &material->passes[ i ];

		// Mirror mode requires flipping the matrix,
		// so we'll need to update the cull mode
		PLGCullMode cullMode;
		if ( arl_rendererState_.cullMode == APE_RENDERER_CULL_FRONT )
			cullMode = PLG_CULL_POSITIVE;
		else if ( arl_rendererState_.cullMode == APE_RENDERER_CULL_BACK )
			cullMode = PLG_CULL_NEGATIVE;
		else if ( arl_rendererState_.cullMode == APE_RENDERER_CULL_NONE )
			cullMode = PLG_CULL_NONE;
		else
			cullMode = curPass->cullMode;

		if ( arl_rendererState_.mirror && ( arl_rendererState_.depth % 2 ) )
		{
			if ( cullMode == PLG_CULL_NEGATIVE )
				cullMode = PLG_CULL_POSITIVE;
			else if ( cullMode == PLG_CULL_POSITIVE )
				cullMode = PLG_CULL_NEGATIVE;
		}

		PlgSetCullMode( cullMode );

		//PlgDepthMask( curPass->depthTest );

		// we have an awkward check for wireframe here because we don't want to just blindly handle it globally,
		// otherwise UI elements will be wireframe too, so instead we'll just check the plg state flag
		if ( PlgIsGraphicsStateEnabled( PLG_GFX_STATE_WIREFRAME ) )
		{
			PlgSetShaderProgram( ape_defaultShaderPrograms_[ APE_SHADER_DEFAULT_VERTEX ] );
			PlgSetShaderUniformValue( curPass->program, "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );
			PlgSetTexture( NULL, 0 );
		}
		else
		{
			PlgSetShaderProgram( curPass->program );
			PlgSetShaderUniformValue( curPass->program, "pl_model", PlGetMatrix( PL_MODELVIEW_MATRIX ), false );

			if ( !arl_rendererState_.overrideBlendMode )
				PlgSetBlendMode( curPass->blendMode[ 0 ], curPass->blendMode[ 1 ] );

			set_global_uniforms( curPass->program, lights != NULL ? lights[ 0 ] : NULL );

			unsigned int curUnit = 0;
			for ( unsigned int j = 0; j < curPass->numVariables; ++j )
			{
				if ( curPass->variables[ j ].type == MATERIAL_VAR_BUILTIN )
				{
					set_built_in_variable( curPass->program, curPass->variables[ j ].programSlot, curPass->variables[ j ].data.i32, &curUnit );
					continue;
				}
				// textures just need to be set per their respective unit
				else if ( curPass->variables[ j ].type == MATERIAL_VAR_TEXTURE || curPass->variables[ j ].type == MATERIAL_VAR_RENDERTARGET )
				{
					PL_GET_CVAR( "r/skipDiffuse", skipDiffuse );
					if ( skipDiffuse != NULL && ( curPass->variables[ j ].hint == ARL_MATERIAL_VAR_HINT_DIFFUSE && skipDiffuse->b_value ) )
						continue;

					PLGTexture *texture;
					if ( curPass->variables[ j ].type == MATERIAL_VAR_RENDERTARGET )
					{
						texture = arl_render_target_get_texture( ( ArRenderTarget * ) curPass->variables[ j ].data.userPtr );
						if ( texture == NULL )
						{
							texture = arl_texture_get_fallback();
						}
					}
					else
					{
						texture = ( PLGTexture * ) curPass->variables[ j ].data.userPtr;
					}
					assert( texture != NULL );

					PL_GET_CVAR( "r/skipNormal", skipNormal );
					if ( skipNormal != NULL && ( curPass->variables[ j ].hint == ARL_MATERIAL_VAR_HINT_NORMAL && skipNormal->b_value ) )
						texture = normalFallbackTexture;
					PL_GET_CVAR( "r/skipSpecular", skipSpecular );
					if ( skipSpecular != NULL && ( curPass->variables[ j ].hint == ARL_MATERIAL_VAR_HINT_SPECULAR && skipSpecular->b_value ) )
						texture = specularFallbackTexture;

					PLGTextureFilter textureFilter = curPass->textureFilter;
					if ( texture->flags & PLG_TEXTURE_FLAG_NOMIPS )
					{
						if ( textureFilter == PLG_TEXTURE_FILTER_MIPMAP_LINEAR )
							textureFilter = PLG_TEXTURE_FILTER_LINEAR;
						else
							textureFilter = PLG_TEXTURE_FILTER_NEAREST;
					}

					PlgSetTexture( texture, curUnit );
					PlgSetTextureFilter( texture, textureFilter );

					PlgSetShaderUniformValueByIndex( curPass->program, curPass->variables[ j ].programSlot, &curUnit, false );
					curUnit++;
					continue;
				}

				PlgSetShaderUniformValueByIndex( curPass->program, curPass->variables[ j ].programSlot, &curPass->variables[ j ].data, false );
			}
		}

		PlgUploadMesh( mesh );
		PlgDrawMesh( mesh );

		ape_rendererPerformance_.numBatches++;
		if ( mesh->primitive == PLG_MESH_TRIANGLES )
			ape_rendererPerformance_.numTriangles += mesh->num_triangles;
		else
			ape_rendererPerformance_.numTriangles += ( mesh->num_verts / 2 );
	}

	PlgSetCullMode( PLG_CULL_POSITIVE );
	PlgSetBlendMode( PLG_BLEND_DISABLE );
}

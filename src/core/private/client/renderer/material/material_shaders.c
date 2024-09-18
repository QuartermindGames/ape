// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Shader management system
// Author:  Mark E. Sowden

#include <plcore/pl_hashtable.h>

#include "ape_private.h"

#include "../renderer.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static PLHashTable      *shaderProgramTable;
static ApeShaderProgram *defaultShaders[ APE_MAX_DEFAULT_SHADERS ];
static bool              isEnumeratingShaders;

#define HOT_RELOAD_TICKS_DEFAULT 60
static bool         hotReload         = false;
static unsigned int incHotReloadTicks = HOT_RELOAD_TICKS_DEFAULT;
static unsigned int hotReloadTicks    = HOT_RELOAD_TICKS_DEFAULT;

static const char *GLOBAL_UNIFORM_NAMES[ APE_SHADER_MAX_UNIFORMS ] = {
        [APE_SHADER_UNIFORM_FOG_COLOUR] = "fogColour",
        [APE_SHADER_UNIFORM_FOG_NEAR]   = "fogNear",
        [APE_SHADER_UNIFORM_FOG_FAR]    = "fogFar",

        [APE_SHADER_UNIFORM_LIGHT_COLOUR]   = "light.colour",
        [APE_SHADER_UNIFORM_LIGHT_POSITION] = "light.position",
        [APE_SHADER_UNIFORM_LIGHT_RADIUS]   = "light.radius",

        [APE_SHADER_UNIFORM_SUN_COLOUR]   = "sun.colour",
        [APE_SHADER_UNIFORM_SUN_POSITION] = "sun.position",

        [APE_SHADER_UNIFORM_AMBIENCE] = "sun.ambience",

        [APE_SHADER_UNIFORM_TEXTURE_MATRIX]    = "pl_texture",
        [APE_SHADER_UNIFORM_VIEW_MATRIX]       = "pl_view",
        [APE_SHADER_UNIFORM_PROJECTION_MATRIX] = "pl_proj",
        [APE_SHADER_UNIFORM_MODEL_MATRIX]      = "pl_model",
};

static PLGShaderStage *register_shader_stage( PLGShaderProgram *program, PLGShaderStageType type, const char *path, char definitions[][ PLG_MAX_DEFINITION_LENGTH ], unsigned int numDefinitions )
{
	PLFile *filePtr = PlOpenFile( path, true );
	if ( filePtr == NULL )
	{
		ape_warning_( "Failed to find shader \"%s\"!\nPL: %s\n", path, PlGetError() );
		return nullptr;
	}

	PLGShaderStage *stage = PlgCreateShaderStage( type );
	PlgSetShaderStageDefinitions( stage, definitions, numDefinitions );

	size_t length = PlGetFileSize( filePtr );
	char  *buffer = PL_NEW_( char, length + 1 );
	PlReadFile( filePtr, buffer, length, 1 );

	PlCloseFile( filePtr );

	// get the directory we're loading the file from,
	// so we can use include relative to the original file
	PLPath directory;
	PlSetupPath( directory, true, "%s", path );
	char *sep = strrchr( directory, '/' );
	if ( sep != NULL )
	{
		*sep = '\0';
	}

	PlgCompileShaderStage( stage, buffer, length, directory );
	if ( PlGetFunctionResult() == PL_RESULT_SUCCESS )
	{
		PlgAttachShaderStage( program, stage );
	}
	else
	{
		ape_warning_( "Failed to register stage, \"%s\"!\nPL: %s\n", path, PlGetError() );
		PlgDestroyShaderStage( stage );
		stage = nullptr;
	}

	PL_DELETE( buffer );

	return stage;
}

static ApeShaderProgram *parse_shader_program( ApeShaderProgram *program, AcmBranch *root )
{
	const char *internalName = acm_branch_get_child_string( root, "description", nullptr );
	if ( internalName == nullptr )
	{
		ape_warning_( "Shader program not assigned a valid 'description'!\n" );
		return nullptr;
	}

	if ( *program->internalName == '\0' )
	{
		snprintf( program->internalName, sizeof( program->internalName ), "%s", internalName );
		if ( ape_get_shader_by_name( program->internalName, APE_SHADER_DEFAULT_NULL ) != nullptr )
		{
			ape_warning_( "Shader program (%s) already registered!\n", program->internalName );
			return nullptr;
		}
	}
	else if ( strcmp( internalName, program->internalName ) != 0 )
	{
		ape_warning_( "Changing the internal name of an already loaded shader isn't allowed!\n" );
		return nullptr;
	}

	const char *vertexPath   = acm_branch_get_child_string( root, "vertexPath", nullptr );
	const char *fragmentPath = acm_branch_get_child_string( root, "fragmentPath", nullptr );

	if ( vertexPath == NULL || fragmentPath == NULL )
	{
		ape_warning_( "No vertex/fragment stage defined in program!\n" );
		return nullptr;
	}

	PLGShaderProgram *internal = PlgCreateShaderProgram();
	if ( internal == NULL )
	{
		ape_warning_( "Failed to create shader program!\nPL: %s\n", PlGetError() );
		return nullptr;
	}

	if ( PlResolveVirtualPath( vertexPath, program->sourcePaths[ PLG_SHADER_TYPE_VERTEX ], sizeof( program->sourcePaths[ PLG_SHADER_TYPE_VERTEX ] ) ) != nullptr )
	{
		program->sourceTimestamps[ PLG_SHADER_TYPE_VERTEX ] = PlGetLocalFileTimeStamp( program->sourcePaths[ PLG_SHADER_TYPE_VERTEX ] );
	}
	if ( PlResolveVirtualPath( fragmentPath, program->sourcePaths[ PLG_SHADER_TYPE_FRAGMENT ], sizeof( program->sourcePaths[ PLG_SHADER_TYPE_FRAGMENT ] ) ) != nullptr )
	{
		program->sourceTimestamps[ PLG_SHADER_TYPE_FRAGMENT ] = PlGetLocalFileTimeStamp( program->sourcePaths[ PLG_SHADER_TYPE_FRAGMENT ] );
	}

	/* these allow for the program to specify what
	 * definitions should be set prior to compiling
	 * the given shader. */

	char fragmentDefinitions[ PLG_MAX_DEFINITIONS ][ PLG_MAX_DEFINITION_LENGTH ];
	PL_ZERO( fragmentDefinitions, PLG_MAX_DEFINITION_LENGTH * PLG_MAX_DEFINITIONS );

	char vertexDefinitions[ PLG_MAX_DEFINITIONS ][ PLG_MAX_DEFINITION_LENGTH ];
	PL_ZERO( vertexDefinitions, PLG_MAX_DEFINITION_LENGTH * PLG_MAX_DEFINITIONS );

	unsigned int numDefinitions[ PLG_MAX_SHADER_TYPES ];
	PL_ZERO( numDefinitions, sizeof( unsigned int ) * PLG_MAX_SHADER_TYPES );

	AcmBranch *child = acm_branch_get_child_by_name( root, "definitions" );
	if ( child != NULL )
	{
		AcmBranch *subChild;
		if ( ( subChild = acm_branch_get_child_by_name( child, "fragment" ) ) != NULL )
		{
			numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] = acm_branch_get_num_of_children( subChild );
			if ( numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] > PLG_MAX_DEFINITIONS )
			{
				numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] = PLG_MAX_DEFINITIONS;
			}

			subChild = acm_branch_get_first_child( subChild );
			for ( unsigned int i = 0; i < numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ]; ++i )
			{
				if ( subChild == NULL )
				{
					ape_warning_( "Hit an invalid child, aborting early!\n" );
					numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] = i;
					break;
				}

				acm_branch_get_string( subChild, fragmentDefinitions[ i ], PLG_MAX_DEFINITION_LENGTH );
				subChild = acm_get_next_child( subChild );
			}
		}
		if ( ( subChild = acm_branch_get_child_by_name( child, "vertex" ) ) != NULL )
		{
			numDefinitions[ PLG_SHADER_TYPE_VERTEX ] = acm_branch_get_num_of_children( subChild );
			if ( numDefinitions[ PLG_SHADER_TYPE_VERTEX ] > PLG_MAX_DEFINITIONS )
			{
				numDefinitions[ PLG_SHADER_TYPE_VERTEX ] = PLG_MAX_DEFINITIONS;
			}

			subChild = acm_branch_get_first_child( subChild );
			for ( unsigned int i = 0; i < numDefinitions[ PLG_SHADER_TYPE_VERTEX ]; ++i )
			{
				if ( subChild == NULL )
				{
					ape_warning_( "Hit an invalid child, aborting early!\n" );
					numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] = i;
					break;
				}

				acm_branch_get_string( subChild, vertexDefinitions[ i ], PLG_MAX_DEFINITION_LENGTH );
				subChild = acm_get_next_child( subChild );
			}
		}
	}

	register_shader_stage( internal, PLG_SHADER_TYPE_VERTEX, vertexPath, vertexDefinitions, numDefinitions[ PLG_SHADER_TYPE_VERTEX ] );
	register_shader_stage( internal, PLG_SHADER_TYPE_FRAGMENT, fragmentPath, fragmentDefinitions, numDefinitions[ PLG_SHADER_TYPE_FRAGMENT ] );

	if ( !PlgLinkShaderProgram( internal ) )
	{
		ape_warning_( "Failed to link shader stages (%s): %s\n", program->internalName, PlGetError() );
		PlgDestroyShaderProgram( internal, true );
		return nullptr;
	}

	if ( program->internal != nullptr )
	{
		PlgDestroyShaderProgram( program->internal, true );
	}

	program->internal = internal;

	/* the default pass is an optional field that can outline
	 * the initial properties that should be used during a draw.
	 * a material can, of course, overwrite these. */
	child = acm_branch_get_child_by_name( root, "defaultPass" );
	if ( child != NULL )
	{
		// zero in-case we're reloading...
		PL_ZERO_( program->defaultPass );
		/* need to assign this for variable validation */
		program->defaultPass.program = program;

		// some sensible defaults...
		program->defaultPass.depthTest     = true;
		program->defaultPass.textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;

		ape_parse_material_pass_( child, &program->defaultPass );
#pragma message "TODO: materials won't automatically inherit these default changes yet..."
	}

	// now lookup all the default uniforms
	for ( uint i = 0; i < APE_SHADER_MAX_UNIFORMS; ++i )
	{
		program->globalUniforms[ i ] = PlgGetShaderUniformSlot( program->internal, GLOBAL_UNIFORM_NAMES[ i ] );
		if ( program->globalUniforms[ i ] < 0 )
		{
			PRINT_DEBUG( "Didn't find global uniform (%s) per shader program (%s).\n", GLOBAL_UNIFORM_NAMES[ i ], program->internalName );
		}
	}

	return program;
}

static void destroy_shader( void *user )
{
	ApeShaderProgram *programIndex = ( ApeShaderProgram * ) user;
	assert( programIndex != nullptr );
	PlgDestroyShaderProgram( programIndex->internal, true );
	PL_DELETE( programIndex );
}

static void load_shader_program_callback( const char *path, PL_UNUSED void *userData )
{
	ape_print_( "Loading program: \"%s\"\n", path );

	AcmBranch *root = acm_load_file( path, "program" );
	if ( root == NULL )
	{
		ape_warning_( "Failed to load shader program \"%s\"!\nPL: %s\n", path, PlGetError() );
		return;
	}

	ApeShaderProgram *program = PL_NEW( ApeShaderProgram );
	if ( parse_shader_program( program, root ) == nullptr )
	{
		ape_warning_( "Failed to parse shader program (%s)!\n", path );
		destroy_shader( program );
		program = nullptr;
	}

	acm_branch_destroy( root );

	if ( program == NULL )
	{
		return;
	}

	if ( PlResolveVirtualPath( path, program->path, sizeof( program->path ) ) != nullptr )
	{
		program->timestamp = PlGetLocalFileTimeStamp( program->path );
	}
	else
	{
		// the only negative outcome from this is that hot-reloading won't work, so just warn...
		ape_warning_( "Failed to resolve virtual path for shader program (%s): %s\n", program->internalName, PlGetError() );
	}

	PlInsertHashTableNode( shaderProgramTable, program->internalName, strlen( program->internalName ), program );
}

static void reload_shader_program( ApeShaderProgram *program )
{
	AcmBranch *root = acm_load_file( program->path, "program" );
	if ( root == nullptr )
	{
		ape_warning_( "Failed to reload shader program (%s): %s\n", program->internalName, acm_get_error_message() );
		return;
	}

	if ( parse_shader_program( program, root ) == nullptr )
	{
		ape_warning_( "Failed to parse shader program (%s) for reload!\n", program->path );
	}

	acm_branch_destroy( root );

	program->timestamp = PlGetLocalFileTimeStamp( program->path );
}

static void reload_shader_program_command( unsigned int argc, char **argv )
{
	if ( argc > 1 )
	{
		ApeShaderProgram *program = ape_get_shader_by_name( argv[ 1 ], APE_SHADER_DEFAULT_NULL );
		if ( program == nullptr )
		{
			ape_warning_( "Failed to find existing shader (%s)!\n", argv[ 1 ] );
			return;
		}

		reload_shader_program( program );
		return;
	}

	PLHashTableNode *node = PlGetFirstHashTableNode( shaderProgramTable );
	while ( node != nullptr )
	{
		ApeShaderProgram *program = PlGetHashTableNodeUserData( node );
		assert( program != nullptr );

		reload_shader_program( program );

		node = PlGetNextHashTableNode( node );
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

ApeShaderProgram *ape_get_shader_by_name( const char *name, ApeDefaultShaderProgram fallback )
{
	ApeShaderProgram *program = ( ApeShaderProgram * ) PlLookupHashTableUserData( shaderProgramTable, name, strlen( name ) );
	if ( program != nullptr )
	{
		return program;
	}

	if ( fallback == APE_SHADER_DEFAULT_NULL )
	{
		//HACK: silence this if we're setting things up...
		if ( !isEnumeratingShaders )
		{
			ape_warning_( "Failed to fetch shader (%s) by name!\n", name );
		}
		return nullptr;
	}

	program = defaultShaders[ fallback ];
	assert( program != nullptr );

	ape_warning_( "Failed to fetch shader (%s) by name! Using fallback (%s)\n", program->internalName );

	return program;
}

ApeShaderProgram *ape_get_default_shader( ApeDefaultShaderProgram defaultShaderProgram )
{
	return defaultShaders[ defaultShaderProgram ];
}

void ape_register_shader_console_variables_()
{
	PlRegisterConsoleVariable( "shaders.autoHotReload", "Enable automatic reload of shaders.",
#if !defined( NDEBUG )
	                           "true",
#else
	                           "false",
#endif
	                           PL_VAR_BOOL, &hotReload, nullptr, true );
	PlRegisterConsoleVariable( "shaders.hotReloadDelay", "Delay before attempting to reload shaders.", PL_TOSTRING( HOT_RELOAD_TICKS_DEFAULT ), PL_VAR_I32, &incHotReloadTicks, nullptr, true );

	PlRegisterConsoleCommand( "reload_shaders", "Reload shader programs.", -1, reload_shader_program_command );
}

void ape_hot_reload_shaders_()
{
	if ( !hotReload || hotReloadTicks > ape_get_num_ticks() )
	{
		return;
	}

	isEnumeratingShaders = true;

	PLHashTableNode *node = PlGetFirstHashTableNode( shaderProgramTable );
	while ( node != nullptr )
	{
		ApeShaderProgram *program = PlGetHashTableNodeUserData( node );
		assert( program != nullptr );

		bool reload = false;

		// first attempt to check the original program file to see if anything changed...
		time_t timestamp = PlGetLocalFileTimeStamp( program->path );
		if ( timestamp != 0 && timestamp != program->timestamp )
		{
			reload = true;
		}

		// now check the other files
		if ( !reload )
		{
			for ( unsigned int i = 0; i < PLG_MAX_SHADER_TYPES; ++i )
			{
				const char *path = program->sourcePaths[ i ];
				if ( *path == '\0' )
				{
					continue;
				}

				timestamp = PlGetLocalFileTimeStamp( path );
				if ( timestamp == 0 || timestamp == program->sourceTimestamps[ i ] )
				{
					continue;
				}

				reload = true;
			}
		}

		if ( reload )
		{
			ape_print_( "Reloading shader program (%s)\n", program->internalName );
			reload_shader_program( program );
		}

		node = PlGetNextHashTableNode( node );
	}

	hotReloadTicks = ape_get_num_ticks() + incHotReloadTicks;

	isEnumeratingShaders = false;
}

void ape_initialize_shaders_( void )
{
	shaderProgramTable = PlCreateHashTable();
	if ( shaderProgramTable == NULL )
	{
		ape_error_( true, "Failed to create shader program list: %s\n", PlGetError() );
	}

	isEnumeratingShaders = true;

	ape_print_( "Scanning for shader programs...\n" );
	PlScanDirectory( "materials/shaders", "n", load_shader_program_callback, false, NULL );

	isEnumeratingShaders = false;

	ape_print_( "%d shader programs indexed\n", PlGetNumHashTableNodes( shaderProgramTable ) );

	// now fetch the default programs
	static const char *defaultShaderNames[ APE_MAX_DEFAULT_SHADERS ] = {
	        [APE_SHADER_DEFAULT]        = "default",
	        [APE_SHADER_DEFAULT_VERTEX] = "default_vertex",
	        [APE_SHADER_DEFAULT_ALPHA]  = "default_alpha",
	        [APE_SHADER_DEFAULT_FONT]   = "font",
	        [APE_SHADER_DEFAULT_SHADOW] = "shadow",
	};
	for ( unsigned int i = 0; i < APE_MAX_DEFAULT_SHADERS; ++i )
	{
		ApeShaderProgram *program = ape_get_shader_by_name( defaultShaderNames[ i ], APE_SHADER_DEFAULT_NULL );
		if ( program == nullptr )
		{
			ape_error_( true, "Failed to find default shader program, \"%s\"!\n", defaultShaderNames[ i ] );
		}

		defaultShaders[ i ] = program;
	}
}

void ape_shutdown_shaders_()
{
	PlDestroyHashTableEx( shaderProgramTable, destroy_shader );
}

void ape_set_active_shader_by_default_( ApeDefaultShaderProgram defaultShaderProgram )
{
	PlgSetShaderProgram( defaultShaders[ defaultShaderProgram ]->internal );
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_shader_set_active_( ApeShaderProgram *self )
{
	PlgSetShaderProgram( self->internal );
}

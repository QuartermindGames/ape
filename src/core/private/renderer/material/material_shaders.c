// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Shader management system
// Author:  Mark E. Sowden

#include <plcore/pl_hashtable.h>

#include "ape_private.h"
#include "material.h"

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
        [APE_SHADER_UNIFORM_NUM_TICKS] = "u_numTicks",
        [APE_SHADER_UNIFORM_VIEW_SIZE] = "u_viewSize",

        [APE_SHADER_UNIFORM_FOG_COLOUR] = "fogColour",
        [APE_SHADER_UNIFORM_FOG_NEAR]   = "fogNear",
        [APE_SHADER_UNIFORM_FOG_FAR]    = "fogFar",

        [APE_SHADER_UNIFORM_LIGHT_COLOUR]    = "lighting.colour",
        [APE_SHADER_UNIFORM_LIGHT_DIRECTION] = "lighting.dir",

        [APE_SHADER_UNIFORM_TEXTURE_MATRIX]    = "pl_texture",
        [APE_SHADER_UNIFORM_VIEW_MATRIX]       = "pl_view",
        [APE_SHADER_UNIFORM_PROJECTION_MATRIX] = "pl_proj",
        [APE_SHADER_UNIFORM_MODEL_MATRIX]      = "pl_model",
};

static QmGfxShaderStage *register_shader_stage( QmGfxShaderProgram *program, QmGfxShaderStageType type, const char *path, char definitions[][ PLG_MAX_DEFINITION_LENGTH ], unsigned int numDefinitions )
{
	QmFsFile *filePtr = qm_fs_file_open( path, true );
	if ( filePtr == NULL )
	{
		ape_console_warning_( "Failed to find shader \"%s\"!\nPL: %s\n", path, PlGetError() );
		return nullptr;
	}

	QmGfxShaderStage *stage = qm_gfx_shader_stage_create( type );
	qm_gfx_shader_stage_set_definitions( stage, definitions, numDefinitions );

	size_t length = qm_fs_file_get_size( filePtr );
	char  *buffer = QM_OS_MEMORY_NEW_( char, length + 1 );
	qm_file_read( filePtr, buffer, length, 1 );

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

	if ( qm_gfx_shader_stage_compile( stage, buffer, length, directory ) )
	{
		qm_gfx_shader_program_attach_stage( program, stage );
	}
	else
	{
		ape_console_warning_( "Failed to register stage, \"%s\"!\nPL: %s\n", path, PlGetError() );
		qm_os_memory_free( stage );
		stage = nullptr;
	}

	qm_os_memory_free( buffer );

	return stage;
}

static void check_shader_capability( ApeShaderProgram *self, const char *definition )
{
	if ( strcmp( definition, "LIGHTING" ) == 0 )
	{
		self->flags |= APE_SHADER_PROGRAM_FLAG_SUPPORTS_LIGHTING;
	}
	else if ( strcmp( definition, "LIGHTMAP" ) == 0 )
	{
		self->flags |= APE_SHADER_PROGRAM_FLAG_SUPPORTS_LIGHTMAP;
	}
}

static ApeShaderProgram *parse_shader_program( ApeShaderProgram *program, AcmBranch *root )
{
	const char *internalName = acm_get_string( root, "description", nullptr );
	if ( internalName == nullptr )
	{
		ape_console_warning_( "Shader program not assigned a valid 'description'!\n" );
		return nullptr;
	}

	if ( *program->internalName == '\0' )
	{
		snprintf( program->internalName, sizeof( program->internalName ), "%s", internalName );
		if ( ape_get_shader_by_name( program->internalName, APE_SHADER_DEFAULT_NULL ) != nullptr )
		{
			ape_console_warning_( "Shader program (%s) already registered!\n", program->internalName );
			return nullptr;
		}
	}
	else if ( strcmp( internalName, program->internalName ) != 0 )
	{
		ape_console_warning_( "Changing the internal name of an already loaded shader isn't allowed!\n" );
		return nullptr;
	}

	const char *vertexPath   = acm_get_string( root, "vertexPath", nullptr );
	const char *fragmentPath = acm_get_string( root, "fragmentPath", nullptr );

	if ( vertexPath == NULL || fragmentPath == NULL )
	{
		ape_console_warning_( "No vertex/fragment stage defined in program!\n" );
		return nullptr;
	}

	QmGfxShaderProgram *internal = qm_gfx_shader_program_create();
	if ( internal == NULL )
	{
		ape_console_warning_( "Failed to create shader program!\nPL: %s\n", PlGetError() );
		return nullptr;
	}

	if ( qm_fs_resolve_virtual_path( vertexPath, program->sourcePaths[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ], sizeof( program->sourcePaths[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ] ) ) != nullptr )
	{
		program->sourceTimestamps[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ] = qm_fs_get_local_file_timestamp( program->sourcePaths[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ] );
	}
	if ( qm_fs_resolve_virtual_path( fragmentPath, program->sourcePaths[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ], sizeof( program->sourcePaths[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] ) ) != nullptr )
	{
		program->sourceTimestamps[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] = qm_fs_get_local_file_timestamp( program->sourcePaths[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] );
	}

	/* these allow for the program to specify what
	 * definitions should be set prior to compiling
	 * the given shader. */

	char fragmentDefinitions[ PLG_MAX_DEFINITIONS ][ PLG_MAX_DEFINITION_LENGTH ];
	QM_OS_ZERO( fragmentDefinitions, PLG_MAX_DEFINITION_LENGTH * PLG_MAX_DEFINITIONS );

	char vertexDefinitions[ PLG_MAX_DEFINITIONS ][ PLG_MAX_DEFINITION_LENGTH ];
	QM_OS_ZERO( vertexDefinitions, PLG_MAX_DEFINITION_LENGTH * PLG_MAX_DEFINITIONS );

	unsigned int numDefinitions[ QM_GFX_MAX_SHADER_STAGE_TYPES ];
	QM_OS_ZERO( numDefinitions, sizeof( unsigned int ) * QM_GFX_MAX_SHADER_STAGE_TYPES );

	AcmBranch *child = acm_get_child_by_name( root, "definitions" );
	if ( child != NULL )
	{
		AcmBranch *subChild;
		if ( ( subChild = acm_get_child_by_name( child, "fragment" ) ) != NULL )
		{
			numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] = acm_get_num_of_children( subChild );
			if ( numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] > PLG_MAX_DEFINITIONS )
			{
				numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] = PLG_MAX_DEFINITIONS;
			}

			subChild = acm_get_first_child( subChild );
			for ( unsigned int i = 0; i < numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ]; ++i )
			{
				if ( subChild == NULL )
				{
					ape_console_warning_( "Hit an invalid child, aborting early!\n" );
					numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] = i;
					break;
				}

				acm_branch_get_string( subChild, fragmentDefinitions[ i ], PLG_MAX_DEFINITION_LENGTH );
				check_shader_capability( program, fragmentDefinitions[ i ] );

				subChild = acm_get_next_child( subChild );
			}
		}
		if ( ( subChild = acm_get_child_by_name( child, "vertex" ) ) != NULL )
		{
			numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ] = acm_get_num_of_children( subChild );
			if ( numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ] > PLG_MAX_DEFINITIONS )
			{
				numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ] = PLG_MAX_DEFINITIONS;
			}

			subChild = acm_get_first_child( subChild );
			for ( unsigned int i = 0; i < numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ]; ++i )
			{
				if ( subChild == NULL )
				{
					ape_console_warning_( "Hit an invalid child, aborting early!\n" );
					numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] = i;
					break;
				}

				acm_branch_get_string( subChild, vertexDefinitions[ i ], PLG_MAX_DEFINITION_LENGTH );
				check_shader_capability( program, vertexDefinitions[ i ] );

				subChild = acm_get_next_child( subChild );
			}
		}
	}

	register_shader_stage( internal, QM_GFX_SHADER_STAGE_TYPE_VERTEX, vertexPath, vertexDefinitions, numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_VERTEX ] );
	register_shader_stage( internal, QM_GFX_SHADER_STAGE_TYPE_FRAGMENT, fragmentPath, fragmentDefinitions, numDefinitions[ QM_GFX_SHADER_STAGE_TYPE_FRAGMENT ] );

	if ( !qm_gfx_shader_program_link( internal ) )
	{
		ape_console_warning_( "Failed to link shader stages (%s): %s\n", program->internalName, PlGetError() );
		qm_os_memory_free( internal );
		return nullptr;
	}

	if ( program->internal != nullptr )
	{
		qm_os_memory_free( program->internal );
	}

	program->internal = internal;

	/* the default pass is an optional field that can outline
	 * the initial properties that should be used during a draw.
	 * a material can, of course, overwrite these. */
	child = acm_get_child_by_name( root, "defaultPass" );
	if ( child != NULL )
	{
		// zero in-case we're reloading...
		QM_OS_ZERO_( program->defaultPass );
		/* need to assign this for variable validation */
		program->defaultPass.program = program;

		// some sensible defaults...
		program->defaultPass.depthMask     = true;
		program->defaultPass.textureFilter = PLG_TEXTURE_FILTER_MIPMAP_LINEAR;

		ape_parse_material_pass_( nullptr, child, &program->defaultPass );
		//TODO: materials won't automatically inherit these default changes yet...
	}

	// now lookup all the default uniforms
	for ( unsigned int i = 0; i < APE_SHADER_MAX_UNIFORMS; ++i )
	{
		program->globalUniforms[ i ] = qm_gfx_shader_program_get_uniform_slot( program->internal, GLOBAL_UNIFORM_NAMES[ i ] );
		if ( program->globalUniforms[ i ] < 0 )
		{
			ape_console_verbose_( "Didn't find global uniform (%s) per shader program (%s).\n", GLOBAL_UNIFORM_NAMES[ i ], program->internalName );
		}
	}

	return program;
}

static void destroy_shader( void *user )
{
	ApeShaderProgram *programIndex = user;
	assert( programIndex != nullptr );
	qm_os_memory_free( programIndex->internal );
	qm_os_memory_free( programIndex );
}

static void load_shader_program_callback( const char *path, [[maybe_unused]] void *userData )
{
	ape_console_print_( "Loading program: \"%s\"\n", path );

	AcmBranch *root = com_acm_load_file( path, "program" );
	if ( root == NULL )
	{
		ape_console_warning_( "Failed to load shader program \"%s\"!\nPL: %s\n", path, PlGetError() );
		return;
	}

	ApeShaderProgram *program = QM_OS_MEMORY_NEW( ApeShaderProgram );
	if ( parse_shader_program( program, root ) == nullptr )
	{
		ape_console_warning_( "Failed to parse shader program (%s)!\n", path );
		destroy_shader( program );
		program = nullptr;
	}

	acm_branch_destroy( root );

	if ( program == NULL )
	{
		return;
	}

	if ( qm_fs_resolve_virtual_path( path, program->path, sizeof( program->path ) ) != nullptr )
	{
		program->timestamp = qm_fs_get_local_file_timestamp( program->path );
	}
	else
	{
		// the only negative outcome from this is that hot-reloading won't work, so just warn...
		ape_console_warning_( "Failed to resolve virtual path for shader program (%s): %s\n", program->internalName, PlGetError() );
	}

	PlInsertHashTableNode( shaderProgramTable, program->internalName, strlen( program->internalName ), program );
}

static void reload_shader_program( ApeShaderProgram *program )
{
	AcmBranch *root = com_acm_load_file( program->path, "program" );
	if ( root == nullptr )
	{
		ape_console_warning_( "Failed to reload shader program (%s): %s\n", program->internalName, acm_get_error_message() );
		return;
	}

	if ( parse_shader_program( program, root ) == nullptr )
	{
		ape_console_warning_( "Failed to parse shader program (%s) for reload!\n", program->path );
	}

	acm_branch_destroy( root );

	program->timestamp = qm_fs_get_local_file_timestamp( program->path );
}

static void reload_shader_program_command( unsigned int argc, char **argv )
{
	if ( argc > 1 )
	{
		ApeShaderProgram *program = ape_get_shader_by_name( argv[ 1 ], APE_SHADER_DEFAULT_NULL );
		if ( program == nullptr )
		{
			ape_console_warning_( "Failed to find existing shader (%s)!\n", argv[ 1 ] );
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
			ape_console_warning_( "Failed to fetch shader (%s) by name!\n", name );
		}
		return nullptr;
	}

	program = defaultShaders[ fallback ];
	assert( program != nullptr );

	ape_console_warning_( "Failed to fetch shader (%s) by name! Using fallback (%s)\n", program->internalName );

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
	PlRegisterConsoleVariable( "shaders.hotReloadDelay", "Delay before attempting to reload shaders.", QM_OS_TO_STRING( HOT_RELOAD_TICKS_DEFAULT ), PL_VAR_I32, &incHotReloadTicks, nullptr, true );

	PlRegisterConsoleCommand( "reload_shaders", "Reload shader programs.", -1, reload_shader_program_command );
}

void ape_material_shaders_check_hot_reload_()
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
		time_t timestamp = qm_fs_get_local_file_timestamp( program->path );
		if ( timestamp != 0 && timestamp != program->timestamp )
		{
			reload = true;
		}

		// now check the other files
		if ( !reload )
		{
			for ( unsigned int i = 0; i < QM_GFX_MAX_SHADER_STAGE_TYPES; ++i )
			{
				const char *path = program->sourcePaths[ i ];
				if ( *path == '\0' )
				{
					continue;
				}

				timestamp = qm_fs_get_local_file_timestamp( path );
				if ( timestamp == 0 || timestamp == program->sourceTimestamps[ i ] )
				{
					continue;
				}

				reload = true;
			}
		}

		if ( reload )
		{
			ape_console_print_( "Reloading shader program (%s)\n", program->internalName );
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
		ape_console_error_( true, "Failed to create shader program list: %s\n", PlGetError() );
	}

	isEnumeratingShaders = true;

	ape_console_print_( "Scanning for shader programs...\n" );
	PlScanDirectory( "materials/shaders", "n", load_shader_program_callback, true, NULL );

	isEnumeratingShaders = false;

	ape_console_print_( "%d shader programs indexed\n", PlGetNumHashTableNodes( shaderProgramTable ) );

	// now fetch the default programs
	static const char *defaultShaderNames[ APE_MAX_DEFAULT_SHADERS ] = {
	        [APE_SHADER_DEFAULT]        = "default",
	        [APE_SHADER_DEFAULT_VERTEX] = "default_vertex",
	        [APE_SHADER_DEFAULT_ALPHA]  = "default_alpha",
	        [APE_SHADER_DEFAULT_FONT]   = "font",
	        [APE_SHADER_DEFAULT_SHADOW] = "shadow",
	        [APE_SHADER_DEFAULT_GRID]   = "editor_grid",
	};
	for ( unsigned int i = 0; i < APE_MAX_DEFAULT_SHADERS; ++i )
	{
		ApeShaderProgram *program = ape_get_shader_by_name( defaultShaderNames[ i ], APE_SHADER_DEFAULT_NULL );
		if ( program == nullptr )
		{
			ape_console_error_( true, "Failed to find default shader program, \"%s\"!\n", defaultShaderNames[ i ] );
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

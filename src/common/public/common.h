// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#define SS_COM_COPYRIGHT "Copyright © 2020-2024 SnortySoft, Mark E Sowden"

typedef enum ComDataType
{
	COM_DATATYPE_BOOL,

	COM_DATATYPE_INT8,
	COM_DATATYPE_INT16,
	COM_DATATYPE_INT32,

	COM_DATATYPE_UINT8,
	COM_DATATYPE_UINT16,
	COM_DATATYPE_UINT32,

	COM_DATATYPE_FLOAT32,
	COM_DATATYPE_FLOAT64,

	COM_DATATYPE_POINTER,

	COM_MAX_DATATYPES
} ComDataType;

PL_EXTERN_C

void com_initialize( void );
const char *com_get_local_data_directory( void );
const char *com_get_app_data_directory( void );
struct NdBranch *com_get_config( const char *name );// attempts to fetch the specified config, otherwise returns an empty config
bool com_write_config( struct NdBranch *root, const char *name );

void com_pkg_write_header( FILE *pack, unsigned int numFiles );
void com_pkg_add_data( FILE *pack, const char *path, const void *buf, size_t size );

/////////////////////////////////////////////////////////////////
// PROFILER

typedef struct ComProfilingGroup ComProfilingGroup;

ComProfilingGroup *comGetProfilingGroup( const char *key );

void comStartProfiling( const char *key );
void comEndProfiling( const char *key );

const char *comGetProfilingGroupName( const ComProfilingGroup *group );

ComProfilingGroup *comGetFirstProfilingGroup( void );
ComProfilingGroup *comGetNextProfilingGroup( ComProfilingGroup *group );

double comGetProfilingGroupTimeTaken( const ComProfilingGroup *group );
double comGetProfilingGroupTimeTakenAverage( const ComProfilingGroup *group );
const double *comGetProfilerGroupSamples( const ComProfilingGroup *group, unsigned int *numPoints );

unsigned int comGetNumProfilerGroups( void );

void com_update_profiler_samples( void );

#define COM_PROFILE_FUNCTION_START() comStartProfiling( PL_FUNCTION )
#define COM_PROFILE_FUNCTION_END()   comEndProfiling( PL_FUNCTION )
#define COM_PROFILE_FUNCTION_CALL( FUNCTION_NAME, FUNCTION ) \
	{                                                        \
		comStartProfiling( FUNCTION_NAME );                  \
		FUNCTION;                                            \
		comEndProfiling( FUNCTION_NAME );                    \
	}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

PL_EXTERN_C_END

// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#define SS_COM_COPYRIGHT "Copyright © 2020-2024 SnortySoft, Mark E Sowden"

// These are non-standard, so declare them here
#if !defined( _POSIX_SOURCE )
#	define _POSIX_SOURCE 1
#endif
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

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

void              com_initialize( void );
const char       *com_get_local_data_directory( void );
const char       *com_get_app_data_directory( void );
struct AcmBranch *com_get_config( const char *name );// attempts to fetch the specified config, otherwise returns an empty config
bool              com_write_config( struct AcmBranch *root, const char *name );

void com_pkg_write_header( FILE *pack, uint numFiles );
void com_pkg_add_data( FILE *pack, const char *path, const void *buf, size_t size );

/////////////////////////////////////////////////////////////////
// PROFILER

typedef struct ComProfilingGroup ComProfilingGroup;

ComProfilingGroup *com_profiler_get_group( const char *key );

void comStartProfiling( const char *key );
void comEndProfiling( const char *key );

const char *com_profiler_get_group_name( const ComProfilingGroup *group );

ComProfilingGroup *com_profiler_get_first_group( void );
ComProfilingGroup *com_profiler_get_next_group( ComProfilingGroup *group );

double        com_profiler_get_time_taken( const ComProfilingGroup *group );
double        com_profiler_get_time_average( const ComProfilingGroup *group );
const double *com_profiler_get_samples( const ComProfilingGroup *group, uint *numPoints );

uint com_profiler_get_num_groups( void );

void com_profiler_update_samples( void );

#define COM_PROFILE_START( NAME ) comStartProfiling( ( NAME ) )
#define COM_PROFILE_END( NAME )   comEndProfiling( ( NAME ) )

#define COM_PROFILE_FUNCTION_START() comStartProfiling( PL_FUNCTION )
#define COM_PROFILE_FUNCTION_END()   comEndProfiling( PL_FUNCTION )
#define COM_PROFILE_FUNCTION_CALL( FUNCTION ) \
	{                                         \
		comStartProfiling( #FUNCTION );       \
		FUNCTION;                             \
		comEndProfiling( #FUNCTION );         \
	}

// Wrapper for Hei macro to take advantage of C23 features
#define COM_ITERATE_LINKED_LIST( VAR, LIST, ITR ) PL_ITERATE_LINKED_LIST( VAR, typeof( *( VAR ) ), LIST, ITR )

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

/**
 * @brief Determines if a given set of vertices form a convex polygon.
 *
 * This function checks whether the vertices provided form a convex polygon by examining the cross products
 * of edges extending from consecutive vertices. If all cross products have the same sign, the polygon is convex,
 * otherwise it is not.
 *
 * @param vertices 		The array of vertices representing the polygon.
 * @param numVertices 	The number of vertices in the polygon.
 * @return 				true if the polygon is convex, false otherwise.
 */
bool com_math_is_polygon_convex( const PLVector2 *vertices, uint numVertices );

/**
 * Computes the face normal of a polygon composed of multiple triangles.
 *
 * @param vertices		A pointer to an array of PLVector3 structures representing the vertices of the polygon.
 * @param numVertices 	The number of vertices in the polygon. This should be a multiple of 3, as each face is a triangle.
 * @return 				A PLVector3 structure representing the normalized face normal vector of the polygon.
 *
 * This function calculates the normal vector for a polygon by considering it as a collection of
 * triangles. Each set of three vertices is treated as a triangle. The function computes the cross
 * product of vectors formed by these vertices to determine the normal for each triangle, sums these
 * normal vectors, and finally normalizes the resulting vector to generate the face normal.
 */
PLVector3 com_math_compute_face_normal( const PLVector3 *vertices, unsigned int numVertices );

PL_EXTERN_C_END

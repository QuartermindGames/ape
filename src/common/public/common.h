// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#define COM_COPYRIGHT "Copyright © 2020-2024 Quartermind Games, Mark E Sowden"

// These are non-standard, so declare them here
#if !defined( _POSIX_SOURCE )
#	define _POSIX_SOURCE 1
#endif
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;

// new types to replace the above,
// follows our convention and less prone to conflict
typedef unsigned char  Byte;
typedef unsigned char  UChar;
typedef unsigned short UShort;
typedef unsigned int   UInt;

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

/**
 * @brief Initializes common library components and settings.
 *
 * This function sets up logging levels, registers packages, and initializes
 * necessary directories for the application's operation. It prepares the
 * common library for use by setting up various subsystems like logging and
 * directory lookups essential for the application's functionality.
 */
void com_initialize( void );

/**
 * @brief Retrieves the local data directory path for the application.
 *
 * This function determines the local data directory path by analyzing the
 * executable directory and sets the path accordingly. It attempts to resolve
 * the directory using relative paths from the executable directory or the
 * current working directory. On Unix systems, it further resolves the path
 * using realpath to obtain the canonicalized absolute pathname.
 *
 * @return A const char pointer to the resolved local data directory path.
 */
const char *com_get_local_data_directory( void );

/**
 * @brief Retrieves the application data directory path.
 *
 * This function returns the path to the application's data directory,
 * where application-specific data is stored. It attempts to fetch the
 * directory path using system functions, and if unsuccessful, defaults
 * to the current directory. The result is cached for subsequent calls.
 *
 * @return A pointer to a string containing the application's data directory path.
 */
const char *com_get_app_data_directory( void );

struct AcmBranch *com_get_config( const char *name );// attempts to fetch the specified config, otherwise returns an empty config
bool              com_write_config( struct AcmBranch *root, const char *name );

void com_pkg_write_header( FILE *pack, uint numFiles );
void com_pkg_add_data( FILE *pack, const char *path, const void *buf, size_t size );

/////////////////////////////////////////////////////////////////
// PROFILER

typedef struct ComProfilingGroup ComProfilingGroup;

/**
 * @brief Retrieves a profiling group based on the given key.
 *
 * This function searches for an existing profiling group associated with the specified key.
 * If the profiling group is found, it is returned. If no such group exists, the function returns NULL.
 * Profiling groups are part of a system for monitoring performance metrics.
 *
 * @param key	The key identifying the desired profiling group.
 * @return		A pointer to the ComProfilingGroup if found, or NULL if no group exists for the provided key.
 */
ComProfilingGroup *com_profiler_get_group( const char *key );

void com_profiler_start( const char *key );
void com_profiler_end( const char *key );

/**
 * @brief Retrieves the name of a profiling group.
 *
 * This function returns the name of the profiling group by accessing the key associated
 * with the given ComProfilingGroup structure. The key is intended to serve as a unique
 * identifier for the group.
 *
 * @param group	A pointer to the ComProfilingGroup structure from which to retrieve the name.
 * @return		A pointer to a constant character string representing the name of the profiling group.
 */
const char *com_profiler_get_group_name( const ComProfilingGroup *group );

/**
 * @brief Retrieves the first profiling group in the sequence.
 *
 * This function returns a pointer to the first profiling group within the
 * profiling groups hash table. If the profiling group hash table is empty
 * or uninitialized, the function returns NULL.
 *
 * @return A pointer to the first ComProfilingGroup if available, or NULL if
 * the profiling groups list is empty or unavailable.
 */
ComProfilingGroup *com_profiler_get_first_group( void );

/**
 * @brief Retrieves the next profiling group in the sequence.
 *
 * This function returns the next profiling group following the provided group in the sequence of profiling groups.
 * If there are no more groups, the function returns NULL.
 * Profiling groups are used to collect and analyze performance metrics.
 *
 * @param group   A pointer to the current ComProfilingGroup from which the next group is retrieved.
 * @return        A pointer to the next ComProfilingGroup, or NULL if there are no more groups.
 */
ComProfilingGroup *com_profiler_get_next_group( ComProfilingGroup *group );

double        com_profiler_get_time_taken( const ComProfilingGroup *group );
double        com_profiler_get_time_average( const ComProfilingGroup *group );
const double *com_profiler_get_samples( const ComProfilingGroup *group, uint *numPoints );

/**
 * @brief Returns the number of existing profiling groups.
 *
 * This function calculates and returns the total number of profiling groups currently available.
 * It checks if the internal hash table for profiling groups has been initialized. If it has not
 * been initialized, the function returns 0, indicating no groups are present.
 *
 * @return The total number of profiling groups, or 0 if no groups exist.
 */
uint com_profiler_get_num_groups( void );

/**
 * @brief Updates the profiling samples for each profiling group.
 *
 * This function iterates over all profiling groups and updates their sample data.
 * It shifts existing sample results within the group and assigns the latest time taken
 * to the last sample position. The operation helps in maintaining a rolling history
 * of time taken for profiling analyses.
 * The function performs no operation if no profiling groups are initialized.
 */
void com_profiler_update_samples( void );

#define COM_ENABLE_PROFILER

#if defined( COM_ENABLE_PROFILER )
#	define COM_PROFILE_START( NAME ) com_profiler_start( ( NAME ) )
#	define COM_PROFILE_END( NAME )   com_profiler_end( ( NAME ) )

#	define COM_PROFILE_FUNCTION_START() com_profiler_start( PL_FUNCTION )
#	define COM_PROFILE_FUNCTION_END()   com_profiler_end( PL_FUNCTION )
#else
#	define COM_PROFILE_START( NAME )
#	define COM_PROFILE_END( NAME )
#	define COM_PROFILE_FUNCTION_START()
#	define COM_PROFILE_FUNCTION_END()
#endif

// Wrappers for Hei macros to take advantage of C23 features
#define COM_ITERATE_LINKED_LIST( VAR, LIST, ITR ) PL_ITERATE_LINKED_LIST( VAR, typeof( *( VAR ) ), LIST, ITR )
#define COM_ITERATE_HASHED_LIST( VAR, LIST, ITR ) PL_ITERATE_HASHED_LIST( VAR, typeof( *( VAR ) ), LIST, ITR )

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

typedef struct PLCollisionAABB  PLCollisionAABB;
typedef struct PLCollisionRay   PLCollisionRay;
typedef struct PLCollisionPlane PLCollisionPlane;

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

bool com_math_ray_intersect_aabb( const PLCollisionRay *ray, const PLCollisionAABB *aabb, PLVector3 *result );
bool com_math_ray_intersect_plane( const PLCollisionRay *ray, const PLCollisionPlane *plane, PLVector3 *result );
bool com_math_ray_intersect_polygon( const PLCollisionRay *ray, const PLVector3 *vertices, uint numVertices, PLVector3 *result );

PL_EXTERN_C_END

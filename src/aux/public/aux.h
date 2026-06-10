// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "plcore/pl.h"

#include "qmmath/public/qm_math.h"
#include "qmmath/public/qm_math_vector.h"
#include "qmmath/public/qm_math_colour.h"

#define COM_COPYRIGHT "Copyright © 2020-2026 Quartermind Games, Mark E Sowden"

#if !defined( _POSIX_SOURCE )
#	define _POSIX_SOURCE 1
#endif

PL_EXTERN_C

/**
 * @brief Initializes aux library components and settings.
 *
 * This function sets up logging levels, registers packages, and initializes
 * necessary directories for the application's operation. It prepares the
 * aux library for use by setting up various subsystems like logging and
 * directory lookups essential for the application's functionality.
 */
void aux_initialize( int argc, char **argv );

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

void com_pkg_write_header( FILE *pack, unsigned int numFiles );
void com_pkg_add_data( FILE *pack, const char *path, const void *buf, size_t size );

/////////////////////////////////////////////////////////////////////////////////////
// ACM Extensions
/////////////////////////////////////////////////////////////////////////////////////

typedef struct AcmBranch AcmBranch;

QmMathVector2f com_acm_get_vector2( AcmBranch *root, const char *name, const QmMathVector2f *fallback );
QmMathVector3f com_acm_get_vector3( AcmBranch *root, const char *name, const QmMathVector3f *fallback );
QmMathVector4f com_acm_get_vector4( AcmBranch *root, const char *name, const QmMathVector4f *fallback );
QmMathColour4f com_acm_get_colour_f32( AcmBranch *root, const char *name, const QmMathColour4f *fallback );

AcmBranch *com_acm_push_vector2( AcmBranch *parent, const char *name, const QmMathVector2f *vector, bool conditional );
AcmBranch *com_acm_push_vector3( AcmBranch *parent, const char *name, const QmMathVector3f *vector, bool conditional );
AcmBranch *com_acm_push_vector4( AcmBranch *parent, const char *name, const QmMathVector4f *vector, bool conditional );
AcmBranch *com_acm_push_colour4f( AcmBranch *parent, const char *name, const QmMathColour4f *colour, bool conditional );

AcmBranch *com_acm_load_file( const char *path, const char *object );

/////////////////////////////////////////////////////////////////////////////////////
// Profiler
/////////////////////////////////////////////////////////////////////////////////////

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

bool com_profiler_start( const char *key );
bool com_profiler_end( const char *key );

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
ComProfilingGroup *com_profiler_get_next_group( const ComProfilingGroup *group );

double        com_profiler_get_time_average( const ComProfilingGroup *group );
const double *com_profiler_get_samples( const ComProfilingGroup *group, unsigned int *numPoints );

/**
 * Returns the time taken from start to end.
 *
 * @param group Group to query.
 * @return		Time in ms.
 */
double com_profiler_get_time( const ComProfilingGroup *group );

/**
 * @brief Returns the number of existing profiling groups.
 *
 * This function calculates and returns the total number of profiling groups currently available.
 * It checks if the internal hash table for profiling groups has been initialized. If it has not
 * been initialized, the function returns 0, indicating no groups are present.
 *
 * @return The total number of profiling groups, or 0 if no groups exist.
 */
unsigned int com_profiler_get_num_groups( void );

/**
 * @brief Updates the profiling samples for each profiling group.
 *
 * This function iterates over all profiling groups and updates their sample data.
 * It shifts existing sample results within the group and assigns the latest time taken
 * to the last sample position. The operation helps in maintaining a rolling history
 * of time taken for profiling analyses.
 * The function performs no operation if no profiling groups are initialized.
 */
void com_profiler_update_samples( unsigned int freq );

#define COM_ENABLE_PROFILER

#if defined( COM_ENABLE_PROFILER )
#	define COM_PROFILE_START( NAME ) com_profiler_start( ( NAME ) )
#	define COM_PROFILE_END( NAME )   com_profiler_end( ( NAME ) )

#	define COM_PROFILE_FUNCTION_START() com_profiler_start( __FUNCTION__ )
#	define COM_PROFILE_FUNCTION_END()   com_profiler_end( __FUNCTION__ )
#else
#	define COM_PROFILE_START( NAME )
#	define COM_PROFILE_END( NAME )
#	define COM_PROFILE_FUNCTION_START()
#	define COM_PROFILE_FUNCTION_END()
#endif

// Wrappers for Hei macros to take advantage of C23 features
#define COM_ITERATE_LINKED_LIST( VAR, LIST, ITR ) PL_ITERATE_LINKED_LIST( VAR, typeof( *( VAR ) ), LIST, ITR )
#define COM_ITERATE_HASHED_LIST( VAR, LIST, ITR ) PL_ITERATE_HASHED_LIST( VAR, typeof( *( VAR ) ), LIST, ITR )

/////////////////////////////////////////////////////////////////////////////////////
// Math
/////////////////////////////////////////////////////////////////////////////////////

// forward declare these from plcore -
// eventually we should move these under the common library
typedef struct PLCollisionAABB   PLCollisionAABB;
typedef struct PLCollisionRay    PLCollisionRay;
typedef struct PLCollisionPlane  PLCollisionPlane;
typedef struct PLCollisionSphere PLCollisionSphere;

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
bool qm_math_is_polygon_convex( const QmMathVector2f *vertices, unsigned int numVertices );

/**
 * Converts the given pitch and yaw into a position around a sphere.
 *
 * @param pitch
 * @param yaw
 * @return
 */
QmMathVector3f com_math_pitch_yaw_to_position( float pitch, float yaw );

/////////////////////////////////////////////////////////////////////////////////////
// Collisions
// TODO: move these into common/common_collision.h
/////////////////////////////////////////////////////////////////////////////////////

typedef struct AuxMathRectI32 AuxMathRectI32;

typedef struct ComCollisionCylinder
{
	float          radius;
	float          height;
	QmMathVector3f origin;
} ComCollisionCylinder;

bool aux_collision_aabb_intersect_aabb( const PLCollisionAABB *self, const PLCollisionAABB *other, QmMathVector3f *result );
bool com_collision_aabb_intersect_polygon( const PLCollisionAABB *aabb, const QmMathVector3f *normal, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *result );

bool com_collision_sphere_intersect_sphere( const PLCollisionSphere *sphere, const PLCollisionSphere *sphere2, QmMathVector3f *result );
bool aux_collision_sphere_intersect_aabb( const PLCollisionSphere *self, const PLCollisionAABB *other, QmMathVector3f *result );
bool aux_collision_sphere_intersect_polygon( const PLCollisionSphere *self, const QmMathVector3f *normal, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *result );

float aux_collision_cylinder_get_top( const ComCollisionCylinder *self );
bool  aux_collision_cylinder_intersect_cylinder( const ComCollisionCylinder *self, const ComCollisionCylinder *other );
bool  aux_collision_cylinder_intersect_aabb( const ComCollisionCylinder *self, const PLCollisionAABB *other, QmMathVector3f *result );
bool  aux_collision_cylinder_intersect_point( const ComCollisionCylinder *self, const QmMathVector3f *point );
bool  aux_collision_cylinder_intersect_polygon( const ComCollisionCylinder *self, const QmMathVector3f *normal, const QmMathVector3f *vertices, unsigned int numVertices );

bool com_collision_ray_intersect_aabb( const PLCollisionRay *ray, const PLCollisionAABB *aabb, QmMathVector3f *result );
bool com_collision_ray_intersect_plane( const PLCollisionRay *ray, const PLCollisionPlane *plane, QmMathVector3f *result );
bool com_collision_ray_intersect_polygon( const PLCollisionRay *ray, const QmMathVector3f *vertices, unsigned int numVertices, QmMathVector3f *result );

bool com_collision_point_intersect_recti32( const QmMathVector2f *point, const AuxMathRectI32 *rect );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

PL_EXTERN_C_END

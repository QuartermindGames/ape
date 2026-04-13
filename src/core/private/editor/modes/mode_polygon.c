// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Polygon Mode
// Author:  Mark E. Sowden

#include "ape_private.h"

#include "editor/editor.h"
#include "renderer/material/material.h"
#include "world/world.h"

/////////////////////////////////////////////////////////////////////////////////////
// Polygon Plotting
/////////////////////////////////////////////////////////////////////////////////////

#if 0// concave supporting implementation... doesn't really work :(
typedef struct Segment
{
	QmMathVector2f start;
	QmMathVector2f end;
} Segment;

static bool test_intersection( Segment s1, Segment s2 )
{
	float d1, d2, d3, d4;
	float x1 = s1.start.x, y1 = s1.start.y, x2 = s1.end.x, y2 = s1.end.y;
	float x3 = s2.start.x, y3 = s2.start.y, x4 = s2.end.x, y4 = s2.end.y;

	d1 = ( x4 - x3 ) * ( y1 - y3 ) - ( y4 - y3 ) * ( x1 - x3 );
	d2 = ( x4 - x3 ) * ( y2 - y3 ) - ( y4 - y3 ) * ( x2 - x3 );
	d3 = ( x2 - x1 ) * ( y3 - y1 ) - ( y2 - y1 ) * ( x3 - x1 );
	d4 = ( x2 - x1 ) * ( y4 - y1 ) - ( y2 - y1 ) * ( x4 - x1 );

	if ( ( ( d1 > 0 && d2 < 0 ) || ( d1 < 0 && d2 > 0 ) ) &&
		 ( ( d3 > 0 && d4 < 0 ) || ( d3 < 0 && d4 > 0 ) ) )
	{
		return true;
	}

	return false;
}

static bool validate_concave_polygon( const QmMathVector2f *vertices, unsigned int numVertices )
{
	if ( numVertices < 4 )
	{
		return true;
	}

	Segment segments[ numVertices ];
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		segments[ i ].start.x = vertices[ i ].x;
		segments[ i ].start.y = vertices[ i ].y;
		segments[ i ].end.x   = vertices[ ( i + 1 ) % numVertices ].x;
		segments[ i ].end.y   = vertices[ ( i + 1 ) % numVertices ].y;
	}

	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		for ( unsigned int j = i + 1; j < numVertices; ++j )
		{
			if ( i != j && test_intersection( segments[ i ], segments[ j ] ) )
			{
				return false;
			}
		}
	}

	return true;
}
#endif

static bool validate_convex_polygon( const QmMathVector2f *vertices, unsigned int numVertices )
{
	// this determines that the plane is convex, hopefully

	if ( numVertices < 4 )
	{
		return true;
	}

	bool sign = false;
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		// ensure any point isn't doubling up
		for ( unsigned int j = i + 1; j < numVertices; ++j )
		{
			if ( !qm_math_vector2f_compare( vertices[ i ], vertices[ j ] ) )
			{
				continue;
			}

			return false;
		}

		QmMathVector2f a;
		a.x = vertices[ ( i + 2 ) % numVertices ].x - vertices[ ( i + 1 ) % numVertices ].x;
		a.y = vertices[ ( i + 2 ) % numVertices ].y - vertices[ ( i + 1 ) % numVertices ].y;

		QmMathVector2f b;
		b.x = vertices[ i ].x - vertices[ ( i + 1 ) % numVertices ].x;
		b.y = vertices[ i ].y - vertices[ ( i + 1 ) % numVertices ].y;

		float cp = a.x * b.y - a.y * b.x;
		if ( i == 0 )
		{
			sign = cp > 0.0f;
		}
		else if ( sign != ( cp > 0 ) )
		{
			return false;
		}
	}

	return true;
}

ApeBrush *ape_editor_mode_polygon_create( ApeEditorInstance *self, const char *materialPath, ApeEditorBrushType type, bool flipFaces )
{
	if ( self->numPolygonPoints < 3 )
	{
		ape_console_warning_( "Not enough points to create a brush.\n" );
		return nullptr;
	}

	ApeCamera *camera = self->camera;
	assert( camera != nullptr );

	ApeRoom *room = ape_camera_get_room( camera );
	if ( room == nullptr )
	{
		ape_console_warning_( "No valid room for brush!\n" );
		return nullptr;
	}

	ApeBrush *brush = ape_brush_create( APE_WORLD_NODE( room ), nullptr, &QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ), &QM_MATH_VECTOR3F( 0.0f, 0.0f, 0.0f ) );
	if ( brush == nullptr )
	{
		return nullptr;
	}

	// determine the orientation of the grid
	QmMathVector3f dir;
	PlExtractMatrix4Directions( &self->grid.transform, nullptr, &dir, nullptr );

	// because the grid operates in 2D space, we need to transform all the vertices into 3D space
	// and use this time to determine the order too...so we can reverse for edge loop if needed
	float           signedArea = 0.0f;
	QmMathVector3f *vertices   = QM_OS_MEMORY_NEW_( QmMathVector3f, self->numPolygonPoints );
	for ( unsigned int i = 0; i < self->numPolygonPoints; ++i )
	{
		// determine order
		unsigned int next = ( i + 1 ) % self->numPolygonPoints;
		signedArea += self->polygonPoints[ i ].x * self->polygonPoints[ next ].y - self->polygonPoints[ next ].x * self->polygonPoints[ i ].y;

		// now transform it into 3D space
		vertices[ i ] = PlTransformVector3( &QM_MATH_VECTOR3F( self->polygonPoints[ i ].x, 0.0f, self->polygonPoints[ i ].y ), &self->grid.transform );
	}

	ApeMaterial *material;
	if ( materialPath != nullptr )
	{
		material = ape_material_cache( materialPath, APE_CACHE_GROUP_WORLD, true );
	}
	else
	{
		material = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR );
	}

	if ( !ape_brush_build_from_polygon_( brush, vertices, self->numPolygonPoints, dir, self->grid.size, flipFaces ? -signedArea : signedArea, material, type ) )
	{
		ape_console_warning_( "Failed to create brush from polygon!\n" );
		ape_material_release( material );
		ape_world_node_destroy( APE_WORLD_NODE( brush ) );
		brush = nullptr;
	}

	qm_os_memory_free( vertices );

	ape_editor_mode_polygon_clear( self );

	return brush;
}

static void compute_polygon_size( ApeEditorInstance *self )
{
	self->polySize.x = self->polygonPoints[ 0 ].x;
	self->polySize.y = self->polygonPoints[ 0 ].y;
	self->polySize.w = self->polySize.x;
	self->polySize.h = self->polySize.y;
	for ( unsigned int i = 0; i < self->numPolygonPoints; ++i )
	{
		if ( self->polygonPoints[ i ].x < self->polySize.x )
		{
			self->polySize.x = self->polygonPoints[ i ].x;
		}
		if ( self->polygonPoints[ i ].y < self->polySize.y )
		{
			self->polySize.y = self->polygonPoints[ i ].y;
		}

		if ( self->polygonPoints[ i ].x > self->polySize.w )
		{
			self->polySize.w = self->polygonPoints[ i ].x;
		}
		if ( self->polygonPoints[ i ].y > self->polySize.h )
		{
			self->polySize.h = self->polygonPoints[ i ].y;
		}
	}

	self->polySize.w = self->polySize.w - self->polySize.x;
	self->polySize.h = self->polySize.h - self->polySize.y;
}

void ape_editor_mode_polygon_remove( ApeEditorInstance *self )
{
	if ( self->numPolygonPoints == 0 )
	{
		return;
	}

	self->numPolygonPoints--;

	compute_polygon_size( self );
}

bool ape_editor_mode_polygon_add( ApeEditorInstance *self )
{
	if ( self->numPolygonPoints >= APE_BRUSH_MAX_FACE_VERTICES )
	{
		ape_console_warning_( "Hit polygon vertex limit (%u >= %u)!\n", self->numPolygonPoints, APE_BRUSH_MAX_FACE_VERTICES );
		return false;
	}

	QmMathVector2f cursor;
	if ( ape_grid_get_cursor_position( &self->grid, &cursor ) == NULL )
	{
		return false;
	}

	if ( self->numPolygonPoints > 0 )
	{
		const QmMathVector2f *start = &self->polygonPoints[ 0 ];
		if ( qm_math_vector2f_compare( *start, cursor ) )
		{
			ape_editor_mode_polygon_create( self, nullptr, APE_EDITOR_BRUSH_TYPE_BLOCK, false );
			return true;
		}
	}

	self->polygonPoints[ self->numPolygonPoints++ ] = cursor;

	// validate and then if this fails, remove the last element
	if ( !validate_convex_polygon( self->polygonPoints, self->numPolygonPoints ) )
	{
		self->numPolygonPoints--;
		return false;
	}

	compute_polygon_size( self );

	return true;
}

void ape_editor_mode_polygon_clear( ApeEditorInstance *instance )
{
	instance->numPolygonPoints = 0;
	instance->polySize         = ( PLRectangleF32 ) {};
}

void ape_editor_mode_polygon_post_render_( ApeEditorInstance *self )
{
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgDisableGraphicsState( PLG_GFX_STATE_DEPTHTEST );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadMatrix( &self->grid.transform );

	// draw pending polygon
	// and attempt to draw it to the cursor too
	QmMathVector2f cursor;
	if ( self->numPolygonPoints > 0 )
	{
		QmMathColour4ub colour                                            = PL_COLOUR_WHITE;
		QmMathVector3f  points[ ( APE_BRUSH_MAX_FACE_VERTICES * 2 ) + 1 ] = {};
		QmMathVector3f *point                                             = points;
		if ( ape_grid_get_cursor_position( &self->grid, &cursor ) != nullptr )
		{
			if ( self->numPolygonPoints < APE_BRUSH_MAX_FACE_VERTICES )
			{
				self->polygonPoints[ self->numPolygonPoints ] = cursor;
				if ( !validate_convex_polygon( self->polygonPoints, self->numPolygonPoints + 1 ) )
				{
					colour = PL_COLOUR_RED;
				}
			}

			for ( unsigned int i = 0; i < self->numPolygonPoints; ++i )
			{
				QmMathVector2f *end = ( i + 1 >= self->numPolygonPoints ) ? &cursor : &self->polygonPoints[ i + 1 ];

				point->x = self->polygonPoints[ i ].x;
				point->y = 0.0f;
				point->z = self->polygonPoints[ i ].y;
				point++;
				point->x = end->x;
				point->y = 0.0f;
				point->z = end->y;
				point++;
			}

			if ( self->numPolygonPoints > 1 )
			{
				// end line, from cursor to first polygon
				point->x = cursor.x;
				point->y = 0.0f;
				point->z = cursor.y;
				point++;
				point->x = self->polygonPoints[ 0 ].x;
				point->y = 0.0f;
				point->z = self->polygonPoints[ 0 ].y;
				point++;
			}
		}
		else if ( self->numPolygonPoints > 1 )
		{
			for ( unsigned int i = 0; i < self->numPolygonPoints; ++i )
			{
				QmMathVector2f *end = ( i + 1 >= self->numPolygonPoints ) ? &self->polygonPoints[ 0 ] : &self->polygonPoints[ i + 1 ];

				point->x = self->polygonPoints[ i ].x;
				point->y = 0.0f;
				point->z = self->polygonPoints[ i ].y;
				point++;
				point->x = end->x;
				point->y = 0.0f;
				point->z = end->y;
				point++;
			}
		}

		PlgDrawLines( points, point - points, colour, 1.0f );
	}

	PlPopMatrix();

	PlgEnableGraphicsState( PLG_GFX_STATE_DEPTHTEST );
}

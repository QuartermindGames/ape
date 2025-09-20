// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Helper functions for drawing primitives.
// Author:  Mark E. Sowden

#include "ape_private.h"
#include "renderer.h"
#include "renderer_font.h"
#include "ape/ape_public_gui.h"
#include "material/material.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

static void get_uv_coords_for_sub_rect( const PLQuad *subRect, PLGTexture *texture, float *tw, float *th, float *tx, float *ty )
{
	*tw = subRect->w / ( float ) texture->w;
	*th = subRect->h / ( float ) texture->h;
	*tx = subRect->x / ( float ) texture->w;
	*ty = subRect->y / ( float ) texture->h;
}

void ape_draw_textured_sub( PLGMesh *mesh, const PLQuad *subRect, PLGTexture *texture, float x, float y, float w, float h )
{
	float tw, th, tx, ty;
	get_uv_coords_for_sub_rect( subRect, texture, &tw, &th, &tx, &ty );

	unsigned int vX = PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x, y, 0 ), &pl_vecOrigin3, &QM_MATH_COLOUR4UB_RGB( 255, 255, 255 ), &QM_MATH_VECTOR2F( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x, y + h, 0 ), &pl_vecOrigin3, &QM_MATH_COLOUR4UB_RGB( 255, 255, 255 ), &QM_MATH_VECTOR2F( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x + w, y, 0 ), &pl_vecOrigin3, &QM_MATH_COLOUR4UB_RGB( 255, 255, 255 ), &QM_MATH_VECTOR2F( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( mesh, &QM_MATH_VECTOR3F( x + w, y + h, 0 ), &pl_vecOrigin3, &QM_MATH_COLOUR4UB_RGB( 255, 255, 255 ), &QM_MATH_VECTOR2F( tx + tw, ty + th ) );

	PlgAddMeshTriangle( mesh, vX, vY, vZ );
	PlgAddMeshTriangle( mesh, vZ, vY, vW );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_draw_sprite( ApeMaterial *material, const PLQuad *subRect, const QmMathColour4f *colour, const QmMathVector3f *position, const QmMathVector3f *origin, const QmMathVector3f *angles, float scale )
{
	PLGTexture *texture = ape_material_get_texture_( material, 0, "diffuseMap" );
	assert( texture != nullptr );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );
	assert( mesh != nullptr );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlTranslateMatrix( *position );
	PlRotateMatrix3f( PL_DEG2RAD( angles->x ), 1.0f, 0.0f, 0.0f );
	PlRotateMatrix3f( PL_DEG2RAD( angles->y ), 0.0f, 1.0f, 0.0f );
	PlRotateMatrix3f( PL_DEG2RAD( angles->z ), 0.0f, 0.0f, 1.0f );

	float tw, th, tx, ty;
	get_uv_coords_for_sub_rect( subRect, texture, &tw, &th, &tx, &ty );

	float w = subRect->w * scale;
	float h = subRect->h * scale;

	float x = origin->x * scale;
	float y = origin->y * scale;

	//FIXME: plgraphics should support using floats for colours damn it!!!!
	QmMathColour4ub c = PlColourF32ToU8( colour );

	PlgImmPushVertex( x, y, 0.0f );
	PlgImmColour( c.r, c.g, c.b, c.a );
	PlgImmTextureCoord( tx, ty + th );

	PlgImmPushVertex( x, y + h, 0.0f );
	PlgImmColour( c.r, c.g, c.b, c.a );
	PlgImmTextureCoord( tx, ty );

	PlgImmPushVertex( x + w, y, 0.0f );
	PlgImmColour( c.r, c.g, c.b, c.a );
	PlgImmTextureCoord( tx + tw, ty + th );

	PlgImmPushVertex( x + w, y + h, 0.0f );
	PlgImmColour( c.r, c.g, c.b, c.a );
	PlgImmTextureCoord( tx + tw, ty );

	ape_material_draw( material, mesh, nullptr );

	PlPopMatrix();
}

void ape_draw_textured_quad( ApeMaterial *material, float x, float y, float w, float h, const QmMathColour4ub *colour, float z )
{
	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );
	assert( mesh != NULL );

	PlgImmPushVertex( x, y, z );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );
	PlgImmTextureCoord( 0.0f, 1.0f );

	PlgImmPushVertex( x, y + h, z );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );
	PlgImmTextureCoord( 0.0f, 0.0f );

	PlgImmPushVertex( x + w, y, z );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );
	PlgImmTextureCoord( 1.0f, 1.0f );

	PlgImmPushVertex( x + w, y + h, z );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );
	PlgImmTextureCoord( 1.0f, 0.0f );

	if ( material != nullptr )
	{
		ape_material_draw( material, mesh, nullptr );
	}
	else
	{
		PlgImmDraw();
	}
}

void ape_draw_axis_pivot( QmMathVector3f position, QmMathVector3f rotation, float scale )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	QmMathVector3f angles;
	angles.x = PL_DEG2RAD( rotation.x );
	angles.y = PL_DEG2RAD( rotation.y );
	angles.z = PL_DEG2RAD( rotation.z );

	PlTranslateMatrix( position );

	PlRotateMatrix3f( angles.x, 1.0f, 0.0f, 0.0f );
	PlRotateMatrix3f( angles.y, 0.0f, 1.0f, 0.0f );
	PlRotateMatrix3f( angles.z, 0.0f, 0.0f, 1.0f );

	PLMatrix4 transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlgDrawSimpleLine( qm_math_vector3f( 0, 0, 0 ), qm_math_vector3f( scale, 0, 0 ), qm_math_colour4ub( 255, 0, 0, 255 ) );
	PlgDrawSimpleLine( qm_math_vector3f( 0, 0, 0 ), qm_math_vector3f( 0, scale, 0 ), qm_math_colour4ub( 0, 255, 0, 255 ) );
	PlgDrawSimpleLine( qm_math_vector3f( 0, 0, 0 ), qm_math_vector3f( 0, 0, scale ), qm_math_colour4ub( 0, 0, 255, 255 ) );
	//printf( "%s\n", PlPrintVector3( &position, pl_int_var ) );

	PlPopMatrix();
}

void ape_draw_digit( PLGTexture *numTextureTable[], float x, float y, int digit )
{
	if ( digit < 0 )
	{
		digit = 0;
	}
	else if ( digit > 9 )
	{
		digit = 9;
	}

	PlgDrawTexturedRectangle( x, y, ( float ) numTextureTable[ digit ]->w, ( float ) numTextureTable[ digit ]->h, numTextureTable[ digit ] );
}

void ape_draw_number( PLGTexture *numTextureTable[], float x, float y, int number )
{
	/* restrict it for sanity */
	if ( number < 0 )
	{
		number = 0;
	}
	else if ( number > 999 )
	{
		number = 999;
	}

	if ( number >= 100 )
	{
		int digit = number / 100;
		ape_draw_digit( numTextureTable, x, y, digit );
		x += ( float ) numTextureTable[ digit ]->w + 1;
	}

	if ( number >= 10 )
	{
		int digit = ( number / 10 ) % 10;
		ape_draw_digit( numTextureTable, x, y, digit );
		x += ( float ) numTextureTable[ digit ]->w + 1;
	}

	ape_draw_digit( numTextureTable, x, y, number % 10 );
}

void ape_draw_graph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max, ApeGuiFont *font )
{
	if ( numPoints < 2 )
	{
		return;
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgSetBlendMode( PLG_BLEND_DEFAULT );
	PlgDrawRectangle( x, y, w, h, qm_math_colour4ub( 0, 0, 0, 200 ) );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

	double oa = min, ob = max;
	for ( unsigned int i = 0; i < numPoints; ++i )
	{
		if ( values[ i ] > max )
		{
			max = values[ i ];
		}
		if ( values[ i ] < min )
		{
			min = values[ i ];
		}
	}

	bool outOfBounds = false;
	if ( oa != min || max != ob )
	{
		outOfBounds = true;
	}

	static const QmMathColour4ub colours[ 3 ] = {
	        PL_COLOUR_GREEN,
	        PL_COLOUR_YELLOW,
	        PL_COLOUR_RED,
	};

	unsigned int    numOutPoints = ( numPoints - 1 ) * 2;
	QmMathVector3f *points       = QM_OS_MEMORY_CALLOC( numOutPoints, sizeof( QmMathVector3f ) );

	/* convert the values we've been provided into points in our graph */
	for ( unsigned int i = 0, j = 1; j < numPoints; i++, j++ )
	{
		points[ i ].x = x + ( ( w / ( numPoints - 1 ) ) * ( j - 1 ) );
		if ( min != max )
		{
			points[ i ].y = y + h - 1 - ( ( values[ j - 1 ] - min ) * ( h / ( max - min ) ) );
		}

		++i;

		points[ i ].x = x + ( ( w / ( numPoints - 1 ) ) * j );
		if ( min != max )
		{
			points[ i ].y = y + h - 1 - ( ( values[ j ] - min ) * ( h / ( max - min ) ) );
		}

		/* leave z, it'll be initialized as 0 */
	}

#if 0
	QmMathVector3f border[] = {
	        { x, y, 0 },
	        { x + w, y, 0 },// top
	        { x, y, 0 },
	        { x, y + h, 0 },// left
	        { x + w, y + h, 0 },
	        { x, y + h, 0 },// bottom
	        { x + w, y + h, 0 },
	        { x + w, y, 0 },// right
	};
	PlgDrawLines( border, QM_OS_ARRAY_ELEMENTS( border ), PL_COLOUR_GOLD, 1.0f );
#endif

	PlgDrawLines( points, numOutPoints, PL_COLOUR_WHITE, 1.0f );

	float sy = y + 4.0f;
	if ( heading != NULL )
	{
		float sw, sh;
		gui_font_get_string_pixel_size( font, 1.0f, heading, strlen( heading ), &sw, &sh );
		gui_font_draw_string( font, x + w - sw - 2.0f, sy, nullptr, &sy, 1.0f, &PL_COLOUR_VIOLET, heading, strlen( heading ), false );
	}

	// Calculate the average sum of all the points
	double avg = 0.0;
	for ( unsigned int i = 0; i < numPoints; ++i )
	{
		avg += values[ i ];
	}

	avg /= numPoints;

	float ch = gui_font_get_line_spacing( font );

	char buf[ 128 ];

	snprintf( buf, sizeof( buf ), "CUR %02f\n", values[ numPoints - 1 ] );
	gui_font_draw_string( font, x + 2.0f, y + h / 2.0f - ch / 2.0f + ch + 8.0f, nullptr, nullptr, 1.0f, outOfBounds ? &PL_COLOUR_INDIAN_RED : &PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );

	snprintf( buf, sizeof( buf ), "AVG %02f\n", avg );
	gui_font_draw_string( font, x + 2.0f, y + h / 2.0f - ch / 2.0f - ch - 8.0f, nullptr, nullptr, 1.0f, outOfBounds ? &PL_COLOUR_INDIAN_RED : &PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );

	//gui_font_draw_string( font, x + 2.0f, y + h / 2 - font)
	//ape_bitmap_font_batch_string( font, x + 2.0f, y + ( h / 2 ) - ( font->ch / 2 ) + font->ch, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), true );
	//
	//ape_bitmap_font_batch_string( font, x + 2.0f, y + ( h / 2 ) - ( font->ch / 2 ) - font->ch, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), true );

	qm_os_memory_free( points );
}

void ape_draw_rectangle_( PLGMesh *mesh, float x, float y, float w, float h, const QmMathColour4ub *colour )
{
	unsigned int a, b, c, d;
	a = PlgPushVertex3f( mesh, x, y, 0.0f );
	PlgColour4bv( mesh, colour );
	b = PlgPushVertex3f( mesh, x + w, y, 0.0f );
	PlgColour4bv( mesh, colour );
	c = PlgPushVertex3f( mesh, x, y + h, 0.0f );
	PlgColour4bv( mesh, colour );
	d = PlgPushVertex3f( mesh, x + w, y + h, 0.0f );
	PlgColour4bv( mesh, colour );

	PlgPushTriangle( mesh, c, b, a );
	PlgPushTriangle( mesh, c, d, b );
}

void ape_draw_bevel_rectangle_( PLGMesh *mesh, float x, float y, float w, float h, float depth, const QmMathColour4ub *colour, bool inset )
{
	unsigned int a, b, c, d;

	static constexpr int DIFF = 32;

	QmMathColour4ub ul = *colour;
	for ( unsigned int i = 0; i < 3; ++i )
	{
		uint8_t *v = &( ( uint8_t * ) &ul )[ i ];
		if ( ( int ) *v + DIFF > 255 )
		{
			*v = 255;
			continue;
		}

		*v = ( ( uint8_t * ) colour )[ i ] + DIFF;
	}

	QmMathColour4ub ll = *colour;
	for ( unsigned int i = 0; i < 3; ++i )
	{
		uint8_t *v = &( ( uint8_t * ) &ll )[ i ];
		if ( ( int ) *v - DIFF < 0 )
		{
			*v = 0;
			continue;
		}

		*v = ( ( uint8_t * ) colour )[ i ] - DIFF;
	}

	if ( inset )
	{
		QmMathColour4ub tmp = ul;
		ul                  = ll;
		ll                  = tmp;
	}

	// top bit

	a = PlgPushVertex3f( mesh, x, y, 0.0f );
	PlgColour4bv( mesh, &ul );
	b = PlgPushVertex3f( mesh, x + w, y, 0.0f );
	PlgColour4bv( mesh, &ul );
	c = PlgPushVertex3f( mesh, x + depth, y + depth, 0.0f );
	PlgColour4bv( mesh, &ul );
	d = PlgPushVertex3f( mesh, x + w - depth, y + depth, 0.0f );
	PlgColour4bv( mesh, &ul );

	PlgPushTriangle( mesh, c, b, a );
	PlgPushTriangle( mesh, c, d, b );

	// bottom bit

	a = PlgPushVertex3f( mesh, x + depth, y + h - depth, 0.0f );
	PlgColour4bv( mesh, &ll );
	b = PlgPushVertex3f( mesh, x + w - depth, y + h - depth, 0.0f );
	PlgColour4bv( mesh, &ll );
	c = PlgPushVertex3f( mesh, x, y + h, 0.0f );
	PlgColour4bv( mesh, &ll );
	d = PlgPushVertex3f( mesh, x + w, y + h, 0.0f );
	PlgColour4bv( mesh, &ll );

	PlgPushTriangle( mesh, c, b, a );
	PlgPushTriangle( mesh, c, d, b );

	// left bit

	a = PlgPushVertex3f( mesh, x, y, 0.0f );
	PlgColour4bv( mesh, &ul );
	b = PlgPushVertex3f( mesh, x + depth, y + depth, 0.0f );
	PlgColour4bv( mesh, &ul );
	c = PlgPushVertex3f( mesh, x, y + h, 0.0f );
	PlgColour4bv( mesh, &ul );
	d = PlgPushVertex3f( mesh, x + depth, y + h - depth, 0.0f );
	PlgColour4bv( mesh, &ul );

	PlgPushTriangle( mesh, c, b, a );
	PlgPushTriangle( mesh, c, d, b );

	// right bit

	a = PlgPushVertex3f( mesh, x + w - depth, y + depth, 0.0f );
	PlgColour4bv( mesh, &ll );
	b = PlgPushVertex3f( mesh, x + w, y, 0.0f );
	PlgColour4bv( mesh, &ll );
	c = PlgPushVertex3f( mesh, x + w - depth, y + h - depth, 0.0f );
	PlgColour4bv( mesh, &ll );
	d = PlgPushVertex3f( mesh, x + w, y + h, 0.0f );
	PlgColour4bv( mesh, &ll );

	PlgPushTriangle( mesh, c, b, a );
	PlgPushTriangle( mesh, c, d, b );

	// middle bit

	a = PlgPushVertex3f( mesh, x + depth, y + depth, 0.0f );
	PlgColour4bv( mesh, colour );
	b = PlgPushVertex3f( mesh, x + w - depth, y + depth, 0.0f );
	PlgColour4bv( mesh, colour );
	c = PlgPushVertex3f( mesh, x + depth, y + h - depth, 0.0f );
	PlgColour4bv( mesh, colour );
	d = PlgPushVertex3f( mesh, x + w - depth, y + h - depth, 0.0f );
	PlgColour4bv( mesh, colour );

	PlgPushTriangle( mesh, c, b, a );
	PlgPushTriangle( mesh, c, d, b );
}

/////////////////////////////////////////////////////////////////////////////////////
// Debug Draw
/////////////////////////////////////////////////////////////////////////////////////

static PLGMesh     *debugDrawMesh;
static ApeMaterial *debugDrawMaterial;

void ape_draw_initialize_debug_mesh_()
{
	assert( debugDrawMesh == nullptr );
	assert( debugDrawMaterial == nullptr );

	debugDrawMesh = PlgCreateMesh( PLG_MESH_LINES, PLG_DRAW_DYNAMIC, 0, 2048 );
	if ( debugDrawMesh == nullptr )
	{
		ape_error_( true, "Failed to create debug draw mesh: %s\n", PlGetError() );
	}

	debugDrawMaterial = ape_material_cache( "materials/engine/vertex.mat.n", APE_CACHE_GROUP_GLOBAL, false );
	if ( debugDrawMaterial == nullptr )
	{
		ape_error_( true, "Failed to cache debug draw material!\n" );
	}
}

void ape_draw_destroy_debug_mesh_()
{
	assert( debugDrawMesh != nullptr );
	assert( debugDrawMaterial != nullptr );

	PlgDestroyMesh( debugDrawMesh );
	debugDrawMesh = nullptr;

	ape_material_release( debugDrawMaterial );
	debugDrawMaterial = nullptr;
}

void ape_draw_debug_clear_()
{
	PlgClearMesh( debugDrawMesh );
}

void ape_draw_debug_mesh_display_()
{
	if ( debugDrawMesh->num_verts == 0 )
	{
		return;
	}

	ape_material_draw( debugDrawMaterial, debugDrawMesh, nullptr );
}

void ape_draw_debug_line( QmMathVector3f start, QmMathVector3f end, QmMathColour4ub colour )
{
	PlgAddMeshVertex( debugDrawMesh, &start, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
	PlgAddMeshVertex( debugDrawMesh, &end, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
}

void ape_draw_debug_arrow( QmMathVector3f start, QmMathVector3f end, QmMathColour4ub colour, float scale )
{
	QmMathVector3f direction = qm_math_vector3f_sub( end, start );
	direction                = qm_math_vector3f_normalize( direction );

	QmMathVector3f arrowHead  = qm_math_vector3f_sub( end, qm_math_vector3f_scale_float( direction, scale ) );
	QmMathVector3f arrowLeft  = qm_math_vector3f_add( arrowHead, qm_math_vector3f_scale_float( qm_math_vector3f_normalize( qm_math_vector3f_cross_product( direction, qm_math_vector3f( 0.0f, 0.0f, 1.0f ) ) ), scale ) );
	QmMathVector3f arrowRight = qm_math_vector3f_add( arrowHead, qm_math_vector3f_scale_float( qm_math_vector3f_normalize( qm_math_vector3f_cross_product( qm_math_vector3f( 0.0f, 0.0f, 1.0f ), direction ) ), scale ) );

	PlgAddMeshVertex( debugDrawMesh, &start, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
	PlgAddMeshVertex( debugDrawMesh, &end, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );

	PlgAddMeshVertex( debugDrawMesh, &end, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
	PlgAddMeshVertex( debugDrawMesh, &arrowLeft, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );

	PlgAddMeshVertex( debugDrawMesh, &end, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
	PlgAddMeshVertex( debugDrawMesh, &arrowRight, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
}

void ape_draw_debug_sphere( QmMathVector3f origin, QmMathColour4ub colour, float scale )
{
	static constexpr unsigned int NUM_SEGMENTS = 16;
	static constexpr float        DELTA        = 2.0f * PL_PI / NUM_SEGMENTS;

	// array to store the vertices
	QmMathVector3f vertices[ NUM_SEGMENTS + 1 ][ NUM_SEGMENTS + 1 ];

	// Generate the vertices
	for ( int i = 0; i <= NUM_SEGMENTS; ++i )
	{
		for ( int j = 0; j <= NUM_SEGMENTS; ++j )
		{
			float angle1 = ( float ) i * DELTA;// azimuth
			float angle2 = ( float ) j * DELTA;// elevation

			QmMathVector3f pos = {
			        .x = origin.x + scale * sinf( angle2 ) * cosf( angle1 ),
			        .y = origin.y + scale * sinf( angle2 ) * sinf( angle1 ),
			        .z = origin.z + scale * cosf( angle2 ),
			};

			vertices[ i ][ j ] = pos;
		}
	}

	// Create lines along latitudes (across)
	for ( int i = 0; i <= NUM_SEGMENTS; ++i )
	{
		for ( int j = 0; j < NUM_SEGMENTS; ++j )
		{
			PlgAddMeshVertex( debugDrawMesh, &vertices[ i ][ j ], &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
			PlgAddMeshVertex( debugDrawMesh, &vertices[ i ][ j + 1 ], &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
		}
	}

	// Create lines along longitudes (up-down)
	for ( int i = 0; i < NUM_SEGMENTS; ++i )
	{
		for ( int j = 0; j <= NUM_SEGMENTS; ++j )
		{
			PlgAddMeshVertex( debugDrawMesh, &vertices[ i ][ j ], &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
			PlgAddMeshVertex( debugDrawMesh, &vertices[ i + 1 ][ j ], &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
		}
	}
}

void ape_draw_debug_axis( const QmMathVector3f origin, QmMathVector3f angles, const float scale )
{
	//TODO: represent angles...
	ape_draw_debug_arrow( origin, qm_math_vector3f_add( origin, QM_MATH_VECTOR3F( scale, 0.0f, 0.0f ) ), PL_COLOUR_RED, scale / 4.0f );
	ape_draw_debug_arrow( origin, qm_math_vector3f_add( origin, QM_MATH_VECTOR3F( 0.0f, scale, 0.0f ) ), PL_COLOUR_GREEN, scale / 4.0f );
	ape_draw_debug_arrow( origin, qm_math_vector3f_add( origin, QM_MATH_VECTOR3F( 0.0f, 0.0f, scale ) ), PL_COLOUR_BLUE, scale / 4.0f );
}

void ape_draw_debug_aabb( const PLCollisionAABB *aabb, QmMathColour4ub colour )
{
	QmMathVector3f corners[ 8 ];

	// bottom
	corners[ 0 ] = qm_math_vector3f( aabb->origin.x + aabb->mins.x, aabb->origin.y + aabb->mins.y, aabb->origin.z + aabb->mins.z );
	corners[ 1 ] = qm_math_vector3f( aabb->origin.x + aabb->mins.x, aabb->origin.y + aabb->mins.y, aabb->origin.z + aabb->maxs.z );
	corners[ 2 ] = qm_math_vector3f( aabb->origin.x + aabb->maxs.x, aabb->origin.y + aabb->mins.y, aabb->origin.z + aabb->mins.z );
	corners[ 3 ] = qm_math_vector3f( aabb->origin.x + aabb->maxs.x, aabb->origin.y + aabb->mins.y, aabb->origin.z + aabb->maxs.z );

	// top
	corners[ 4 ] = qm_math_vector3f( aabb->origin.x + aabb->mins.x, aabb->origin.y + aabb->maxs.y, aabb->origin.z + aabb->mins.z );
	corners[ 5 ] = qm_math_vector3f( aabb->origin.x + aabb->mins.x, aabb->origin.y + aabb->maxs.y, aabb->origin.z + aabb->maxs.z );
	corners[ 6 ] = qm_math_vector3f( aabb->origin.x + aabb->maxs.x, aabb->origin.y + aabb->maxs.y, aabb->origin.z + aabb->mins.z );
	corners[ 7 ] = qm_math_vector3f( aabb->origin.x + aabb->maxs.x, aabb->origin.y + aabb->maxs.y, aabb->origin.z + aabb->maxs.z );

	ape_draw_debug_line( corners[ 0 ], corners[ 1 ], colour );
	ape_draw_debug_line( corners[ 0 ], corners[ 2 ], colour );
	ape_draw_debug_line( corners[ 3 ], corners[ 1 ], colour );
	ape_draw_debug_line( corners[ 3 ], corners[ 2 ], colour );

	ape_draw_debug_line( corners[ 4 ], corners[ 5 ], colour );
	ape_draw_debug_line( corners[ 4 ], corners[ 6 ], colour );
	ape_draw_debug_line( corners[ 7 ], corners[ 5 ], colour );
	ape_draw_debug_line( corners[ 7 ], corners[ 6 ], colour );

	// corners
	ape_draw_debug_line( corners[ 0 ], corners[ 4 ], colour );
	ape_draw_debug_line( corners[ 1 ], corners[ 5 ], colour );
	ape_draw_debug_line( corners[ 2 ], corners[ 6 ], colour );
	ape_draw_debug_line( corners[ 3 ], corners[ 7 ], colour );
}

void ape_draw_debug_cylinder( const ComCollisionCylinder *cylinder, const QmMathColour4ub *colour, unsigned int resolution )
{
	static constexpr unsigned int MAX_RESOLUTION = 64;

	if ( resolution == 0 || cylinder->radius == 0.0f )
	{
		return;
	}

	resolution = PL_MIN( resolution, MAX_RESOLUTION - 1 );

	QmMathVector3f vertices[ MAX_RESOLUTION ] = {};
	for ( unsigned int i = 0; i < resolution; ++i )
	{
		float angle   = PL_DEG2RAD( 360.0f * i / resolution );
		vertices[ i ] = qm_math_vector3f(
		        cylinder->origin.x + cosf( angle ) * cylinder->radius,
		        cylinder->origin.y,
		        cylinder->origin.z + sinf( angle ) * cylinder->radius );
	}

	for ( unsigned int i = 0; i < resolution; ++i )
	{
		const QmMathVector3f *a = &vertices[ i ];
		const QmMathVector3f *b = &vertices[ i == 0 ? resolution - 1 : i - 1 ];

		// bottom
		ape_draw_debug_line( *a, *b, *colour );

		// top
		QmMathVector3f ua = qm_math_vector3f( a->x, cylinder->origin.y + cylinder->height, a->z );
		QmMathVector3f ub = qm_math_vector3f( b->x, cylinder->origin.y + cylinder->height, b->z );
		ape_draw_debug_line( ua, ub, *colour );

		// line between
		ape_draw_debug_line( *a, ua, *colour );
	}

	ape_draw_debug_axis( cylinder->origin, ( QmMathVector3f ) {}, 2.0f );
}

void ape_draw_debug_cone( QmMathVector3f origin, QmMathVector3f angles, const QmMathColour4ub *colour, float range, float radius, unsigned int resolution )
{
	static constexpr unsigned int MAX_RESOLUTION = 64;

	if ( resolution == 0 || range == 0.0f )
	{
		return;
	}

	resolution = PL_MIN( resolution, MAX_RESOLUTION - 1 );

	PLMatrix4 mx = PlRotateMatrix4( PL_DEG2RAD( angles.x ), &QM_MATH_VECTOR3F( 1.0f, 0.0f, 0.0f ) );
	PLMatrix4 my = PlRotateMatrix4( PL_DEG2RAD( angles.y ), &QM_MATH_VECTOR3F( 0.0f, 1.0f, 0.0f ) );
	PLMatrix4 mz = PlRotateMatrix4( PL_DEG2RAD( angles.z ), &QM_MATH_VECTOR3F( 0.0f, 0.0f, 1.0f ) );

	PLMatrix4 rt = PlMatrix4Identity();
	rt           = PlMultiplyMatrix4( &rt, &mx );
	rt           = PlMultiplyMatrix4( &rt, &my );
	rt           = PlMultiplyMatrix4( &rt, &mz );

	QmMathVector3f vertices[ MAX_RESOLUTION ] = {};
	for ( unsigned int i = 0; i < resolution; ++i )
	{
		float          angle = PL_DEG2RAD( 360.0f * i / resolution );
		QmMathVector3f pos   = qm_math_vector3f( cosf( angle ) * radius, range, sinf( angle ) * radius );
		vertices[ i ]        = qm_math_vector3f_add( origin, PlTransformVector3( &pos, &rt ) );
	}

	for ( unsigned int i = 0; i < resolution; ++i )
	{
		const QmMathVector3f *a = &vertices[ i ];
		const QmMathVector3f *b = &vertices[ i == 0 ? resolution - 1 : i - 1 ];

		ape_draw_debug_line( *a, *b, *colour );
		ape_draw_debug_line( origin, *a, *colour );
	}
}

void ape_draw_debug_plane( const PLCollisionPlane *plane, QmMathColour4ub colour, float scale )
{
	QmMathVector3f normal = qm_math_vector3f_normalize( plane->normal );

	ape_draw_debug_arrow( plane->origin, qm_math_vector3f_add( plane->origin, qm_math_vector3f_scale_float( normal, 4.0f ) ), colour, 1.0f );

	QmMathVector3f tangent;
	if ( fabsf( normal.x ) > fabsf( normal.y ) )
	{
		tangent = QM_MATH_VECTOR3F( -normal.z, 0, normal.x );
	}
	else
	{
		tangent = QM_MATH_VECTOR3F( 0, normal.z, -normal.y );
	}
	tangent = qm_math_vector3f_normalize( tangent );

	QmMathVector3f bitangent = qm_math_vector3f_cross_product( normal, tangent );
	tangent                  = qm_math_vector3f_scale_float( tangent, scale );
	bitangent                = qm_math_vector3f_scale_float( bitangent, scale );

	QmMathVector3f corner1 = qm_math_vector3f_add( plane->origin, qm_math_vector3f_add( tangent, bitangent ) );
	QmMathVector3f corner2 = qm_math_vector3f_add( plane->origin, qm_math_vector3f_sub( tangent, bitangent ) );
	QmMathVector3f corner3 = qm_math_vector3f_sub( plane->origin, qm_math_vector3f_add( tangent, bitangent ) );
	QmMathVector3f corner4 = qm_math_vector3f_sub( plane->origin, qm_math_vector3f_sub( tangent, bitangent ) );

	ape_draw_debug_line( corner1, corner2, colour );
	ape_draw_debug_line( corner2, corner3, colour );
	ape_draw_debug_line( corner3, corner4, colour );
	ape_draw_debug_line( corner4, corner1, colour );
}

void ape_draw_debug_polygon( const QmMathVector3f *vertices, unsigned int numVertices, QmMathColour4ub colour )
{
	for ( unsigned int i = 0; i < numVertices; ++i )
	{
		ape_draw_debug_line( vertices[ i ], vertices[ ( i + 1 ) % numVertices ], colour );
	}
}

void ape_draw_debug_string( const float x, const float y, const float z, const QmMathColour4ub *colour, const char *string, ... )
{
	va_list args;
	va_start( args, string );
	char *buf;
	vasprintf( &buf, string, args );
	va_end( args );

	ApeGuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_TINY );

	gui_font_draw_string( font, x, y, nullptr, nullptr, 1.0f, colour, buf, strlen( buf ), false );

	free( buf );
}

// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Helper functions for drawing primitives.
// Author:  Mark E. Sowden

#include "ape_private.h"
#include "renderer.h"
#include "renderer_font.h"

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

	unsigned int vX = PlgAddMeshVertex( mesh, &PL_VECTOR3( x, y, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PL_VECTOR2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( mesh, &PL_VECTOR3( x, y + h, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PL_VECTOR2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( mesh, &PL_VECTOR3( x + w, y, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PL_VECTOR2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( mesh, &PL_VECTOR3( x + w, y + h, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PL_VECTOR2( tx + tw, ty + th ) );

	PlgAddMeshTriangle( mesh, vX, vY, vZ );
	PlgAddMeshTriangle( mesh, vZ, vY, vW );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_draw_sprite( ApeMaterial *material, const PLQuad *subRect, const PLColourF32 *colour, const PLVector3 *position, const PLVector3 *origin, const PLVector3 *angles, float scale )
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
	PLColour c = PlColourF32ToU8( colour );

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

void ape_draw_textured_quad( ApeMaterial *material, float x, float y, float w, float h, const PLColour *colour )
{
	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_STRIP );
	assert( mesh != NULL );

	PlgImmPushVertex( x, y, 0.0f );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );
	PlgImmTextureCoord( 0.0f, 1.0f );

	PlgImmPushVertex( x, y + h, 0.0f );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );
	PlgImmTextureCoord( 0.0f, 0.0f );

	PlgImmPushVertex( x + w, y, 0.0f );
	PlgImmColour( colour->r, colour->g, colour->b, colour->a );
	PlgImmTextureCoord( 1.0f, 1.0f );

	PlgImmPushVertex( x + w, y + h, 0.0f );
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

void arl_draw_axis_pivot( PLVector3 position, PLVector3 rotation, float scale )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PlLoadIdentityMatrix();

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PLVector3 angles;
	angles.x = PL_DEG2RAD( rotation.x );
	angles.y = PL_DEG2RAD( rotation.y );
	angles.z = PL_DEG2RAD( rotation.z );

	PlTranslateMatrix( position );

	PlRotateMatrix3f( angles.x, 1.0f, 0.0f, 0.0f );
	PlRotateMatrix3f( angles.y, 0.0f, 1.0f, 0.0f );
	PlRotateMatrix3f( angles.z, 0.0f, 0.0f, 1.0f );

	PLMatrix4 transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlgDrawSimpleLine( PL_VECTOR3( 0, 0, 0 ), PL_VECTOR3( scale, 0, 0 ), PLColour( 255, 0, 0, 255 ) );
	PlgDrawSimpleLine( PL_VECTOR3( 0, 0, 0 ), PL_VECTOR3( 0, scale, 0 ), PLColour( 0, 255, 0, 255 ) );
	PlgDrawSimpleLine( PL_VECTOR3( 0, 0, 0 ), PL_VECTOR3( 0, 0, scale ), PLColour( 0, 0, 255, 255 ) );
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

void ape_draw_graph( const char *heading, float x, float y, float w, float h, const double *values, unsigned int numPoints, float min, float max )
{
	if ( numPoints < 2 )
	{
		return;
	}

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

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

#if 0
	bool outOfBounds = false;
	if ( oa != min || max != ob )
	{
		outOfBounds = true;
	}
#endif

	static const PLColour colours[ 3 ] = {
	        PL_COLOUR_GREEN,
	        PL_COLOUR_YELLOW,
	        PL_COLOUR_RED,
	};

	unsigned int numOutPoints = ( numPoints - 1 ) * 2;
	PLVector3   *points       = PlCAllocA( numOutPoints, sizeof( PLVector3 ) );

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

	PlgSetBlendMode( PLG_BLEND_DEFAULT );
	PlgDrawRectangle( x, y, w, h, PLColour( 0, 0, 0, 200 ) );
	PlgSetBlendMode( PLG_BLEND_DISABLE );

#if 0
	PLVector3 border[] = {
	        { x, y, 0 },
	        { x + w, y, 0 },// top
	        { x, y, 0 },
	        { x, y + h, 0 },// left
	        { x + w, y + h, 0 },
	        { x, y + h, 0 },// bottom
	        { x + w, y + h, 0 },
	        { x + w, y, 0 },// right
	};
	PlgDrawLines( border, PL_ARRAY_ELEMENTS( border ), PL_COLOUR_GOLD, 1.0f );
#endif

	PlgDrawLines( points, numOutPoints, PL_COLOUR_WHITE, 1.0f );

	ApeBitmapFont *font = ape_get_default_small_bitmap_font();
	ape_bitmap_font_begin_draw( font );

	if ( heading != NULL )
	{
		size_t len  = strlen( heading );
		float  cPos = ( x + w - ( len * font->cw ) ) - 2.0f;
		ape_bitmap_font_batch_string( font, cPos, y + 2.0f, 1.0f, PL_COLOUR_VIOLET, heading, len, false );
	}

	// Calculate the average sum of all the points
	double avg = 0.0;
	for ( unsigned int i = 0; i < numPoints; ++i )
	{
		avg += values[ i ];
	}

	avg /= numPoints;

	char buf[ 128 ];

	// Current and average readings
	snprintf( buf, sizeof( buf ), "CUR %02f", values[ numPoints - 1 ] );
	ape_bitmap_font_batch_string( font, x + 2.0f, y + ( h / 2 ) - ( font->ch / 2 ) + font->ch, 1.0f, /*outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN*/ PL_COLOUR_VIOLET, buf, strlen( buf ), true );
	snprintf( buf, sizeof( buf ), "AVG %02f", avg );
	ape_bitmap_font_batch_string( font, x + 2.0f, y + ( h / 2 ) - ( font->ch / 2 ) - font->ch, 1.0f, /*outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN*/ PL_COLOUR_VIOLET, buf, strlen( buf ), true );

#if 0
	snprintf( buf, sizeof( buf ), "y+:%02f", max );
	Font_AddBitmapStringToPass( font, x + 2.0f, y + 2.0f, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );
	snprintf( buf, sizeof( buf ), "y-:%02f", min );
	Font_AddBitmapStringToPass( font, x + 2.0f, y + ( h - font->ch ) - 2.0f, 1.0f, outOfBounds ? PL_COLOUR_INDIAN_RED : PL_COLOUR_SEA_GREEN, buf, strlen( buf ), false );
#endif

	ape_bitmap_font_draw( font );

	PL_DELETE( points );
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

	debugDrawMaterial = ape_material_cache( "materials/engine/vertex.mat.n", APE_CACHE_GROUP_GLOBAL, false, false );
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
	ape_material_draw( debugDrawMaterial, debugDrawMesh, nullptr );
}

void ape_draw_debug_line( PLVector3 start, PLVector3 end, PLColour colour )
{
	PlgAddMeshVertex( debugDrawMesh, &start, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
	PlgAddMeshVertex( debugDrawMesh, &end, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
}

void ape_draw_debug_arrow( PLVector3 start, PLVector3 end, PLColour colour )
{
	PLVector3 direction = PlSubtractVector3( end, start );
	direction           = PlNormalizeVector3( direction );

	PLVector3 arrowHead  = PlSubtractVector3( end, PlScaleVector3F( direction, 0.5f ) );
	PLVector3 arrowLeft  = PlAddVector3( arrowHead, PlScaleVector3F( PlNormalizeVector3( PlVector3CrossProduct( direction, PL_VECTOR3( 0.0f, 0.0f, 1.0f ) ) ), 0.5f ) );
	PLVector3 arrowRight = PlAddVector3( arrowHead, PlScaleVector3F( PlNormalizeVector3( PlVector3CrossProduct( PL_VECTOR3( 0.0f, 0.0f, 1.0f ), direction ) ), 0.5f ) );

	PlgAddMeshVertex( debugDrawMesh, &start, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
	PlgAddMeshVertex( debugDrawMesh, &end, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );

	PlgAddMeshVertex( debugDrawMesh, &end, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
	PlgAddMeshVertex( debugDrawMesh, &arrowLeft, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );

	PlgAddMeshVertex( debugDrawMesh, &end, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
	PlgAddMeshVertex( debugDrawMesh, &arrowRight, &pl_vecOrigin3, &colour, &pl_vecOrigin2 );
}

void ape_draw_debug_sphere( PLVector3 origin, PLColour colour, float scale )
{
	static constexpr uint  NUM_SEGMENTS = 16;
	static constexpr float DELTA        = 2.0f * PL_PI / NUM_SEGMENTS;

	// array to store the vertices
	PLVector3 vertices[ NUM_SEGMENTS + 1 ][ NUM_SEGMENTS + 1 ];

	// Generate the vertices
	for ( int i = 0; i <= NUM_SEGMENTS; ++i )
	{
		for ( int j = 0; j <= NUM_SEGMENTS; ++j )
		{
			float angle1 = ( float ) i * DELTA;// azimuth
			float angle2 = ( float ) j * DELTA;// elevation

			PLVector3 pos = {
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

void ape_draw_debug_axis( PLVector3 origin, PLVector3 angles, float scale )
{
	//TODO: represent angles...
	ape_draw_debug_arrow( origin, PlAddVector3( origin, ( PLVector3 ){ scale, 0.0f, 0.0f } ), PL_COLOUR_RED );
	ape_draw_debug_arrow( origin, PlAddVector3( origin, ( PLVector3 ){ 0.0f, scale, 0.0f } ), PL_COLOUR_GREEN );
	ape_draw_debug_arrow( origin, PlAddVector3( origin, ( PLVector3 ){ 0.0f, 0.0f, scale } ), PL_COLOUR_BLUE );
}

void ape_draw_debug_aabb( const PLCollisionAABB *aabb, PLColour colour )
{
	PLVector3 corners[ 8 ];

	// bottom
	corners[ 0 ] = PL_VECTOR3( aabb->origin.x + aabb->mins.x, aabb->origin.y + aabb->mins.y, aabb->origin.z + aabb->mins.z );
	corners[ 1 ] = PL_VECTOR3( aabb->origin.x + aabb->mins.x, aabb->origin.y + aabb->mins.y, aabb->origin.z + aabb->maxs.z );
	corners[ 2 ] = PL_VECTOR3( aabb->origin.x + aabb->maxs.x, aabb->origin.y + aabb->mins.y, aabb->origin.z + aabb->mins.z );
	corners[ 3 ] = PL_VECTOR3( aabb->origin.x + aabb->maxs.x, aabb->origin.y + aabb->mins.y, aabb->origin.z + aabb->maxs.z );

	// top
	corners[ 4 ] = PL_VECTOR3( aabb->origin.x + aabb->mins.x, aabb->origin.y + aabb->maxs.y, aabb->origin.z + aabb->mins.z );
	corners[ 5 ] = PL_VECTOR3( aabb->origin.x + aabb->mins.x, aabb->origin.y + aabb->maxs.y, aabb->origin.z + aabb->maxs.z );
	corners[ 6 ] = PL_VECTOR3( aabb->origin.x + aabb->maxs.x, aabb->origin.y + aabb->maxs.y, aabb->origin.z + aabb->mins.z );
	corners[ 7 ] = PL_VECTOR3( aabb->origin.x + aabb->maxs.x, aabb->origin.y + aabb->maxs.y, aabb->origin.z + aabb->maxs.z );

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

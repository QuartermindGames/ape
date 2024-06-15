// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
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

	unsigned int vX = PlgAddMeshVertex( mesh, &PLVector3( x, y, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PLVector2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( mesh, &PLVector3( x, y + h, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PLVector2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( mesh, &PLVector3( x + w, y, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PLVector2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( mesh, &PLVector3( x + w, y + h, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PLVector2( tx + tw, ty + th ) );

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
	PlRotateMatrix( PL_DEG2RAD( angles->x ), 1.0f, 0.0f, 0.0f );
	PlRotateMatrix( PL_DEG2RAD( angles->y ), 0.0f, 1.0f, 0.0f );
	PlRotateMatrix( PL_DEG2RAD( angles->z ), 0.0f, 0.0f, 1.0f );

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

	ape_material_draw( material, mesh, NULL, 0 );

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
		ape_material_draw( material, mesh, NULL, 0 );
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

	PlRotateMatrix( angles.x, 1.0f, 0.0f, 0.0f );
	PlRotateMatrix( angles.y, 0.0f, 1.0f, 0.0f );
	PlRotateMatrix( angles.z, 0.0f, 0.0f, 1.0f );

	PLMatrix4 transform = *PlGetMatrix( PL_MODELVIEW_MATRIX );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( scale, 0, 0 ), PLColour( 255, 0, 0, 255 ) );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 0, scale, 0 ), PLColour( 0, 255, 0, 255 ) );
	PlgDrawSimpleLine( transform, PLVector3( 0, 0, 0 ), PLVector3( 0, 0, scale ), PLColour( 0, 0, 255, 255 ) );
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
	PLVector3 *points = PlCAllocA( numOutPoints, sizeof( PLVector3 ) );

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
		size_t len = strlen( heading );
		float cPos = ( x + w - ( len * font->cw ) ) - 2.0f;
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

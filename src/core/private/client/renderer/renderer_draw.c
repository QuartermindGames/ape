// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "core_private.h"
#include "renderer.h"

/////////////////////////////////////////////////////////////////
// 2D Primitives

static void GetUVCoordsForSubRect( const PLQuad *subRect, PLGTexture *texture, float *tw, float *th, float *tx, float *ty )
{
	*tw = subRect->w / ( float ) texture->w;
	*th = subRect->h / ( float ) texture->h;
	*tx = subRect->x / ( float ) texture->w;
	*ty = subRect->y / ( float ) texture->h;
}

void Renderer_Draw_TexturedSubRect2D( PLGMesh *mesh, const PLQuad *subRect, PLGTexture *texture, float x, float y, float w, float h )
{
	float tw, th, tx, ty;
	GetUVCoordsForSubRect( subRect, texture, &tw, &th, &tx, &ty );

	unsigned int vX = PlgAddMeshVertex( mesh, &PLVector3( x, y, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PLVector2( tx, ty ) );
	unsigned int vY = PlgAddMeshVertex( mesh, &PLVector3( x, y + h, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PLVector2( tx, ty + th ) );
	unsigned int vZ = PlgAddMeshVertex( mesh, &PLVector3( x + w, y, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PLVector2( tx + tw, ty ) );
	unsigned int vW = PlgAddMeshVertex( mesh, &PLVector3( x + w, y + h, 0 ), &pl_vecOrigin3, &PLColourRGB( 255, 255, 255 ), &PLVector2( tx + tw, ty + th ) );

	PlgAddMeshTriangle( mesh, vX, vY, vZ );
	PlgAddMeshTriangle( mesh, vZ, vY, vW );
}

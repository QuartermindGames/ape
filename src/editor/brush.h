/*
===========================================================================
Copyright (C) 1997-2006 Id Software, Inc.

This file is part of Quake 2 Tools source code.

Quake 2 Tools source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake 2 Tools source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake 2 Tools source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// brush.h

#pragma once

typedef struct {
	int		numpoints;
	int		maxpoints;
	float 	points[ 8 ][ 5 ];			// variable sized
} winding_t;


// the normals on planes point OUT of the brush
#define	MAXPOINTS	16
typedef struct face_s {
	struct face_s *next;
	vec3_t		planepts[ 3 ];
	texdef_t	texdef;

	plane_t		plane;

	winding_t *face_winding;

	vec3_t		d_color;
	qtexture_t *d_texture;

	//	int         d_numpoints;
	//	vec3_t     *d_points;
} face_t;

#define	MAX_FACES	16
class Brush {
public:
	Brush();
	Brush( vec3_t mins, vec3_t maxs, texdef_t *texdef );
	~Brush();

	Brush *prev{ nullptr }, *next{ nullptr };	// links in active/selected
	Brush *oprev{ nullptr }, *onext{ nullptr };	// links in entity
	struct entity_s *owner{ nullptr };
	vec3_t	mins, maxs;

	face_t *brush_faces{ nullptr };

	void AddToList( Brush *list );
	void RemoveFromList();

	void Build();
	void BuildWindings();

	Brush *Clone();

	void Draw( const huang::Viewport *viewport );

	winding_t *MakeFaceWinding( face_t *face );

	void Move( vec3_t move );

	face_t *Ray( vec3_t origin, vec3_t dir, float *dist );

	void SelectFaceForDragging( face_t *f, bool shear );
	void SetTexture( texdef_t *texdef );
	void SideSelect( vec3_t origin, vec3_t dir, bool shear );

	void RemoveEmptyFaces();

	static Brush *Parse();
	void Write( FILE *f );

private:
	void MakeFacePlanes();
	void DrawBrushEntityName();
	void SnapPlanepts();
};

void     Brush_MakeSided( int sides );
int        AddPlanept( float *f );
face_t *Face_Clone( face_t *f );
void       Face_Draw( face_t *face );

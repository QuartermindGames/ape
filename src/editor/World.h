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

// map.h -- the state of the current world that all views are displaying

#pragma once

#include <vector>

#include <PL/pl_graphics.h>

extern char currentmap[ PL_SYSTEM_MAX_PATH ];

// head/tail of doubly linked lists
extern Brush active_brushes;  // brushes currently being displayed
extern Brush selected_brushes;// highlighted
extern face_t *selected_face;
extern Brush *selected_face_brush;
extern Brush filtered_brushes;// brushes that have been filtered or regioned

extern entity_t entities;
extern entity_t *world_entity;// the world entity is NOT included in
                              // the entities chain

extern qboolean modified;// for quit confirmations

extern vec3_t region_mins, region_maxs;
extern qboolean region_active;

void Map_LoadFile( const char *filename );
void Map_SaveFile( const char *filename, qboolean use_region );
void Map_New( void );
void Map_BuildBrushData( void );

void Map_RegionOff( void );
void Map_RegionXY( void );
void Map_RegionTallBrush( void );
void Map_RegionBrush( void );
void Map_RegionSelectedBrushes( void );
qboolean Map_IsBrushFiltered( Brush *b );

entity_t *Map_FindClass( const char *cname );

namespace huang {
	class World {
	public:
		World();
		explicit World( const char *path );
		~World();

		void SaveFile( const char *path );

		entity_t *FindClass( const char *className );

		struct Face {
			int vertexIndices[ 32 ];
			unsigned int numEdges;

			char materialPath[ 32 ];
		};

		std::vector< PLVertex > vertices;
		std::vector< Face > faces;
		std::vector< entity_t > actors;

		std::vector< PLVertex * > selectedVertices;
		std::vector< Face * > selectedFaces;
		std::vector< entity_t * > selectedActors;

		/**
          * Returns true if nothing is selected.
          */
		bool IsSelectionEmpty() const {
			return ( selectedVertices.empty() &&
			         selectedFaces.empty() &&
			         selectedActors.empty() );
		}

		void ClearSelection() {
			selectedVertices.clear();
			selectedFaces.clear();
			selectedActors.clear();
		}

		bool isModified{ false };

		char name[ 64 ];
	};
}// namespace huang

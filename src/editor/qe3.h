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

#pragma once

// disable data conversion warnings for gl
#pragma warning( disable : 4244 )// MIPS
#pragma warning( disable : 4136 )// X86
#pragma warning( disable : 4051 )// ALPHA

#if defined( _WIN32 )
#include <windows.h>

#define Q_stricmp _stricmp
#define Q_unlink _unlink
#else
#include <unistd.h>

#define Q_stricmp strcasecmp
#define Q_unlink unlink
#endif

#include <algorithm>

#include <GL/glew.h>
#include <GL/glu.h>

#include <fx.h>
#include <fxkeys.h>
#include <fx3d.h>

#include <math.h>
#include <stdlib.h>
#include <stdint.h>

// Common Interface
#include "common/common.h"
#include "common/node.h"

#include <plcore/pl_console.h>

#include "cmdlib.h"
#include "mathlib.h"
#include "parse.h"
#include "lbmlib.h"

#if defined( _WIN32 )
#include <commctrl.h>
#include "afxres.h"
#include "resource.h"
#endif

#include "qedefs.h"

#include "Serialized.h"
#include "MainWindow.h"

typedef struct {
	vec3_t normal;
	double dist;
	int type;
} plane_t;

#include "qfiles.h"

#include "textures.h"
#include "CameraPerspective.h"
#include "brush.h"
#include "entity.h"
#include "World.h"
#include "select.h"

#include "xy.h"
#include "z.h"
#include "mru.h"

typedef struct {
	int p1, p2;
	face_t *f1, *f2;
} pedge_t;

typedef struct {
	char szProject[ 256 ];// last project loaded
	vec3_t colors[ COLOR_LAST ];
	FXbool show_names,
	        show_coordinates;
	int exclude;
} SavedInfo_t;

//
// system functions
//
void Sys_UpdateStatusBar();
void Sys_UpdateWindows( int bits );
void Sys_Printf( const char *text, ... );
double Sys_DoubleTime();
void Sys_GetCursorPos( int *x, int *y );
void Sys_SetCursorPos( int x, int y );
void Sys_SetTitle( char *text );
void Sys_BeginWait();
void Sys_EndWait();

/*
** most of the QE globals are stored in this structure
*/
struct QEGlobals_t {
	FXbool d_showgrid{ true };
	FXuint d_gridsize{ 8 };

	int d_num_entities{ 0 };

	entity_t *d_project_entity{ nullptr };

	float d_new_brush_bottom_z, d_new_brush_top_z;

	vec3_t d_points[ MAX_POINTS ];
	int d_numpoints{ 0 };

	pedge_t d_edges[ MAX_EDGES ];
	unsigned int d_numedges{ 0 };

	int d_num_move_points{ 0 };
	float *d_move_points[ 1024 ];

	qtexture_t *d_qtextures{ nullptr };

	texturewin_t d_texturewin;

	int d_pointfile_display_list;

	//LPMRUMENU    d_lpMruMenu;

	SavedInfo_t d_savedinfo;

	int d_workcount;

	// connect entities uses the last two brushes selected
	int d_select_count;
	Brush *d_select_order[ 2 ];
	vec3_t d_select_translate;// for dragging w/o making new display lists
	select_t d_select_mode;

	int d_font_list;

	int d_parsed_brushes;

	qboolean show_blocks;
};

void *qmalloc( size_t size );
char *copystring( char *s );
char *ExpandReletivePath( char *p );

//
// drag.c
//
void Drag_Begin( int x, int y, const bool buttons[],
                 vec3_t xaxis, vec3_t yaxis,
                 vec3_t origin, vec3_t dir );
void Drag_MouseMoved( int x, int y, const bool buttons[] );
void Drag_MouseUp( void );

//
// csg.c
//
void CSG_MakeHollow( void );
void CSG_Subtract( void );

//
// vertsel.c
//

void SetupVertexSelection( void );
void SelectEdgeByRay( vec3_t org, vec3_t dir );
void SelectVertexByRay( vec3_t org, vec3_t dir );

void ConnectEntities( void );

extern int update_bits;

extern void *bsp_process;

char *TranslateString( char *buf );

void FillTextureMenu( void );

//
// entityw.c
//
void FillClassList( void );
BOOL UpdateEntitySel( eclass_t *pec );
void SetInspectorMode( int iType );
void SetSpawnFlags( void );
void GetSpawnFlags( void );
void SetKeyValuePairs( void );
extern void BuildGammaTable( float g );


// win_dlg.c

void DoFind( void );
void DoRotate( void );
void DoSides( void );
void DoAbout( void );
void DoSurface( void );

/*
** QE function declarations
*/
void QE_CheckAutoSave( void );
void QE_ConvertDOSToUnixName( char *dst, const char *src );
void QE_CountBrushesAndUpdateStatusBar( void );
void QE_CheckOpenGLForErrors( void );
void QE_ExpandBspString( char *bspaction, char *out, char *mapname );
void QE_Init( void );
qboolean QE_KeyDown( int key );
qboolean QE_LoadProject( const char *projectfile );
qboolean QE_SingleBrush( void );

/*
** extern declarations
*/
extern QEGlobals_t g_qeglobals;

namespace huang {
	namespace util {
		const char *GetMaterialsDirectory();
		const char *GetWorldsDirectory();

		FXIcon *LoadImageIcon( FXApp *app, const char *path );

		enum class MenuType {
			COMMAND,
			CHECKBOX,
			SEPERATOR,
			RADIO,
		};

		struct MenuItem {
			const char *label{ nullptr };
			MenuType type{ MenuType::COMMAND };
			FXSelector selector{ 0 };
			FXObject *target{ nullptr };
			FXIcon *icon{ nullptr };
		};

		FXMenuPane *CreateMenus( FXApp *app, FXMenuBar *menuBar, const char *menuName, MenuItem *items );

		namespace reg {
			const char *ReadString( const char *section, const char *key, const char *def = "" );
			int ReadInt( const char *section, const char *key, int def = 0 );
			bool ReadBool( const char *section, const char *key, bool def = false );
			float ReadFloat( const char *section, const char *key, float def = 0.0f );
			FXColor ReadColour( const char *section, const char *key, FXColor def = 0u );
			int ReadColourF( const char *section, const char *key, vec3_t out, const vec3_t def = vec3_origin );

			bool WriteString( const char *section, const char *key, const char *value );
			bool WriteInt( const char *section, const char *key, int value );
			bool WriteBool( const char *section, const char *key, bool value );
			bool WriteFloat( const char *section, const char *key, float value );
			bool WriteColour( const char *section, const char *key, FXColor value );
			bool WriteColourF( const char *section, const char *key, const vec3_t value );
		}// namespace reg
	}    // namespace util
}// namespace huang

#define FX_EVENT_FUNC( NAME ) long NAME( FXObject *, FXSelector, void * )

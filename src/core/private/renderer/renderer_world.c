// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#include "qmos/public/qm_os_random.h"

#include "ape_private.h"
#include "renderer.h"
#include "renderer_texture.h"
#include "material/material.h"
#include "camera/camera.h"
#include "editor/editor.h"
#include "world/world.h"

//TODO: eventually we should do away with this!!!
static constexpr unsigned int MAX_MATERIALS_PER_PASS = 256;
static constexpr unsigned int MAX_SUB_MESHES         = 8192;

//TODO: eventually we should do away with this!!!
typedef struct DisplayList
{
	ApeMaterial *material;
	int          subMeshes[ MAX_SUB_MESHES ];
	int          firstSubMeshes[ MAX_SUB_MESHES ];
	int          numSubMeshes;
} DisplayList;
static DisplayList  displayLists[ MAX_MATERIALS_PER_PASS ];
static unsigned int numDisplayLists;

static bool showHiddenFaces;

void ape_renderer_world_register_console_variables_()
{
	PlRegisterConsoleVariable( "renderer_world.showHiddenFaces", "Toggle hidden faces.", "false", PL_VAR_BOOL, &showHiddenFaces, nullptr, false );
}

static void draw_face_wireframe( const ApeBrushFace *face )
{
	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		const ApeBrushFaceVertex *a = &face->vertices[ face->edgeLoopOrder[ i ] ];
		PlgImmPushVertex( brush->vertices[ a->posIndex ].x, brush->vertices[ a->posIndex ].y, brush->vertices[ a->posIndex ].z );
		if ( ape_brush_face_is_portal( face ) )
		{
			PlgImmColour( 255, 0, 255, 255 );
		}
		else
		{
			PlgImmColour( 255, 255, 255, 255 );
		}

		const ApeBrushFaceVertex *b = i + 1 < face->numVertices ? &face->vertices[ face->edgeLoopOrder[ i + 1 ] ] : &face->vertices[ face->edgeLoopOrder[ 0 ] ];
		PlgImmPushVertex( brush->vertices[ b->posIndex ].x, brush->vertices[ b->posIndex ].y, brush->vertices[ b->posIndex ].z );
		if ( ape_brush_face_is_portal( face ) )
		{
			PlgImmColour( 255, 0, 255, 255 );
		}
		else
		{
			PlgImmColour( 255, 255, 255, 255 );
		}
	}
}

static void draw_room_wireframe( const ApeCamera *camera )
{
#if 0//TODO
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgImmBegin( PLG_MESH_LINES );

	unsigned int   numFaces;
	ApeBrushFace **faces = ( ApeBrushFace ** ) PlGetVectorArrayDataEx( camera->pvs.faces, &numFaces );
	for ( unsigned int i = 0; i < numFaces; ++i )
	{
		draw_face_wireframe( faces[ i ] );
	}

	PlgImmDraw();
#endif
}

/**
 * World is drawn using polygons, rather than straight up triangles,
 * so to more accurately display it in wireframe, we'll need to render
 * it in such a mode ourselves. This is mostly for the sake of the
 * editor.
 */
void ape_world_draw_wireframe_( ApeWorld *world, ApeCamera *camera )
{
	assert( ( camera != NULL ) && ( world != NULL ) );

	const ApeRoom *room = ape_camera_get_room( camera );
	if ( room == nullptr )
	{
		return;
	}

	draw_room_wireframe( camera );
}

static void build_selection_display_list( ApeWorldNode *node, ApeEditorInstance *instance, unsigned int *offset )
{
	if ( node->type != APE_WORLD_NODE_TYPE_BRUSH )
	{
		return;
	}

	const ApeBrush *brush = ( ApeBrush * ) node;
	for ( unsigned int i = 0; i < brush->numFaces; *offset += brush->faces[ i ].numVertices, ++i )
	{
		assert( displayLists[ 0 ].numSubMeshes < MAX_SUB_MESHES );
		if ( displayLists[ 0 ].numSubMeshes >= MAX_SUB_MESHES )
		{
			ape_console_warning_( "Hit submesh limit for draw, will squeeze into another batch!\n" );
			break;
		}

		const ApeBrushFace *face = &brush->faces[ i ];
		if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
		{
			void *p;
			QM_OS_LINKED_LIST_ITERATE( p, instance->selectedObjects, i )
			{
				if ( ( ApeBrushFace * ) p == face )
				{
					if ( ape_camera_test_aabb( instance->camera, &face->bounds ) )
					{
						displayLists[ 0 ].subMeshes[ displayLists[ 0 ].numSubMeshes ]      = face->numVertices;
						displayLists[ 0 ].firstSubMeshes[ displayLists[ 0 ].numSubMeshes ] = *offset;
						displayLists[ 0 ].numSubMeshes++;
					}

					break;
				}
			}
		}
	}
}

void ape_room_draw_selected_( ApeRoom *room, ApeEditorInstance *instance )
{
	if ( qm_os_linked_list_get_size( instance->selectedObjects ) == 0 )
	{
		return;
	}

	displayLists[ 0 ] = ( DisplayList ) {};

	unsigned int offset = 0;
	build_selection_display_list( APE_WORLD_NODE( room ), instance, &offset );

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, APE_WORLD_NODE( room )->children, i )
	{
		build_selection_display_list( child, instance, &offset );
	}

	if ( displayLists[ 0 ].numSubMeshes == 0 )
	{
		return;
	}

	PLGMesh *mesh        = APE_WORLD_NODE( room )->mesh;
	mesh->numSubMeshes   = displayLists[ 0 ].numSubMeshes;
	mesh->firstSubMeshes = displayLists[ 0 ].firstSubMeshes;
	mesh->subMeshes      = displayLists[ 0 ].subMeshes;

	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR_SELECTION );
	assert( material != nullptr );
	ape_material_draw( material, mesh, &ape_rendererState_ );

	ape_rendererPerformance_.numFacesDrawn += mesh->numSubMeshes;

	mesh->numSubMeshes = displayLists[ 0 ].numSubMeshes = 0;
}

static void build_brush_display_list( ApeWorldNode *node, const ApeLight *light, const ApeCamera *camera, const ApeCameraVisibleRoom *visibleRoom, unsigned int *offset, ApeRendererPassFlag stage )
{
	if ( node->flags == APE_WORLD_NODE_FLAG_HIDDEN || node->type != APE_WORLD_NODE_TYPE_BRUSH )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	const ApeBrush *brush = ( ApeBrush * ) node;
	for ( unsigned int i = 0; i < brush->numFaces; *offset += brush->faces[ i ].numVertices, ++i )
	{
		ApeBrushFace *face = &brush->faces[ i ];

		// horrible linear lookup to figure out which display list to shove this into
		DisplayList *list = nullptr;
		for ( unsigned int j = 0; j < numDisplayLists; ++j )
		{
			if ( displayLists[ j ].material == face->material )
			{
				list = &displayLists[ j ];
				break;
			}
		}

		if ( list == nullptr )
		{
			continue;
		}

		assert( list->numSubMeshes < MAX_SUB_MESHES );
		if ( list->numSubMeshes >= MAX_SUB_MESHES )
		{
			ape_console_warning_( "Hit submesh limit for draw, will squeeze into another batch!\n" );
			break;
		}

		if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SHADED && stage & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS && face->lightmapIndex != ape_rendererState_.lightmapIndex )
		{
			continue;
		}

		// check the face isn't the same face we're looking in from
		if ( visibleRoom->entrance != nullptr && visibleRoom->entrance->portalFace == face )
		{
			continue;
		}

		if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN && !showHiddenFaces )
		{
			continue;
		}

		if ( ape_brush_face_is_portal( face ) && !( stage & APE_RENDERER_PASS_FLAG_TRANSLUCENT ) )
		{
			continue;
		}

		if ( light != nullptr )
		{
			QmMathVector3f lightPos = ape_light_get_position( light );
			if ( light->type == APE_LIGHT_TYPE_OMNI && !PlIsSphereIntersectingAabb( &PlSetupCollisionSphere( lightPos, light->radius ), &face->bounds ) )
			{
				continue;
			}

#if 0// ditched for speed...
			PLCollisionPlane plane = { .normal = face->normal, .origin = face->bounds.absOrigin };
			if ( ape_light_test_plane_shadow( light, material, &plane ) )
			{
				continue;
			}
#endif
		}

		if ( ape_camera_pvs_test_brush_face_( camera, face ) )
		{
			list->subMeshes[ list->numSubMeshes ]      = face->numVertices;
			list->firstSubMeshes[ list->numSubMeshes ] = *offset;
			list->numSubMeshes++;
		}
	}

	COM_PROFILE_FUNCTION_END();
}

static void draw_brushes( ApeWorldNode *worldNode, const ApeCameraVisibleRoom *visibleRoom, ApeCamera *camera, ApeLight *light, const ApeRendererPassFlag flags )
{
	COM_PROFILE_FUNCTION_START();

	//TODO: this is operating off a universal list, should only operate on *world* materials!!!
	PLLinkedList *materialList = ape_material_get_group_( APE_CACHE_GROUP_WORLD );
	assert( materialList != nullptr );

	numDisplayLists = 0;

	// setup the display lists
	ApeMaterial *material;
	COM_ITERATE_LINKED_LIST( material, materialList, i )
	{
		// blended materials get drawn later
		if ( ( flags & APE_RENDERER_PASS_FLAG_TRANSLUCENT && !ape_material_is_blended( material ) ) || ( flags & APE_RENDERER_PASS_FLAG_OPAQUE && ape_material_is_blended( material ) ) )
		{
			continue;
		}

		displayLists[ numDisplayLists ].material = material;
		numDisplayLists++;
	}

	unsigned int offset = 0;
	build_brush_display_list( worldNode, light, camera, visibleRoom, &offset, flags );

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, worldNode->children, i )
	{
		build_brush_display_list( child, light, camera, visibleRoom, &offset, flags );
	}

	PL_GET_CVAR( "renderer.showNormals", showNormals );
	for ( unsigned int i = 0; i < numDisplayLists; ++i )
	{
		if ( displayLists[ i ].numSubMeshes == 0 )
		{
			continue;
		}

		PLGMesh *mesh        = worldNode->mesh;
		mesh->numSubMeshes   = displayLists[ i ].numSubMeshes;
		mesh->firstSubMeshes = displayLists[ i ].firstSubMeshes;
		mesh->subMeshes      = displayLists[ i ].subMeshes;

		if ( showNormals->b_value )
		{
			displayLists[ i ].material = ape_material_get_default( APE_MATERIAL_DEFAULT_DEBUG_NORMALS );
		}

		ape_material_draw( displayLists[ i ].material, mesh, &ape_rendererState_ );

		ape_rendererPerformance_.numFacesDrawn += mesh->numSubMeshes;

		mesh->numSubMeshes = displayLists[ i ].numSubMeshes = 0;
	}

	COM_PROFILE_FUNCTION_END();
}

static void draw_node_meshes( ApeWorldNode *worldNode, const ApeCameraVisibleRoom *visibleRoom, ApeCamera *camera, ApeLight *light, const ApeRendererPassFlag flags )
{
	ape_world_node_update_mesh_cache_( worldNode );

	if ( worldNode->mesh != nullptr )
	{
		qm_gfx_debug_push_group_marker( "Node Mesh Draw" );

		PlMatrixMode( PL_MODELVIEW_MATRIX );
		PlPushMatrix();

		PLMatrix4 transform = ape_world_node_get_transform( worldNode );
		PlLoadMatrix( &transform );

		if ( flags & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS && camera->drawMode == APE_CAMERA_DRAW_MODE_SHADED )
		{
			// yes, this is very inefficient - this is just temporary!
			// we'll update this to use texture arrays for the other lightmaps
			// in future so we can do all this in one draw as before, but that
			// will require expanding our graphics abstraction a bit to expose
			// support for it...
			ApeRoom *room = visibleRoom->room;
			for ( unsigned int i = 0; i < room->numLightmaps; ++i )
			{
				assert( room->lightmaps[ i ] != nullptr );

				ape_rendererState_.lightmapTexture = room->lightmaps[ i ]->texture->internal;
				ape_rendererState_.lightmapIndex   = i;

				draw_brushes( worldNode, visibleRoom, camera, light, flags );
			}

			// now draw the last set without an assigned lightmap
			ape_rendererState_.lightmapTexture = nullptr;
			ape_rendererState_.lightmapIndex   = APE_BRUSH_FACE_LIGHTMAP_INVALID;
		}

		draw_brushes( worldNode, visibleRoom, camera, light, flags );

		PlPopMatrix();

		qm_gfx_debug_pop_group_marker();
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, worldNode->children, i )
	{
		draw_node_meshes( child, visibleRoom, camera, light, flags );
	}
}

void ape_model_draw_models( ApeRoom *room, const ApeCamera *camera, const ApeRendererPassState *state );

static void draw_room( ApeCamera *camera, const ApeCameraVisibleRoom *visibleRoom, ApeLight *light, const ApeRendererPassFlag flags )
{
	if ( !( flags & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS ) && light == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	ApeRoom *room = visibleRoom->room;
	if ( flags & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS )
	{
		ape_rendererState_.lighting.ambience = QM_MATH_COLOUR4F_TO_3F( room->ambientLight );
	}

	// recurse down the tree to draw all nodes with explicit meshes
	draw_node_meshes( APE_WORLD_NODE( room ), visibleRoom, camera, light, flags );

	//TODO: botch, we don't check render pass flag under draw models yet
	if ( !( flags & APE_RENDERER_PASS_FLAG_TRANSLUCENT ) )
	{
		ape_model_draw_models( room, camera, &ape_rendererState_ );
	}

	if ( flags & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS )
	{
		ape_rendererState_.lighting.ambience = ( QmMathColour3f ) {};
	}

	COM_PROFILE_FUNCTION_END();
}

/////////////////////////////////////////////////////////////////////////////////////
// Stencil Shadows
/////////////////////////////////////////////////////////////////////////////////////

static constexpr float F_INFINITY = 10000.0f;

static QmMathVector3f get_shadow_projection( const ApeLight *light, const QmMathVector3f *vertex, const QmMathVector3f *faceOrigin )
{
	QmMathVector3f pos = ape_world_node_get_position( APE_WORLD_NODE( light ) );
	if ( light->type == APE_LIGHT_TYPE_SUN )
	{
		return qm_math_vector3f_scale_float( qm_math_vector3f_normalize( pos ), F_INFINITY );
	}
	if ( light->type == APE_LIGHT_TYPE_SPOT )
	{
		return qm_math_vector3f_scale_float( *vertex, light->radius );
	}

#if 0// this doesn't work right now...

	/* Because brushes are fairly primitive, this code needs to determine the extent relative to the face projection,
	 * otherwise the vertices at both extreme ends will hit the extent of the light radius and the shadow wont actually
	 * project for the total extent - leaving you with weirdly cut-off shadows.
	 */

	// first determine the project from the face origin

	static constexpr float F_PI = 4.0f / 3.0f * QM_MATH_PI;

	float c = light->radius * light->radius * light->radius;
	float r = powf( F_PI * c, 1.0f / 3.0f );

	float range = qm_math_vector3f_length( qm_math_vector3f_normalize( qm_math_vector3f_sub( *faceOrigin, pos ) ) );
	if ( range > r )
	{
		range = r;
	}

	range = r - range;

	QmMathVector3f fdir = qm_math_vector3f_normalize( qm_math_vector3f_sub( *faceOrigin, pos ) );
	QmMathVector3f fpos = qm_math_vector3f_add( *faceOrigin, qm_math_vector3f_scale_float( fdir, range ) );

	QmMathVector3f vdir = qm_math_vector3f_normalize( qm_math_vector3f_sub( *vertex, pos ) );
	QmMathVector3f vpos = qm_math_vector3f_add( *vertex, qm_math_vector3f_scale_float( vdir, range ) );

	ape_draw_debug_arrow( *faceOrigin, fpos, PL_COLOUR_GREEN, 1.0f );
	ape_draw_debug_arrow( *vertex, vpos, PL_COLOUR_RED, 1.0f );

	return qm_math_vector3f_add( *vertex, vpos );

#elif 0// this restricts each vertex to the extent of the light radius

	QmMathVector3f s = qm_math_vector3f_normalize( qm_math_vector3f_sub( *vertex, pos ) );
	float          r = qm_math_vector3f_length( qm_math_vector3f_sub( *vertex, pos ) );
	if ( r > light->radius )
	{
		r = light->radius;
	}

	return qm_math_vector3f_add( *vertex, qm_math_vector3f_scale_float( s, light->radius - r ) );

#else// this restricts each vertex to a volume derived from the light radius

	static constexpr float F_PI = 4.0f / 3.0f * QM_MATH_PI;

	float          c = light->radius * light->radius * light->radius;
	float          r = powf( F_PI * c, 1.0f / 3.0f );
	QmMathVector3f s = qm_math_vector3f_normalize( qm_math_vector3f_sub( *vertex, pos ) );
	return qm_math_vector3f_add( *vertex, qm_math_vector3f_scale_float( s, r ) );

#endif
}

static void draw_brush_stencil_shadow_cap( const ApeBrushFace *face, const ApeLight *light, bool start, unsigned int *indices )
{
	ApeBrush *brush = face->parent;
	assert( brush != nullptr );

	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		const ApeBrushFaceVertex *vertex = &face->vertices[ face->edgeLoopOrder[ i ] ];

		// get the projected position (if start, just uses the initial position)
		const QmMathVector3f ppos = start ? brush->vertices[ vertex->posIndex ] : get_shadow_projection( light, &brush->vertices[ vertex->posIndex ], &face->bounds.absOrigin );

		indices[ i ] = PlgImmPushVertex( ppos.x, ppos.y, ppos.z );
	}

	for ( unsigned int i = 1; i + 1 < face->numVertices; ++i )
	{
		PlgImmPushTriangle( indices[ 0 ], indices[ start ? i : ( i + 1 ) ], indices[ start ? ( i + 1 ) : i ] );
	}
}

static void draw_node_shadow_volume( ApeWorldNode *node, const ApeLight *light, PLGMesh *mesh, unsigned int *numIndices )
{
#if 0// projection on gpu
if ( !build_shadow_display_list( room, light ) )
{
	return;
}

draw_room_submesh( room->mesh, shadowMaterial, 0, light );
#endif

	if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		const ApeBrush *brush = ( ApeBrush * ) node;
		for ( unsigned int i = 0; i < brush->numFaces; ++i )
		{
			const ApeBrushFace *face = &brush->faces[ i ];
			if ( /*face->flags & APE_BRUSH_FACE_FLAG_HIDDEN ||*/ !ape_material_can_cast_shadows( face->material ) )
			{
				continue;
			}

			PLCollisionPlane plane = ( PLCollisionPlane ) { .normal = brush->faces[ i ].normal, .origin = brush->faces[ i ].bounds.absOrigin };
			if ( ape_light_test_plane( light, &plane ) )
			{
				continue;
			}

			//todo: this check should probably be integrated into light_test_plane...
			QmMathVector3f lightPos = ape_light_get_position( light );
			if ( light->type == APE_LIGHT_TYPE_OMNI && !PlIsSphereIntersectingAabb( &PlSetupCollisionSphere( lightPos, light->radius ), &brush->faces[ i ].bounds ) )
			{
				continue;
			}

			// There's probably a more efficient way of doing this,
			// but let's go ahead and store all the indices into a dynamic array
			*numIndices += face->numVertices * 2;// * 2 for edges
			static unsigned int *indices    = nullptr;
			static unsigned int  maxIndices = 0;
			if ( indices == NULL )
			{
				maxIndices = ( *numIndices * brush->numFaces );
				indices    = QM_OS_MEMORY_NEW_( unsigned int, maxIndices );
			}
			else if ( *numIndices > maxIndices )
			{
				maxIndices = *numIndices + 16;
				indices    = qm_os_memory_realloc( indices, sizeof( unsigned int ) * maxIndices );
			}

			unsigned int *fl = &indices[ *numIndices - ( face->numVertices * 2 ) ];
			draw_brush_stencil_shadow_cap( face, light, false, fl );
			unsigned int *sl = &indices[ *numIndices - face->numVertices ];
			draw_brush_stencil_shadow_cap( face, light, true, sl );

			// Now produce the edges
			for ( int j = 0; j < face->numVertices; j++ )
			{
				PlgImmPushTriangle( fl[ j ], fl[ ( j + 1 ) % face->numVertices ], sl[ j ] );
				PlgImmPushTriangle( sl[ j ], fl[ ( j + 1 ) % face->numVertices ], sl[ ( j + 1 ) % face->numVertices ] );
			}
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		draw_node_shadow_volume( child, light, mesh, numIndices );
	}
}

void ape_world_draw_stencil_shadows_( ApeCamera *camera, const ApeLight *light )
{
	assert( camera != nullptr && light != nullptr );

	ApeRoom *room = ape_camera_get_room( camera );
	if ( room == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	//todo: new (temporary) stuff!
	//todo: cache!!!!

	unsigned int numIndices = 0;
	PLGMesh     *mesh       = PlgImmBegin( PLG_MESH_TRIANGLES );

	draw_node_shadow_volume( APE_WORLD_NODE( room ), light, mesh, &numIndices );

	if ( numIndices > 0 )
	{
		ApeMaterial *shadowMaterial = ape_material_get_default( APE_MATERIAL_DEFAULT_SHADOW );
		assert( shadowMaterial != NULL );
		ape_material_draw( shadowMaterial, mesh, &ape_rendererState_ );
	}

	PlPopMatrix();

	COM_PROFILE_FUNCTION_END();
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

static void draw_translucent_room( ApeCamera *camera, const ApeCameraVisibleRoom *visibleRoom )
{
	COM_PROFILE_FUNCTION_START();

	qm_gfx_debug_push_group_marker( "Translucent Room" );

	// and now depth pre-pass
	draw_room( camera, visibleRoom, nullptr, APE_RENDERER_PASS_FLAG_DEPTH_PREPASS | APE_RENDERER_PASS_FLAG_TRANSLUCENT );

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SHADED )
	{
		ape_rendererState_.overrideDepthMask = true;
		ape_rendererState_.depthMask         = false;

		for ( unsigned int i = 0; i < visibleRoom->numLights; ++i )
		{
			ApeLight *light = visibleRoom->lights[ i ];
			if ( !( light->flags & APE_LIGHT_FLAG_DYNAMIC && light->flags & APE_LIGHT_FLAG_ENABLED ) )
			{
				continue;
			}

			//TODO: viewport clipping per light volume

			ape_rendererState_.overrideBlendMode = true;
			ape_rendererState_.blendModeA        = PLG_BLEND_ONE;
			ape_rendererState_.blendModeB        = PLG_BLEND_ONE;

			draw_room( camera, visibleRoom, visibleRoom->lights[ i ], APE_RENDERER_PASS_FLAG_TRANSLUCENT );

			ape_rendererState_.overrideBlendMode = false;
		}

		ape_rendererState_.overrideDepthMask = false;
		ape_rendererState_.depthMask         = true;
	}

	qm_gfx_debug_pop_group_marker();

	COM_PROFILE_FUNCTION_END();
}

static void draw_solid_room_lit( const ApeCameraVisibleRoom *visibleRoom, ApeCamera *camera, ApeLight *light, bool depth )
{
	if ( light->colour.a <= 0.0f )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	//TODO: viewport clipping per light volume, there was some code below for it but I've scrapped it for now

	const bool drawShadows = !depth && ape_config_.renderer.useStencilShadowVolumes && ape_light_get_shadow_type( light ) == APE_LIGHT_SHADOW_TYPE_DYNAMIC;
	if ( drawShadows )
	{
		ape_rendererState_.cullMode = APE_RENDERER_CULL_MODE_NONE;

		if ( ape_config_.renderer.showShadowWireframe )
		{
			PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
			ape_world_draw_stencil_shadows_( camera, light );
			PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
		}

		PlgClearBuffers( PLG_BUFFER_STENCIL );

		PlgEnableGraphicsState( PLG_GFX_STATE_STENCILTEST );
		PlgEnableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
		PlgColourMask( false, false, false, false );

		PlgStencilBufferFunction( PLG_COMPARE_ALWAYS, 0x0, 0xFF );
		PlgStencilOp( PLG_STENCIL_FACE_FRONT, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_INCRWRAP, PLG_STENCIL_OP_KEEP );
		PlgStencilOp( PLG_STENCIL_FACE_BACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_DECRWRAP, PLG_STENCIL_OP_KEEP );

		ape_world_draw_stencil_shadows_( camera, light );

		PlgDisableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
		PlgColourMask( true, true, true, true );

		PlgStencilBufferFunction( PLG_COMPARE_EQUAL, 0x0, 0xFF );
		PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP );

		ape_rendererState_.cullMode = APE_RENDERER_CULL_MODE_DEFAULT;
	}

	ape_rendererState_.overrideBlendMode = true;
	ape_rendererState_.blendModeA        = PLG_BLEND_ONE;
	ape_rendererState_.blendModeB        = PLG_BLEND_ONE;

	draw_room( camera, visibleRoom, light, APE_RENDERER_PASS_FLAG_OPAQUE );

	ape_rendererState_.overrideBlendMode = false;

	if ( drawShadows )
	{
		if ( !depth )
		{
			PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );
		}
	}

	COM_PROFILE_FUNCTION_END();
}

static void draw_solid_room( ApeCamera *camera, const ApeCameraVisibleRoom *visibleRoom, bool depth )
{
	COM_PROFILE_FUNCTION_START();

	qm_gfx_debug_push_group_marker( "Solid Room" );

	// and now depth pre-pass
	draw_room( camera, visibleRoom, nullptr, APE_RENDERER_PASS_FLAG_DEPTH_PREPASS | APE_RENDERER_PASS_FLAG_OPAQUE );

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SHADED )
	{
		qm_gfx_debug_push_group_marker( "Shaded" );

		ape_rendererState_.overrideDepthMask = true;
		ape_rendererState_.depthMask         = false;

		for ( unsigned int i = 0; i < visibleRoom->numLights; ++i )
		{
			ApeLight *light = visibleRoom->lights[ i ];
			if ( !( light->flags & APE_LIGHT_FLAG_DYNAMIC && light->flags & APE_LIGHT_FLAG_ENABLED ) )
			{
				continue;
			}

			draw_solid_room_lit( visibleRoom, camera, light, depth );
		}

		ape_rendererState_.overrideDepthMask = false;
		ape_rendererState_.depthMask         = depth;

		qm_gfx_debug_pop_group_marker();
	}

	qm_gfx_debug_pop_group_marker();

	COM_PROFILE_FUNCTION_END();
}

static void draw_portal_face( const ApeBrushFace *portal, bool useMaterial )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( portal->parent ) );
	PlMultiMatrix( &transform );

	ApeBrush *brush = portal->parent;
	assert( brush != nullptr );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
	for ( unsigned int j = 0; j < portal->numVertices; ++j )
	{
		const ApeBrushFaceVertex *vertex = &portal->vertices[ portal->edgeLoopOrder[ j ] ];
		//TODO: handle transforms for the brush in software here
		PlgImmPushVertex( brush->vertices[ vertex->posIndex ].x, brush->vertices[ vertex->posIndex ].y, brush->vertices[ vertex->posIndex ].z );
		PlgImmColour( 0, 0, 0, 0 );
	}

	if ( useMaterial )
	{
		ape_material_draw( portal->material, mesh, &ape_rendererState_ );
	}
	else
	{
		ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_VERTEX );
		ape_material_draw( material, mesh, &ape_rendererState_ );
	}

	PlPopMatrix();
}

static void draw_wireframe_portal_face( const ApeBrushFace *portal )
{
	ApeBrush *brush = portal->parent;
	assert( brush != nullptr );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( portal->parent ) );
	PlMultiMatrix( &transform );

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgImmBegin( PLG_MESH_LINE_LOOP );

	for ( unsigned int j = 0; j < portal->numVertices; ++j )
	{
		const ApeBrushFaceVertex *vertex = &portal->vertices[ portal->edgeLoopOrder[ j ] ];
		PlgImmPushVertex( brush->vertices[ vertex->posIndex ].x, brush->vertices[ vertex->posIndex ].y, brush->vertices[ vertex->posIndex ].z );
		PlgImmColour( 0, 255, 0, 255 );
	}

	PlgImmDraw();

	PlPopMatrix();
}

static float sgn( const float a )
{
	if ( a > 0.0f ) return 1.0f;
	if ( a < 0.0f ) return -1.0f;
	return 0.0f;
}

// based on https://terathon.com/blog/oblique-clipping.html
static PLMatrix4 modify_portal_projection_matrix( const PLMatrix4 *projMatrix, const QmMathVector4f *plane )
{
	// Calculate the clip-space corner point opposite the clipping plane
	// as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
	// transform it into camera space by multiplying it
	// by the inverse of the projection matrix
	QmMathVector4f q = qm_math_vector4f( ( sgn( plane->x ) + projMatrix->m[ 8 ] ) / projMatrix->m[ 0 ],
	                                     ( sgn( plane->y ) + projMatrix->m[ 9 ] ) / projMatrix->m[ 5 ],
	                                     -1.0f,
	                                     ( 1.0f + projMatrix->m[ 10 ] ) / projMatrix->m[ 14 ] );

	// Calculate the scaled plane vector
	QmMathVector4f c = qm_math_vector4f_scale_float( *plane, 2.0f / qm_math_vector4f_dot_product( *plane, q ) );

	// Replace the third row of the projection matrix
	PLMatrix4 matrix = *projMatrix;
	matrix.m[ 2 ]    = c.x;
	matrix.m[ 6 ]    = c.y;
	matrix.m[ 10 ]   = c.z + 1.0f;
	matrix.m[ 14 ]   = c.w;

	return matrix;
}

static void draw_portal( ApeCamera *camera, const ApeViewport *viewport, const ApeCameraVisibleRoom *visibleRoom, const ApeCameraVisiblePortal *visiblePortal )
{
	if ( visiblePortal->nextRoom == nullptr )
	{
		return;
	}

	ApeCameraVisibleRoom *nextVisibleRoom = visiblePortal->nextRoom;
	if ( nextVisibleRoom->room == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	ApeBrushFace *portal            = visiblePortal->portalFace;
	ApeBrushFace *destinationPortal = ape_brush_face_get_portal_destination( portal );

	qm_gfx_debug_push_group_marker( "portal" );

	// first draw the portal stencil we'll test again

	qm_gfx_clip_viewport( visiblePortal->screenRect.x, visiblePortal->screenRect.y,
	                      visiblePortal->screenRect.z, visiblePortal->screenRect.w );

	PlgEnableGraphicsState( PLG_GFX_STATE_STENCILTEST );
	PlgStencilBufferFunction( PLG_COMPARE_ALWAYS, 4, 0xFF );
	PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_REPLACE );

	PlgColourMask( false, false, false, false );

	draw_portal_face( portal, false );

	PlgStencilBufferFunction( PLG_COMPARE_EQUAL, 4, 0xFF );
	PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP );

	//TODO: uuuuuhhhhhgggg
	PlgClearBuffers( PLG_BUFFER_DEPTH );

	PlgColourMask( true, true, true, true );

	// setup clipping plane

	ape_rendererState_.mirror = ape_brush_face_is_mirror( visiblePortal->portalFace );

	QmMathVector4f clipPlane;
	if ( ape_rendererState_.mirror )
	{
		clipPlane = qm_math_vector4f( visiblePortal->normal.x, visiblePortal->normal.y, visiblePortal->normal.z,
		                              -qm_math_vector3f_dot_product( visiblePortal->normal, visiblePortal->origin ) );
	}
	else
	{
		clipPlane = qm_math_vector4f( -visiblePortal->normal.x, -visiblePortal->normal.y, -visiblePortal->normal.z,
		                              -qm_math_vector3f_dot_product( visiblePortal->normal, visiblePortal->origin ) );
	}

	PLMatrix4 clipMatrix = PlMatrix4Identity();
	qm_gfx_set_clip_plane( &clipPlane, &clipMatrix, false );

	// setup the projection matrix
	PLMatrix4 oldProj = camera->proj;
#if 0// TODO: of course... this isn't quite right
	camera->proj      = qm_math_matrix4_oblique( &camera->proj, clipPlane );
#endif

	// now recurse into the next room

	ape_rendererState_.depth++;

	// set the view matrix we need
	camera->view = nextVisibleRoom->viewMatrix;
	PlgSetViewMatrix( &nextVisibleRoom->viewMatrix );
	ape_camera_setup_frustum( camera );

	ape_room_draw_( camera, nextVisibleRoom, viewport );

	// restore the projection matrix
	camera->proj = oldProj;

	// reset the view matrix back
	camera->view = visibleRoom->viewMatrix;
	PlgSetViewMatrix( &camera->view );
	ape_camera_setup_frustum( camera );

	// and pop out

	ape_rendererState_.depth--;
	ape_rendererState_.mirror = false;

	qm_gfx_set_clip_plane( nullptr, nullptr, false );

	// depth buffer pop

	PlgColourMask( false, false, false, false );

	ape_rendererState_.overrideDepthMask = true;
	ape_rendererState_.depthMask         = true;

	draw_portal_face( portal, true );

	ape_rendererState_.overrideDepthMask = false;

	PlgColourMask( true, true, true, true );

	PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );

	// reset the viewport
	ape_viewport_set_clip( viewport );

	qm_gfx_debug_pop_group_marker();

	COM_PROFILE_FUNCTION_END();
}

static void draw_room_editor( const ApeCameraVisibleRoom *visibleRoom )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	for ( unsigned int i = 0; i < visibleRoom->numNodes; ++i )
	{
		ApeWorldNode *node = visibleRoom->nodes[ i ];
		if ( node == nullptr || node->classType->onDrawEditor == nullptr )
		{
			continue;
		}

		node->classType->onDrawEditor( node, false );
	}

	// sigh...
	for ( unsigned int i = 0; i < visibleRoom->numLights; ++i )
	{
		ApeWorldNode *node = APE_WORLD_NODE( visibleRoom->lights[ i ] );
		if ( node == nullptr || node->classType->onDrawEditor == nullptr )
		{
			continue;
		}

		node->classType->onDrawEditor( node, false );
	}

	ape_editor_selection_render_post_( instance );

	COM_PROFILE_FUNCTION_END();
}

//TODO: move into room code
void ape_room_draw_( ApeCamera *camera, ApeCameraVisibleRoom *visibleRoom, const ApeViewport *viewport )
{
	if ( ape_config_.renderer.showSelectionBuffer )
	{
		return;
	}

	ApeRoom *room = visibleRoom->room;
	assert( room != nullptr );

	// handle fog overrides

	QmMathColour4f fogColourRestore = ape_rendererState_.fogColour;
	float          fogNearRestore   = ape_rendererState_.fogNear;
	float          fogFarRestore    = ape_rendererState_.fogFar;

	ape_rendererState_.fogColour = room->fogColour;
	ape_rendererState_.fogNear   = ape_config_.renderer.fogNearOverride > -1.0f ? ape_config_.renderer.fogNearOverride : room->fogNear;
	ape_rendererState_.fogFar    = ape_config_.renderer.fogFarOverride > -1.0f ? ape_config_.renderer.fogFarOverride : room->fogFar;

	qm_gfx_debug_push_group_marker( "room_draw" );

	// deal with the portals first
	if ( ape_config_.world.showAllRooms )
	{
		PLCollisionAABB roomBounds = ape_world_node_get_transformed_local_bounds( APE_WORLD_NODE( room ) );
		ape_draw_debug_aabb( &roomBounds, PL_COLOUR_GREEN );

		for ( unsigned int i = 0; i < visibleRoom->numPortals; ++i )
		{
			ApeCameraVisiblePortal *visiblePortal = &visibleRoom->portals[ i ];
			if ( visiblePortal->nextRoom == nullptr )
			{
				continue;
			}

			ApeCameraVisibleRoom *nextVisibleRoom = visiblePortal->nextRoom;
			if ( nextVisibleRoom->room == nullptr )
			{
				continue;
			}

			const ApeBrushFace *portal = visiblePortal->portalFace;
			draw_wireframe_portal_face( portal );

			if ( ape_brush_face_is_mirror( portal ) )
			{
				continue;
			}

			ape_rendererState_.depth++;
			ape_room_draw_( camera, nextVisibleRoom, viewport );
			ape_rendererState_.depth--;
		}
	}
	else
	{
		for ( unsigned int i = 0; i < visibleRoom->numPortals; ++i )
		{
			draw_portal( camera, viewport, visibleRoom, &visibleRoom->portals[ i ] );
		}
	}

	switch ( camera->drawMode )
	{
		default:
		case APE_CAMERA_DRAW_MODE_WIREFRAME:
		{
			draw_room_wireframe( camera );
			break;
		}
		case APE_CAMERA_DRAW_MODE_TEXTURED:
		case APE_CAMERA_DRAW_MODE_SHADED:
		{
			draw_solid_room( camera, visibleRoom, false );
			draw_translucent_room( camera, visibleRoom );

			ape_decal_manager_draw_( room->decalManager );
			break;
		}
	}

	ape_rendererState_.fogColour = fogColourRestore;
	ape_rendererState_.fogNear   = fogNearRestore;
	ape_rendererState_.fogFar    = fogFarRestore;

	draw_room_editor( visibleRoom );

	ape_draw_debug_mesh_display_();

	qm_gfx_debug_pop_group_marker();
}

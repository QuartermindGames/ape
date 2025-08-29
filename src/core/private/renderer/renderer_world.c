// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "qmos/public/qm_os_random.h"

#include "ape_private.h"
#include "renderer.h"
#include "material/material.h"
#include "camera/camera.h"
#include "editor/editor.h"
#include "world/world.h"

//TODO: eventually we should do away with this
#define MAX_MATERIALS_PER_PASS 256
#define MAX_SUB_MESHES         8192
static int subMeshes[ MAX_MATERIALS_PER_PASS ][ MAX_SUB_MESHES ];
static int firstSubMeshes[ MAX_MATERIALS_PER_PASS ][ MAX_SUB_MESHES ];
static int numSubMeshes[ MAX_MATERIALS_PER_PASS ];

static bool showHiddenFaces;

void ape_renderer_world_register_console_variables_()
{
	PlRegisterConsoleVariable( "renderer_world.showHiddenFaces", "Toggle hidden faces.", "false", PL_VAR_BOOL, &showHiddenFaces, nullptr, false );
}

static void draw_face_wireframe( const ApeBrushFace *face )
{
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		const ApeBrushFaceVertex *a = face->edgeLoop[ i ];
		PlgImmPushVertex( a->position->x, a->position->y, a->position->z );
		if ( ape_brush_face_is_portal( face ) )
		{
			PlgImmColour( 255, 0, 255, 255 );
		}
		else
		{
			PlgImmColour( 255, 255, 255, 255 );
		}

		const ApeBrushFaceVertex *b = ( ( i + 1 ) < face->numVertices ) ? face->edgeLoop[ i + 1 ] : face->edgeLoop[ 0 ];
		PlgImmPushVertex( b->position->x, b->position->y, b->position->z );
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
	if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		const ApeBrush *brush = ( ApeBrush * ) node;
		if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
		{
			bool  selected = false;
			void *p;
			COM_ITERATE_LINKED_LIST( p, instance->selectedObjects, i )
			{
				if ( ( ApeBrush * ) p == brush )
				{
					selected = true;
					break;
				}
			}

			if ( !selected )
			{
				return;
			}
		}

		for ( unsigned int i = 0; i < brush->numFaces; *offset += brush->faces[ i ].numVertices, ++i )
		{
			assert( numSubMeshes[ 0 ] < MAX_SUB_MESHES );
			if ( numSubMeshes[ 0 ] >= MAX_SUB_MESHES )
			{
				ape_warning_( "Hit submesh limit for draw, will squeeze into another batch!\n" );
				break;
			}

			const ApeBrushFace *face = &brush->faces[ i ];
			if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
			{
				void *p;
				COM_ITERATE_LINKED_LIST( p, instance->selectedObjects, i )
				{
					if ( ( ApeBrushFace * ) p == face )
					{
						if ( PlgIsBoxInsideView( instance->camera->internal, &face->bounds ) )
						{
							subMeshes[ 0 ][ numSubMeshes[ 0 ] ]      = face->numVertices;
							firstSubMeshes[ 0 ][ numSubMeshes[ 0 ] ] = *offset;
							numSubMeshes[ 0 ]++;

							ape_rendererPerformance_.numFacesDrawn++;
						}

						break;
					}
				}
			}
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		build_selection_display_list( child, instance, offset );
	}
}

static void build_brush_display_list( ApeWorldNode *node, ApeMaterial *material, ApeLight *light, ApeCamera *camera, const ApeCameraVisibleRoom *visibleRoom, unsigned int *offset, ApeRendererPassFlag stage )
{
	if ( node->flags != APE_WORLD_NODE_FLAG_HIDDEN && node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		const ApeBrush *brush = ( ApeBrush * ) node;
		for ( unsigned int i = 0; i < brush->numFaces; *offset += brush->faces[ i ].numVertices, ++i )
		{
			assert( numSubMeshes[ 0 ] < MAX_SUB_MESHES );
			if ( numSubMeshes[ 0 ] >= MAX_SUB_MESHES )
			{
				ape_warning_( "Hit submesh limit for draw, will squeeze into another batch!\n" );
				break;
			}

			ApeBrushFace *face = &brush->faces[ i ];
			// check the face isn't the same face we're looking in from
			if ( visibleRoom->entrance != nullptr && visibleRoom->entrance->portalFace == face )
			{
				continue;
			}

			if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN && !showHiddenFaces )
			{
				continue;
			}

			if ( ( !( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN ) && material != face->material ) || ( ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN && material != ape_material_get_default( APE_MATERIAL_DEFAULT_HIDDEN ) ) ) )
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

			if ( PlgIsBoxInsideView( camera->internal, &face->bounds ) )
			{
				subMeshes[ 0 ][ numSubMeshes[ 0 ] ]      = face->numVertices;
				firstSubMeshes[ 0 ][ numSubMeshes[ 0 ] ] = *offset;
				numSubMeshes[ 0 ]++;

				ape_rendererPerformance_.numFacesDrawn++;
			}
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		build_brush_display_list( child, material, light, camera, visibleRoom, offset, stage );
	}
}

static void draw_visible_camera_nodes( ApeCamera *camera, ApeLight *light, const ApeRendererPassFlag flags )
{
#if 0//TODO
	unsigned int   num;
	ApeWorldNode **visibleNodes = ape_camera_get_visible_nodes_( camera, &num );
	for ( unsigned int i = 0; i < num; ++i )
	{
		if ( visibleNodes[ i ]->type == APE_WORLD_NODE_TYPE_ENTITY )
		{
			ApeEntity *entity = ( ApeEntity * ) visibleNodes[ i ];
			ape_entity_draw( entity, light, flags );
		}
	}
#endif
}

static void draw_node_meshes( ApeWorldNode *worldNode, const ApeCameraVisibleRoom *visibleRoom, ApeCamera *camera, ApeLight *light, const ApeRendererPassFlag flags )
{
	ape_world_node_update_mesh_cache_( worldNode );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PLMatrix4 transform = ape_world_node_get_transform( worldNode );
	PlLoadMatrix( &transform );

	if ( worldNode->mesh != nullptr )
	{
		//TODO: this is operating off a universal list, should only operate on *world* materials!!!
		PLLinkedList *materialList = ape_memory_get_pool_list_( APE_CACHE_POOL_MATERIALS );
		assert( materialList != nullptr );

		ApeMemoryCacheHeader *header;
		COM_ITERATE_LINKED_LIST( header, materialList, i )
		{
			ApeMaterial *material = header->userData;
			assert( material != nullptr );

			// blended materials get drawn later
			if ( ( flags & APE_RENDERER_PASS_FLAG_TRANSLUCENT && !ape_material_is_blended( material ) ) || ( flags & APE_RENDERER_PASS_FLAG_OPAQUE && ape_material_is_blended( material ) ) )
			{
				continue;
			}

			COM_PROFILE_START( "build_brush_display_list" );

			unsigned int offset = 0;
			build_brush_display_list( worldNode, material, light, camera, visibleRoom, &offset, flags );

			COM_PROFILE_END( "build_brush_display_list" );

			if ( numSubMeshes[ 0 ] == 0 )
			{
				continue;
			}

			PLGMesh *mesh        = worldNode->mesh;
			mesh->numSubMeshes   = numSubMeshes[ 0 ];
			mesh->firstSubMeshes = firstSubMeshes[ 0 ];
			mesh->subMeshes      = subMeshes[ 0 ];

			ape_material_draw( material, mesh, light != nullptr ? ( ApeLightPointerArray ) { light } : nullptr );

			mesh->numSubMeshes = numSubMeshes[ 0 ] = 0;
		}
	}

	PlPopMatrix();

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, worldNode->children, i )
	{
		draw_node_meshes( child, visibleRoom, camera, light, flags );
	}
}

void ape_model_draw_models( const ApeRoom *room, const ApeCamera *camera, ApeLight *light );

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
		ape_rendererState_.ambience = room->ambientLight;
	}

	// draw other node types
	//TODO: all this needs sorting for transparency... temporary!!!
	draw_visible_camera_nodes( camera, light, flags );

	// recurse down the tree to draw all nodes with explicit meshes
	draw_node_meshes( APE_WORLD_NODE( room ), visibleRoom, camera, light, flags );

	//TODO: botch, we don't check render pass flag under draw models yet
	if ( !( flags & APE_RENDERER_PASS_FLAG_TRANSLUCENT ) )
	{
		ape_model_draw_models( room, camera, light );
	}

	if ( flags & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS )
	{
		ape_rendererState_.ambience = QM_MATH_COLOUR4F( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	COM_PROFILE_FUNCTION_END();
}

/////////////////////////////////////////////////////////////////////////////////////
// Stencil Shadows
/////////////////////////////////////////////////////////////////////////////////////

static constexpr float F_INFINITY = 10000.0f;

static QmMathVector3f get_projection( const ApeLight *light, const QmMathVector3f *vertex, const QmMathVector3f *faceOrigin )
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

	static constexpr float F_PI = 4.0f / 3.0f * PL_PI;

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

	static constexpr float F_PI = 4.0f / 3.0f * PL_PI;

	float          c = light->radius * light->radius * light->radius;
	float          r = powf( F_PI * c, 1.0f / 3.0f );
	QmMathVector3f s = qm_math_vector3f_normalize( qm_math_vector3f_sub( *vertex, pos ) );
	return qm_math_vector3f_add( *vertex, qm_math_vector3f_scale_float( s, r ) );

#endif
}

static void draw_brush_stencil_shadow_cap( const ApeBrushFace *face, const ApeLight *light, bool start, unsigned int *indices )
{
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		const ApeBrushFaceVertex *vertex = face->edgeLoop[ i ];

		// get the projected position (if start, just uses the initial position)
		const QmMathVector3f ppos = start ? *vertex->position : get_projection( light, vertex->position, &face->bounds.absOrigin );

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
			if ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN || !ape_material_shadows_enabled( face->material ) )
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

void ape_world_draw_stencil_shadows_( ApeCamera *camera, ApeLight *light )
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
		ape_material_draw( shadowMaterial, mesh, nullptr );
	}

	PlPopMatrix();

	COM_PROFILE_FUNCTION_END();
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void ape_room_draw_selected_( ApeRoom *room, ApeEditorInstance *instance )
{
	if ( PlIsLinkedListEmpty( instance->selectedObjects ) )
	{
		return;
	}

	unsigned int offset = 0;
	build_selection_display_list( &room->base, instance, &offset );

	if ( numSubMeshes[ 0 ] == 0 )
	{
		return;
	}

	PLGMesh *mesh        = APE_WORLD_NODE( room )->mesh;
	mesh->numSubMeshes   = numSubMeshes[ 0 ];
	mesh->firstSubMeshes = firstSubMeshes[ 0 ];
	mesh->subMeshes      = subMeshes[ 0 ];

	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR_SELECTION );
	assert( material != nullptr );
	ape_material_draw( material, mesh, nullptr );

	mesh->numSubMeshes = numSubMeshes[ 0 ] = 0;
}

static void draw_translucent_room( ApeCamera *camera, const ApeCameraVisibleRoom *visibleRoom, float depth )
{
	COM_PROFILE_FUNCTION_START();

	PlgPushDebugGroupMarker( "Translucent Room" );

	// and now depth pre-pass
	draw_room( camera, visibleRoom, nullptr, APE_RENDERER_PASS_FLAG_DEPTH_PREPASS | APE_RENDERER_PASS_FLAG_TRANSLUCENT );

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SHADED )
	{
		PlgDepthMask( false );

		for ( unsigned int i = 0; i < visibleRoom->numLights; ++i )
		{
			if ( visibleRoom->lights[ i ]->colour.a == 0.0f )
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

		PlgDepthMask( depth );
	}

	PlgPopDebugGroupMarker();

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

	const bool drawShadows = !depth && ape_config_.renderer.useStencilShadowVolumes && ( ape_light_get_shadow_type( light ) == APE_LIGHT_SHADOW_TYPE_DYNAMIC );
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

	PlgPushDebugGroupMarker( "Solid Room" );

	// and now depth pre-pass
	draw_room( camera, visibleRoom, nullptr, APE_RENDERER_PASS_FLAG_DEPTH_PREPASS | APE_RENDERER_PASS_FLAG_OPAQUE );

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SHADED )
	{
		PlgPushDebugGroupMarker( "Shaded" );

		PlgDepthMask( false );

		for ( unsigned int i = 0; i < visibleRoom->numLights; ++i )
		{
			ApeLight *light = visibleRoom->lights[ i ];
			if ( ape_config_.renderer.lightJitterSamples > 0 )
			{
				unsigned int seed = ape_config_.renderer.lightJitterSamples;

				QmMathVector3f storePos   = light->base.position;
				float          storePower = light->colour.a;
				for ( unsigned int j = 0; j < ape_config_.renderer.lightJitterSamples; ++j )
				{
#define JITTER_VARIATION ( qm_os_random_float( &seed, ( ( float ) i ) * ( ape_config_.renderer.lightJitterSamples * 2.0f ) / ape_config_.renderer.lightJitterSamples ) - \
	                       qm_os_random_float( &seed, ( ( float ) i ) * ( ape_config_.renderer.lightJitterSamples * 2.0f ) / ape_config_.renderer.lightJitterSamples ) )
					light->base.position.x += JITTER_VARIATION;
					light->base.position.y += JITTER_VARIATION;
					light->base.position.z += JITTER_VARIATION;
					light->colour.a = ( i * 1.0f / ape_config_.renderer.lightJitterSamples );

					draw_solid_room_lit( visibleRoom, camera, visibleRoom->lights[ i ], depth );
				}

				light->base.position = storePos;
				light->colour.a      = storePower;
				continue;
			}

			draw_solid_room_lit( visibleRoom, camera, light, depth );
		}

		PlgDepthMask( depth );

		PlgPopDebugGroupMarker();
	}

	PlgPopDebugGroupMarker();

	COM_PROFILE_FUNCTION_END();
}

static void draw_portal_face( const ApeBrushFace *portal, bool useMaterial )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( portal->parent ) );
	PlMultiMatrix( &transform );

	PLGMesh *mesh = PlgImmBegin( PLG_MESH_TRIANGLE_FAN );
	for ( unsigned int j = 0; j < portal->numVertices; ++j )
	{
		const ApeBrushFaceVertex *vertex = portal->edgeLoop[ j ];
		//TODO: handle transforms for the brush in software here
		PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
		PlgImmColour( 0, 0, 0, 0 );
	}

	if ( useMaterial )
	{
		ape_material_draw( portal->material, mesh, nullptr );
	}
	else
	{
		ape_set_active_shader_by_default_( APE_SHADER_DEFAULT );
		PlgImmDraw();
	}

	PlPopMatrix();
}

static void draw_wireframe_portal_face( const ApeBrushFace *portal )
{
	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	PLMatrix4 transform = ape_world_node_get_transform( APE_WORLD_NODE( portal->parent ) );
	PlMultiMatrix( &transform );

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgImmBegin( PLG_MESH_LINE_LOOP );

	for ( unsigned int j = 0; j < portal->numVertices; ++j )
	{
		const ApeBrushFaceVertex *vertex = portal->edgeLoop[ j ];
		PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
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

	PlgPushDebugGroupMarker( "portal" );

	// first draw the portal stencil we'll test again

	PlgClipViewport( visiblePortal->screenRect.x, visiblePortal->screenRect.y,
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

	PLMatrix4 clipMatrix = PlTranslateMatrix4( visiblePortal->origin );
	ape_draw_debug_sphere( visiblePortal->origin, PL_COLOUR_RED, 16.0f );
	QmMathVector4f clipPlane;
	if ( ape_rendererState_.mirror )
	{
		clipPlane   = qm_math_vector4f( visiblePortal->normal.x, visiblePortal->normal.y, visiblePortal->normal.z, 0.0f );
		clipPlane.w = -qm_math_vector3f_dot_product( visiblePortal->normal, visiblePortal->origin );
	}
	else
	{
		clipPlane   = qm_math_vector4f( visiblePortal->normal.x, visiblePortal->normal.y, visiblePortal->normal.z, 0.0f );
		clipPlane.w = -qm_math_vector3f_dot_product( visiblePortal->normal, visiblePortal->origin );
	}

	PlgSetClipPlane( &clipPlane, &clipMatrix, false );

	// now recurse into the next room

	ape_rendererState_.depth++;

	// set the view matrix we need
	camera->internal->internal.view = nextVisibleRoom->viewMatrix;
	PlgSetViewMatrix( &nextVisibleRoom->viewMatrix );
	PlgSetupCameraFrustum( camera->internal );

	ape_room_draw_( camera, nextVisibleRoom, viewport );

	// reset the view matrix back
	camera->internal->internal.view = visibleRoom->viewMatrix;
	PlgSetViewMatrix( &camera->internal->internal.view );
	PlgSetupCameraFrustum( camera->internal );

	// and pop out

	ape_rendererState_.depth--;
	ape_rendererState_.mirror = false;

	PlgSetClipPlane( nullptr, nullptr, false );

	// depth buffer pop

	PlgColourMask( false, false, false, false );
	PlgDepthMask( true );

	draw_portal_face( portal, true );

	PlgColourMask( true, true, true, true );

	PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );

	// reset the viewport
	ape_viewport_set_clip( viewport );

	PlgPopDebugGroupMarker();

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

	PlgPushDebugGroupMarker( "room_draw" );

	// deal with the portals first
	if ( ape_config_.world.showAllRooms )
	{
		ApeRoom        *room       = visibleRoom->room;
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
			draw_translucent_room( camera, visibleRoom, false );

			ape_decal_manager_draw_( visibleRoom->room->decalManager );
			break;
		}
	}

	draw_room_editor( visibleRoom );

	ape_draw_debug_mesh_display_();

	PlgPopDebugGroupMarker();
}

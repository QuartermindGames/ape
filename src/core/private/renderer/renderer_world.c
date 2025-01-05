// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "ape_private.h"
#include "renderer.h"

#include "world/world.h"
#include "model/model.h"

//TODO: eventually we should do away with this
#define MAX_MATERIALS_PER_PASS 256
#define MAX_SUB_MESHES         8192
static int subMeshes[ MAX_MATERIALS_PER_PASS ][ MAX_SUB_MESHES ];
static int firstSubMeshes[ MAX_MATERIALS_PER_PASS ][ MAX_SUB_MESHES ];
static int numSubMeshes[ MAX_MATERIALS_PER_PASS ];

static bool showHiddenFaces;
void        ape_renderer_world_register_console_variables_()
{
	PlRegisterConsoleVariable( "renderer_world.showHiddenFaces", "Toggle hidden faces.", "false", PL_VAR_BOOL, &showHiddenFaces, nullptr, false );
}

static void draw_face_wireframe( const ApeBrushFace *face )
{
	for ( uint i = 0; i < face->numVertices; ++i )
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
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgImmBegin( PLG_MESH_LINES );

	uint           numFaces;
	ApeBrushFace **faces = ( ApeBrushFace ** ) PlGetVectorArrayDataEx( camera->pvs.visibleFaces, &numFaces );
	for ( uint i = 0; i < numFaces; ++i )
	{
		draw_face_wireframe( faces[ i ] );
	}

	PlgImmDraw();
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

static void build_mesh_cache( PLGMesh *mesh, ApeWorldNode *node )
{
	if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		const ApeBrush *brush = ( ApeBrush * ) node;
		for ( uint i = 0; i < brush->numFaces; ++i )
		{
			const ApeBrushFace *face = &brush->faces[ i ];
			for ( uint j = 0; j < face->numVertices; ++j )
			{
				const ApeBrushFaceVertex *vertex = face->edgeLoop[ j ];

				//TODO: handle transforms for the brush in software here

				const uint idx = PlgAddMeshVertex( mesh, vertex->position, &vertex->normal, &PL_COLOURU8( 255, 255, 255, 255 ), &vertex->textureCoords );

				// these have to be set seperate for now, need an api for it
				mesh->vertices[ idx ].tangent   = vertex->tangent;
				mesh->vertices[ idx ].bitangent = vertex->bitangent;
			}
		}
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		build_mesh_cache( mesh, child );
	}
}

static uint get_total_verts_for_tree( ApeWorldNode *node )
{
	uint numVertices = 0;
	if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		const ApeBrush *brush = ( ApeBrush * ) node;
		numVertices           = brush->numVertices;
	}

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, node->children, i )
	{
		numVertices += get_total_verts_for_tree( child );
	}

	return numVertices;
}

static void update_mesh_cache_( ApeRoom *self )
{
	COM_PROFILE_FUNCTION_START();

	if ( !self->isDirty )
	{
		return;
	}

	if ( self->mesh == nullptr )
	{
		self->mesh = PlgCreateMesh( PLG_MESH_TRIANGLE_FAN, PLG_DRAW_STATIC, 0, get_total_verts_for_tree( &self->base ) );
		if ( self->mesh == nullptr )
		{
			ape_warning_( "Failed to create mesh for room: %s\n", PlGetError() );
			return;
		}
	}

	// not an expensive operation, so call regardless
	PlgClearMesh( self->mesh );

	// iterate over all the brushes under the room, build up the mesh
	build_mesh_cache( self->mesh, &self->base );

	// finally, upload it
	PlgUploadMesh( self->mesh );
	self->isDirty = false;

	COM_PROFILE_FUNCTION_END();
}

static void build_selection_display_list( ApeWorldNode *node, ApeEditorInstance *instance, uint *offset )
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

		for ( uint i = 0; i < brush->numFaces; *offset += brush->faces[ i ].numVertices, ++i )
		{
			assert( numSubMeshes[ 0 ] < MAX_SUB_MESHES );
			if ( numSubMeshes[ 0 ] >= MAX_SUB_MESHES )
			{
				PRINT_WARNING( "Hit submesh limit for draw, will squeeze into another batch!\n" );
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

static void build_brush_display_list( ApeWorldNode *node, ApeMaterial *material, ApeLight *light, ApeCamera *camera, uint *offset, ApeRendererPassFlag stage )
{
	if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
	{
		const ApeBrush *brush = ( ApeBrush * ) node;
		for ( uint i = 0; i < brush->numFaces; *offset += brush->faces[ i ].numVertices, ++i )
		{
			assert( numSubMeshes[ 0 ] < MAX_SUB_MESHES );
			if ( numSubMeshes[ 0 ] >= MAX_SUB_MESHES )
			{
				PRINT_WARNING( "Hit submesh limit for draw, will squeeze into another batch!\n" );
				break;
			}

			ApeBrushFace *face = &brush->faces[ i ];
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

			if ( light != nullptr && ( light->type == APE_LIGHT_TYPE_OMNI ) && !PlIsSphereIntersectingAabb( &PlSetupCollisionSphere( light->base.position, light->radius ), &face->bounds ) )
			{
				continue;
			}

#if 0// ditched for speed...
			PLCollisionPlane plane = { .normal = face->normal, .origin = face->bounds.absOrigin };
			if ( light != nullptr && ape_light_test_plane_shadow( light, material, &plane ) )
			{
				continue;
			}
#endif

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
		build_brush_display_list( child, material, light, camera, offset, stage );
	}
}

static void draw_visible_camera_nodes( ApeCamera *camera, ApeLight *light )
{
	unsigned int   num;
	ApeWorldNode **visibleNodes = ape_camera_get_visible_nodes_( camera, &num );
	for ( unsigned int i = 0; i < num; ++i )
	{
		if ( visibleNodes[ i ]->type != APE_WORLD_NODE_TYPE_MODEL )
		{
			continue;
		}

		const ApeModelNode *modelNode = ( ApeModelNode * ) visibleNodes[ i ];
		ape_model_draw( modelNode->model, &( ApeModelAnimationState ) {}, PlGetMatrix( PL_MODELVIEW_MATRIX ), light );
	}
}

static void draw_room( ApeRoom *room, ApeCamera *camera, ApeLight *light, const ApeRendererPassFlag stage )
{
	if ( !( stage & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS ) && light == NULL )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	if ( stage & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS )
	{
		ape_rendererState_.ambience = room->ambientLight;
	}

	// draw other node types
	//TODO: all this needs sorting for transparency... temporary!!!
	draw_visible_camera_nodes( camera, light );

	update_mesh_cache_( room );

	//TODO: this is operating off a universal list, should only operate on *world* materials!!!
	PLLinkedList *materialList = ape_memory_get_pool_list_( APE_CACHE_POOL_MATERIALS );
	assert( materialList != nullptr );

	ApeMemoryCacheHeader *header;
	COM_ITERATE_LINKED_LIST( header, materialList, i )
	{
		ApeMaterial *material = header->userData;
		assert( material != nullptr );

		// blended materials get drawn later
		if ( stage & APE_RENDERER_PASS_FLAG_TRANSLUCENT && !ape_material_is_blended( material ) || ( stage & APE_RENDERER_PASS_FLAG_OPAQUE && ape_material_is_blended( material ) ) )
		{
			continue;
		}

		COM_PROFILE_START( "build_brush_display_list" );

		uint offset = 0;
		build_brush_display_list( &room->base, material, light, camera, &offset, stage );

		COM_PROFILE_END( "build_brush_display_list" );

		if ( numSubMeshes[ 0 ] == 0 )
		{
			continue;
		}

		PLGMesh *mesh        = room->mesh;
		mesh->numSubMeshes   = numSubMeshes[ 0 ];
		mesh->firstSubMeshes = firstSubMeshes[ 0 ];
		mesh->subMeshes      = subMeshes[ 0 ];

		ape_material_draw( material, mesh, light != nullptr ? ( ApeLightPointerArray ) { light } : nullptr );

		mesh->numSubMeshes = numSubMeshes[ 0 ] = 0;
	}

	if ( stage & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS )
	{
		ape_rendererState_.ambience = PL_COLOURF32( 0.0f, 0.0f, 0.0f, 0.0f );
	}

	COM_PROFILE_FUNCTION_END();
}

/////////////////////////////////////////////////////////////////////////////////////
// Stencil Shadows
/////////////////////////////////////////////////////////////////////////////////////

static constexpr float F_INFINITY = 10000.0f;

static PLVector3 get_projection( const ApeLight *light, const PLVector3 *origin )
{
	if ( light->type != APE_LIGHT_TYPE_SUN )
	{
		PLVector3   sub    = PlNormalizeVector3( PlSubtractVector3( *origin, light->base.position ) );
		float       dif    = PlVector3Length( PlSubtractVector3( *origin, light->base.position ) );
		const float radius = light->radius;
		if ( dif > radius )
		{
			dif = radius;
		}

		sub = PlScaleVector3F( sub, radius - dif );
		return sub;
	}

	return PlScaleVector3F( PlNormalizeVector3( light->base.position ), F_INFINITY );
}

static void draw_brush_stencil_shadow_cap( const ApeBrushFace *face, const ApeLight *light, bool start, uint *indices )
{
	for ( uint i = 0; i < face->numVertices; ++i )
	{
		const ApeBrushFaceVertex *vertex        = face->edgeLoop[ i ];
		const PLVector3           projDirection = start ? pl_vecOrigin3 : get_projection( light, vertex->position );
		indices[ i ]                            = PlgImmPushVertex( vertex->position->x + projDirection.x,
		                                                            vertex->position->y + projDirection.y,
		                                                            vertex->position->z + projDirection.z );
#if 1// for debugging
		PlgImmColour( start ? 255 : 0, start ? 0 : 255, 255, 255 );
#endif
	}

	for ( uint i = 1; i + 1 < face->numVertices; ++i )
	{
		PlgImmPushTriangle( indices[ 0 ], indices[ start ? i : ( i + 1 ) ], indices[ start ? ( i + 1 ) : i ] );
	}
}

static void draw_node_shadow_volume( ApeWorldNode *node, const ApeLight *light, PLGMesh *mesh, uint *numIndices )
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
		for ( uint i = 0; i < brush->numFaces; ++i )
		{
			const ApeBrushFace *face = &brush->faces[ i ];
			if ( ( face->flags & APE_BRUSH_FACE_FLAG_HIDDEN ) || !ape_material_shadows_enabled( face->material ) )
			{
				continue;
			}

			PLCollisionPlane plane = ( PLCollisionPlane ) { .normal = brush->faces[ i ].normal, .origin = brush->faces[ i ].bounds.absOrigin };
			if ( ape_light_test_plane( light, &plane ) )
			{
				continue;
			}

			//todo: this check should probably be integrated into light_test_plane...
			if ( light->type == APE_LIGHT_TYPE_OMNI && !PlIsSphereIntersectingAabb( &PlSetupCollisionSphere( light->base.position, light->radius ), &brush->faces[ i ].bounds ) )
			{
				continue;
			}

			// There's probably a more efficient way of doing this,
			// but let's go ahead and store all the indices into a dynamic array
			*numIndices += ( face->numVertices * 2 );// * 2 for edges
			static uint *indices    = nullptr;
			static uint  maxIndices = 0;
			if ( indices == NULL )
			{
				maxIndices = ( *numIndices * brush->numFaces );
				indices    = PL_NEW_( uint, maxIndices );
			}
			else if ( *numIndices > maxIndices )
			{
				maxIndices = *numIndices + 16;
				indices    = PL_REALLOCA( indices, sizeof( uint ) * maxIndices );
			}

			uint *fl = &indices[ *numIndices - ( face->numVertices * 2 ) ];
			draw_brush_stencil_shadow_cap( face, light, false, fl );
			uint *sl = &indices[ *numIndices - face->numVertices ];
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

	uint     numIndices = 0;
	PLGMesh *mesh       = PlgImmBegin( PLG_MESH_TRIANGLES );

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

	uint offset = 0;
	build_selection_display_list( &room->base, instance, &offset );

	if ( numSubMeshes[ 0 ] == 0 )
	{
		return;
	}

	PLGMesh *mesh        = room->mesh;
	mesh->numSubMeshes   = numSubMeshes[ 0 ];
	mesh->firstSubMeshes = firstSubMeshes[ 0 ];
	mesh->subMeshes      = subMeshes[ 0 ];

	ApeMaterial *material = ape_material_get_default( APE_MATERIAL_DEFAULT_EDITOR_SELECTION );
	assert( material != nullptr );
	ape_material_draw( material, mesh, nullptr );

	mesh->numSubMeshes = numSubMeshes[ 0 ] = 0;
}

void ape_world_draw_( ApeCamera *camera, ApeLight *light, const ApeRendererPassFlag stage )
{
	if ( ( stage & APE_RENDERER_PASS_FLAG_DEPTH_PREPASS ) && ape_config_.renderer.skipAmbience )
	{
		return;
	}

	ApeRoom *room = ape_camera_get_room( camera );
	if ( room == nullptr )
	{
		return;
	}

	draw_room( room, camera, light, stage );
}

static void draw_translucent_room( ApeRoom *room, ApeCamera *camera, float depth )
{
	// and now depth pre-pass
	draw_room( room, camera, nullptr, APE_RENDERER_PASS_FLAG_DEPTH_PREPASS | APE_RENDERER_PASS_FLAG_TRANSLUCENT );

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SHADED )
	{
		PlgDepthMask( false );

		unsigned int numLights;
		ApeLight   **lights = ape_camera_get_visible_lights_( camera, &numLights );
		for ( unsigned int i = 0; i < numLights; ++i )
		{
			if ( lights[ i ]->colour.a == 0.0f )
			{
				continue;
			}

			//TODO: viewport clipping per light volume

			ape_rendererState_.overrideBlendMode = true;
			ape_rendererState_.blendModeA        = PLG_BLEND_ONE;
			ape_rendererState_.blendModeB        = PLG_BLEND_ONE;

			draw_room( room, camera, lights[ i ], APE_RENDERER_PASS_FLAG_TRANSLUCENT );

			ape_rendererState_.overrideBlendMode = false;
		}

		PlgDepthMask( depth );
	}
}

static void draw_solid_room( ApeRoom *room, ApeCamera *camera, bool depth )
{
	// and now depth pre-pass
	draw_room( room, camera, nullptr, APE_RENDERER_PASS_FLAG_DEPTH_PREPASS | APE_RENDERER_PASS_FLAG_OPAQUE );

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_SHADED )
	{
		PlgInsertDebugMarker( "solid room shaded" );

		PlgDepthMask( false );

		unsigned int numLights;
		ApeLight   **lights = ape_camera_get_visible_lights_( camera, &numLights );
		for ( unsigned int i = 0; i < numLights; ++i )
		{
			if ( lights[ i ]->colour.a <= 0.0f )
			{
				continue;
			}

			//TODO: viewport clipping per light volume, there was some code below for it but I've scrapped it for now

			const bool drawShadows = false;//ape_config_.renderer.useStencilShadowVolumes && ( ape_light_get_shadow_type( lights[ i ] ) == APE_LIGHT_SHADOW_TYPE_DYNAMIC );
			if ( drawShadows )
			{
				PlgClearBuffers( PLG_BUFFER_STENCIL );

				ape_rendererState_.cullMode = APE_RENDERER_CULL_MODE_NONE;

				if ( ape_config_.renderer.showShadowWireframe )
				{
					PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
					ape_world_draw_stencil_shadows_( camera, lights[ i ] );
					PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
				}

				PlgEnableGraphicsState( PLG_GFX_STATE_STENCILTEST );
				PlgEnableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
				PlgColourMask( false, false, false, false );

				PlgStencilBufferFunction( PLG_COMPARE_ALWAYS, 0x0, 0xFF );
				PlgStencilOp( PLG_STENCIL_FACE_FRONT, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_INCRWRAP, PLG_STENCIL_OP_KEEP );
				PlgStencilOp( PLG_STENCIL_FACE_BACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_DECRWRAP, PLG_STENCIL_OP_KEEP );

				ape_world_draw_stencil_shadows_( camera, lights[ i ] );

				PlgDisableGraphicsState( PLG_GFX_STATE_DEPTH_CLAMP );
				PlgColourMask( true, true, true, true );

				PlgStencilBufferFunction( PLG_COMPARE_EQUAL, 0x0, 0xFF );
				PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP );

				ape_rendererState_.cullMode = APE_RENDERER_CULL_MODE_DEFAULT;
			}

			ape_rendererState_.overrideBlendMode = true;
			ape_rendererState_.blendModeA        = PLG_BLEND_ONE;
			ape_rendererState_.blendModeB        = PLG_BLEND_ONE;

			draw_room( room, camera, lights[ i ], APE_RENDERER_PASS_FLAG_OPAQUE );

			ape_rendererState_.overrideBlendMode = false;

			if ( drawShadows )
			{
				PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );
			}
		}

		PlgDepthMask( depth );

		PlgInsertDebugMarker( "solid room shaded end" );
	}
}

void setup_reflection_matrix( const PLVector3 *normal, const PLVector3 *planePoint, PLMatrix4 *reflectionMatrix )
{
	const float d = -PlVector3DotProduct( *normal, *planePoint );

	// Fill the reflection matrix
	reflectionMatrix->mm[ 0 ][ 0 ] = 1.0f - 2.0f * normal->x * normal->x;
	reflectionMatrix->mm[ 0 ][ 1 ] = -2.0f * normal->x * normal->y;
	reflectionMatrix->mm[ 0 ][ 2 ] = -2.0f * normal->x * normal->z;
	reflectionMatrix->mm[ 0 ][ 3 ] = 0.0f;

	reflectionMatrix->mm[ 1 ][ 0 ] = -2.0f * normal->y * normal->x;
	reflectionMatrix->mm[ 1 ][ 1 ] = 1.0f - 2.0f * normal->y * normal->y;
	reflectionMatrix->mm[ 1 ][ 2 ] = -2.0f * normal->y * normal->z;
	reflectionMatrix->mm[ 1 ][ 3 ] = 0.0f;

	reflectionMatrix->mm[ 2 ][ 0 ] = -2.0f * normal->z * normal->x;
	reflectionMatrix->mm[ 2 ][ 1 ] = -2.0f * normal->z * normal->y;
	reflectionMatrix->mm[ 2 ][ 2 ] = 1.0f - 2.0f * normal->z * normal->z;
	reflectionMatrix->mm[ 2 ][ 3 ] = 0.0f;

	reflectionMatrix->mm[ 3 ][ 0 ] = -2.0f * normal->x * d;
	reflectionMatrix->mm[ 3 ][ 1 ] = -2.0f * normal->y * d;
	reflectionMatrix->mm[ 3 ][ 2 ] = -2.0f * normal->z * d;
	reflectionMatrix->mm[ 3 ][ 3 ] = 1.0f;
}

static void draw_portal_face( const ApeBrushFace *portal )
{
	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlgImmBegin( PLG_MESH_TRIANGLE_FAN );

	for ( uint j = 0; j < portal->numVertices; ++j )
	{
		const ApeBrushFaceVertex *vertex = portal->edgeLoop[ j ];
		//TODO: handle transforms for the brush in software here
		PlgImmPushVertex( vertex->position->x, vertex->position->y, vertex->position->z );
		PlgImmColour( 0, 0, 0, 0 );
	}

	PlgImmDraw();
}

//TODO: move into room code
void ape_room_draw_( ApeRoom *room, ApeCamera *camera, const ApeViewport *viewport )
{
	COM_PROFILE_FUNCTION_START();

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();

	//TODO: first thing to do is to deal with the portals

#if 0
	switch ( camera->drawMode )
	{
		default:
			break;
		case APE_CAMERA_DRAW_MODE_WIREFRAME:
			ape_world_draw_wireframe_( world, camera );
		break;
		case APE_CAMERA_DRAW_MODE_SOLID:
		case APE_CAMERA_DRAW_MODE_TEXTURED:
			ape_world_draw_( camera, nullptr, APE_RENDERER_PASS_FLAG_DEPTH_PREPASS | APE_RENDERER_PASS_FLAG_OPAQUE );
		ape_world_draw_( camera, nullptr, APE_RENDERER_PASS_FLAG_DEPTH_PREPASS | APE_RENDERER_PASS_FLAG_TRANSLUCENT );
		break;
		case APE_CAMERA_DRAW_MODE_SHADED:
			render_solid_world( camera, viewport );
		render_transparent_world( camera );
		break;
	}
#endif

	// first draw the portals
	PlgInsertDebugMarker( "Portal pass" );
	if ( ape_rendererState_.depth == 0 )
	{
		uint                       numPortals;
		static const ApeBrushFace *current[ 4 ] = {};
		ApeBrushFace             **portals      = ( ApeBrushFace ** ) PlGetVectorArrayDataEx( camera->pvs.visiblePortals, &numPortals );
		for ( uint i = 0; i < numPortals; ++i )
		{
			const ApeBrushFace *portal          = portals[ i ];
			ApeRoom            *destinationRoom = ape_brush_face_get_room( portal );

			if ( portal == current[ ape_rendererState_.depth ] )
			{
				continue;
			}

			PlgClearBuffers( PLG_BUFFER_STENCIL );

			PlgEnableGraphicsState( PLG_GFX_STATE_STENCILTEST );
			PlgStencilBufferFunction( PLG_COMPARE_ALWAYS, 4, 0xFF );
			PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_REPLACE );

			PlgColourMask( false, false, false, false );

			draw_portal_face( portal );

			PlgStencilBufferFunction( PLG_COMPARE_EQUAL, 4, 0xFF );
			PlgStencilOp( PLG_STENCIL_FACE_FRONTANDBACK, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP, PLG_STENCIL_OP_KEEP );

			PlgClearBuffers( PLG_BUFFER_DEPTH );

			PlgColourMask( true, true, true, true );

			PlMatrixMode( PL_VIEW_MATRIX );
			PlPushMatrix();

			//TODO: sigh... We need to deal with this twice, because camera does it's own shit!!
			PLMatrix4 store = camera->internal->internal.view;
			PlLoadMatrix( &camera->internal->internal.view );

			if ( ape_brush_face_is_mirror( portal ) )
			{
				ape_rendererState_.mirror = true;

				PLMatrix4 reflection;
				setup_reflection_matrix( &portal->normal, &portal->bounds.absOrigin, &reflection );
				PlMultiMatrix( &reflection );
			}

			camera->internal->internal.view = *PlGetMatrix( PL_VIEW_MATRIX );
			PlgSetViewMatrix( &camera->internal->internal.view );
			PlgSetupCameraFrustum( camera->internal );

			PlPopMatrix();

			ape_rendererState_.depth++;
			current[ ape_rendererState_.depth ] = portal;

			PLVector3 planePoint = portal->bounds.absOrigin;
			float     d          = -PlVector3DotProduct( portal->normal, planePoint );
			PlgSetClipPlane( &PL_VECTOR4( portal->normal.x, portal->normal.y, portal->normal.z, d ) );

			PlgInsertDebugMarker( "Solid portal room pass" );
			draw_solid_room( destinationRoom, camera, ape_rendererState_.depth > 0 );
			//PlgInsertDebugMarker( "Translucent portal room pass" );
			//draw_translucent_room( room, camera );

			PlgSetClipPlane( nullptr );

			ape_rendererState_.depth--;
			ape_rendererState_.mirror = false;

			//TODO: sigh... We need to deal with this twice, because camera does it's own shit!!
			camera->internal->internal.view = store;
			PlgSetViewMatrix( &camera->internal->internal.view );
			PlgSetupCameraFrustum( camera->internal );

			// depth trick...
			{
				PlgColourMask( false, false, false, false );
				PlgDepthMask( true );

				draw_portal_face( portal );

				PlgColourMask( true, true, true, true );
			}

			PlgDisableGraphicsState( PLG_GFX_STATE_STENCILTEST );
		}

		PlgInsertDebugMarker( "Portal pass done" );
	}

	if ( camera->drawMode == APE_CAMERA_DRAW_MODE_WIREFRAME )
	{
		draw_room_wireframe( camera );
	}
	else
	{
		PlgInsertDebugMarker( "Solid room pass" );
		draw_solid_room( room, camera, false );
		PlgInsertDebugMarker( "Translucent room pass" );
		draw_translucent_room( room, camera, false );
	}

	PlPopMatrix();

	COM_PROFILE_FUNCTION_END();
}

// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Primary code for dealing with editor functionality.

#include "plcore/pl_hashtable.h"

#include "aux/public/aux_project.h"

#include "ape_private.h"

#include "editor.h"

#include "core_console.h"
#include "renderer/renderer.h"
#include "renderer/material/material.h"

#include "world/world.h"
#include "gui/gui_private.h"

static AcmBranch *editorConfigRoot;

static QmMathVector2f editorDefaultMaterialScale = QM_MATH_VECTOR2F( 0.25f, 0.25f );

static bool editorShowLightmapOverlay;

static bool         showIcons;
static ApeMaterial *nodeIcons[ APE_WORLD_MAX_NODE_TYPES ];
static float        iconFade = 0.25f;

bool ape_editorStatus_ = false;

static void cache_node_icons( void )
{
	nodeIcons[ APE_WORLD_NODE_TYPE_EMPTY ]  = ape_material_cache( "materials/editor/icons/icon_node.mat.n", APE_CACHE_GROUP_EDITOR, true );
	nodeIcons[ APE_WORLD_NODE_TYPE_MODEL ]  = nodeIcons[ APE_WORLD_NODE_TYPE_EMPTY ];
	nodeIcons[ APE_WORLD_NODE_TYPE_ROOT ]   = nodeIcons[ APE_WORLD_NODE_TYPE_EMPTY ];
	nodeIcons[ APE_WORLD_NODE_TYPE_ROOM ]   = ape_material_cache( "materials/editor/icons/icon_room.mat.n", APE_CACHE_GROUP_EDITOR, true );
	nodeIcons[ APE_WORLD_NODE_TYPE_BRUSH ]  = ape_material_cache( "materials/editor/icons/icon_brush.mat.n", APE_CACHE_GROUP_EDITOR, true );
	nodeIcons[ APE_WORLD_NODE_TYPE_LIGHT ]  = ape_material_cache( "materials/editor/icons/icon_light.mat.n", APE_CACHE_GROUP_EDITOR, true );
	nodeIcons[ APE_WORLD_NODE_TYPE_CAMERA ] = ape_material_cache( "materials/editor/icons/icon_camera.mat.n", APE_CACHE_GROUP_EDITOR, true );
	nodeIcons[ APE_WORLD_NODE_TYPE_ENTITY ] = ape_material_cache( "materials/editor/icons/icon_entity.mat.n", APE_CACHE_GROUP_EDITOR, true );

	// root is weird; as in theory, you shouldn't see it...
	nodeIcons[ APE_WORLD_NODE_TYPE_ROOT ] = nodeIcons[ APE_WORLD_NODE_TYPE_EMPTY ];
}

static void release_node_icons( void )
{
	for ( unsigned int i = 0; i < APE_WORLD_MAX_NODE_TYPES; ++i )
	{
		if ( nodeIcons[ i ] == NULL )
		{
			continue;
		}

		ape_material_release( nodeIcons[ i ] );
		nodeIcons[ i ] = nullptr;
	}
}

bool ape_editor_is_active()
{
	return ape_editorStatus_;
}

AcmBranch *ape_editor_get_config()
{
	return editorConfigRoot;
}

/////////////////////////////////////////////////////////////////////////////////////
// Editor Instance Management
/////////////////////////////////////////////////////////////////////////////////////

void ape_grid_setup_( ApeEditorGrid *self );

static ApeEditorInstance *editorInstance;

ApeEditorInstance *ape_editor_instance_create_( ApeEditorMode mode )
{
	ApeEditorInstance *instance = QM_OS_MEMORY_NEW( ApeEditorInstance );

	if ( ape_editor_instance_setup( instance, mode ) == nullptr )
	{
		qm_os_memory_free( instance );
		instance = nullptr;
	}

	return instance;
}

ApeEditorInstance *ape_editor_instance_setup( ApeEditorInstance *self, ApeEditorMode mode )
{
	PL_ZERO( self, sizeof( ApeEditorInstance ) );

	self->mode         = mode;
	self->geometryMode = APE_EDITOR_GEOMETRY_MODE_PLOT;

	ape_grid_setup_( &self->grid );

	cache_node_icons();

	self->selectionTable = PlCreateHashTable();
	if ( self->selectionTable == nullptr )
	{
		ape_console_error_( true, "Failed to create selection table for instance: %s\n", PlGetError() );
	}

	self->subSelectionTable = PlCreateHashTable();
	if ( self->subSelectionTable == nullptr )
	{
		ape_console_error_( true, "Failed to create sub-selection table for instance: %s\n", PlGetError() );
	}

	self->selectedObjects = qm_os_linked_list_create();
	if ( self->selectedObjects == nullptr )
	{
		ape_console_error_( true, "Failed to create selected objects list for instance: %s\n", PlGetError() );
	}

	return self;
}

void ape_editor_instance_cleanup( ApeEditorInstance *self )
{
	self->numPolygonPoints = 0;

	PlDestroyHashTable( self->selectionTable );
	self->selectionTable = nullptr;
	PlDestroyHashTable( self->subSelectionTable );
	self->subSelectionTable = nullptr;

	qm_os_memory_free( self->selectedObjects );
	self->selectedObjects = nullptr;
	self->hoverSelection  = nullptr;

	release_node_icons();

	if ( editorInstance == self )
	{
		ape_editor_set_active_instance( nullptr );
	}
}

void ape_editor_set_active_instance( ApeEditorInstance *instance )
{
	editorInstance = instance;
}

ApeEditorInstance *ape_editor_get_active_instance( void )
{
	return editorInstance;
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_editor_toggle_faces( ApeEditorInstance *self )
{
	ApeBrushFace *face;
	QM_OS_LINKED_LIST_ITERATE( face, self->selectedObjects, i )
	{
		face->flags ^= APE_BRUSH_FACE_FLAG_HIDDEN;
	}
}

void ape_editor_toggle_other_faces( ApeEditorInstance *self )
{
	ApeBrushFace *referenceFace = nullptr;
	bool          setVisible;

	ApeBrushFace *selectedFace;
	QM_OS_LINKED_LIST_ITERATE( selectedFace, self->selectedObjects, i )
	{
		ApeBrush *brush = selectedFace->parent;
		assert( brush != nullptr );
		for ( unsigned int j = 0; j < brush->numFaces; ++j )
		{
			ApeBrushFace *currentFace = &brush->faces[ j ];
			bool          isSelected  = false;

			ApeBrushFace *otherSelectedFace;
			QM_OS_LINKED_LIST_ITERATE( otherSelectedFace, self->selectedObjects, k )
			{
				if ( currentFace == otherSelectedFace )
				{
					isSelected = true;
					break;
				}
			}

			if ( isSelected )
			{
				continue;
			}

			if ( referenceFace == nullptr )
			{
				referenceFace = currentFace;
				setVisible    = !( referenceFace->flags & APE_BRUSH_FACE_FLAG_HIDDEN );
			}

			if ( !setVisible )
			{
				currentFace->flags &= ~APE_BRUSH_FACE_FLAG_HIDDEN;
			}
			else
			{
				currentFace->flags |= APE_BRUSH_FACE_FLAG_HIDDEN;
			}
		}
	}
}

void ape_editor_flip_faces( ApeEditorInstance *self )
{
	if ( self->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return;
	}

	ApeBrushFace *face;
	QM_OS_LINKED_LIST_ITERATE( face, self->selectedObjects, i )
	{
		ape_brush_flip_face_( face );
	}
}

void ape_editor_shade_faces_smooth( ApeEditorInstance *self )
{
	if ( self->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return;
	}

	ape_brush_smooth_faces( self->selectedObjects );
}

void ape_editor_shade_faces_flat( ApeEditorInstance *self )
{
	if ( self->geometryMode != APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		return;
	}

	ApeBrushFace *face;
	QM_OS_LINKED_LIST_ITERATE( face, self->selectedObjects, i )
	{
		ape_brush_face_compute_normal( face );
	}

	//TODO: get rid of this!
	face = ape_editor_get_first_selected( self );

	ApeWorldNode *parent = ape_world_node_get_parent( APE_WORLD_NODE( face->parent ) );
	if ( parent != nullptr )
	{
		ape_world_node_mark_dirty_( parent );
	}
}

void ape_editor_duplicate_selection( ApeEditorInstance *self )
{
	PLLinkedList *newSelectionList = PlCreateLinkedList();
	if ( newSelectionList == nullptr )
	{
		ape_console_warning_( "Failed to create new selection list: %s\n", PlGetError() );
		return;
	}

	ApeWorldNode *worldNode;
	QM_OS_LINKED_LIST_ITERATE( worldNode, self->selectedObjects, i )
	{
		if ( worldNode->classType->clone == nullptr )
		{
			ape_console_warning_( "Cannot duplicate this type of node (%u)!\n", worldNode->type );
			continue;
		}

		ApeWorldNode *newNode = worldNode->classType->clone( worldNode );
		if ( newNode == nullptr )
		{
			// callback hopefully spat out an error
			continue;
		}

		PlInsertLinkedListNode( newSelectionList, worldNode );
	}

	ape_console_print_( "Duplicated %u nodes\n", PlGetNumLinkedListNodes( newSelectionList ) );

	ape_editor_selection_rebuild_( self );

	// now add the new items to the selection, so we can immediately start adjusting them around if we want
	COM_ITERATE_LINKED_LIST( worldNode, newSelectionList, i )
	{
		ape_editor_add_object_to_selection( self, worldNode );
	}

	PlDestroyLinkedList( newSelectionList );
}

void ape_editor_shift_selection( ApeEditorInstance *self, const QmMathVector3f *dir )
{
	ApeCamera *camera = self->camera;
	assert( camera != nullptr );

	ApeRoom *room = ape_camera_get_room( camera );
	assert( room != nullptr );

	QmMathVector3f left, up, forward;
	PlExtractMatrix4Directions( &self->grid.transform, &left, &up, &forward );

	// transform the dir relative to the grid
	QmMathVector3f gridDir = qm_math_vector3f_scale_float( left, dir->x );
	gridDir                = qm_math_vector3f_add( gridDir, qm_math_vector3f_scale_float( up, dir->y ) );
	gridDir                = qm_math_vector3f_add( gridDir, qm_math_vector3f_scale_float( forward, dir->z ) );

	switch ( self->geometryMode )
	{
		default:
			break;
		case APE_EDITOR_GEOMETRY_MODE_VERTEX:
		{
			QmMathVector3f *vertex;
			QM_OS_LINKED_LIST_ITERATE( vertex, self->selectedObjects, i )
			{
				*vertex = qm_math_vector3f_add( *vertex, qm_math_vector3f_scale_float( gridDir, self->grid.size ) );

				// try to determine what faces we need to update... sigh
				intptr_t  ptr   = ( intptr_t ) vertex;
				ApeBrush *brush = PlLookupHashTableUserData( self->subSelectionTable, &ptr, sizeof( intptr_t ) );
				if ( brush != nullptr )
				{
					for ( unsigned int j = 0; j < brush->numFaces; ++j )
					{
						for ( unsigned int k = 0; k < brush->faces[ j ].numVertices; ++k )
						{
							if ( &brush->vertices[ brush->faces[ j ].vertices[ k ].posIndex ] == vertex )
							{
								ape_brush_face_compute_normal( &brush->faces[ j ] );
								break;
							}
						}
					}

					ape_brush_compute_face_bounds( brush );
					ape_brush_compute_bounds( brush );

					ape_brush_mark_parent_dirty( brush );
				}
				else
				{
					ape_console_warning_( "Failed to lookup parent brush when adjusting vertex!\n" );
				}
			}
			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_FACE:
		{
			PLHashTable *vertexTable = PlCreateHashTable();

			// determine what vertices we're shifting
			ApeBrushFace *face;
			QM_OS_LINKED_LIST_ITERATE( face, self->selectedObjects, i )
			{
				ApeBrush *brush = face->parent;
				assert( brush != nullptr );

				for ( unsigned int j = 0; j < face->numVertices; ++j )
				{
					intptr_t ptr = ( intptr_t ) &brush->vertices[ face->vertices[ j ].posIndex ];
					PlInsertHashTableNode( vertexTable, &ptr, sizeof( intptr_t ), &brush->vertices[ face->vertices[ j ].posIndex ] );
				}

				ape_brush_mark_parent_dirty( brush );
			}

			QmMathVector3f *vertex;
			COM_ITERATE_HASHED_LIST( vertex, vertexTable, i )
			{
				*vertex = qm_math_vector3f_add( *vertex, qm_math_vector3f_scale_float( gridDir, self->grid.size ) );
			}

			// recompute normals, bounds, etc.
			//TODO: this will operate on the same brush multiple times if multiple faces are selected :(
			QM_OS_LINKED_LIST_ITERATE( face, self->selectedObjects, i )
			{
				ApeBrush *brush = face->parent;
				if ( brush == nullptr )
				{
					continue;
				}

				ape_brush_compute_face_bounds( brush );
				ape_brush_compute_face_normals( brush );
				ape_brush_compute_bounds( brush );
			}

			PlDestroyHashTable( vertexTable );
			break;
		}
		case APE_EDITOR_GEOMETRY_MODE_TRANSFORM:
		{
			ApeWorldNode *node;
			QM_OS_LINKED_LIST_ITERATE( node, self->selectedObjects, i )
			{
				if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
				{
					// these are special, because we're transforming all the vertices, rather than the explicit position!

					ApeBrush *brush = ( ApeBrush * ) node;
					for ( unsigned int j = 0; j < brush->numVertices; ++j )
					{
						brush->vertices[ j ] = qm_math_vector3f_add( brush->vertices[ j ], qm_math_vector3f_scale_float( gridDir, self->grid.size ) );
					}

					ape_brush_compute_bounds( brush );
					//TODO: make faces relative to brush so that this isn't necessary!!!
					ape_brush_compute_face_bounds( brush );

					ape_brush_mark_parent_dirty( brush );
					continue;
				}

				QmMathVector3f pos = ape_world_node_get_position( node );
				pos                = qm_math_vector3f_add( pos, qm_math_vector3f_scale_float( gridDir, self->grid.size ) );
				ape_world_node_set_position( node, &pos );
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////

static void save_command( unsigned int, char **argv )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		ape_console_warning_( "No active instance to close!\n" );
		return;
	}

	//TODO
}

static void load_command( unsigned int, char **argv )
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		ape_console_warning_( "No active instance to close!\n" );
		return;
	}

	//TODO
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void ape_initialize_editor_( void )
{
	AcmBranch *root = com_project_get_config();
	assert( root != NULL );
	editorConfigRoot = acm_get_child_by_name( root, "editor" );
	if ( editorConfigRoot == nullptr )
	{
		editorConfigRoot = acm_push_object( root, "editor" );
	}

	ape_editor_selection_initialize_();
}

void ape_shutdown_editor_( void )
{
	ape_editor_selection_shutdown_();
}

void ape_editor_set_geometry_mode( ApeEditorInstance *self, ApeEditorGeometryMode geometryMode )
{
	if ( self->geometryMode == geometryMode )
	{
		return;
	}

	unsigned int numBrushes = 0;
	ApeBrush   **brushes    = nullptr;
	// if we're switching from transform to face, let's put it into a list we can restore
	if ( geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE && self->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
	{
		unsigned int numSelected = qm_os_linked_list_get_size( self->selectedObjects );
		brushes                  = QM_OS_MEMORY_NEW_( ApeBrush *, numSelected );

		ApeWorldNode *node;
		QM_OS_LINKED_LIST_ITERATE( node, self->selectedObjects, i )
		{
			if ( node->type != APE_WORLD_NODE_TYPE_BRUSH )
			{
				continue;
			}

			brushes[ numBrushes++ ] = ( ApeBrush * ) node;
		}
	}
	// or face to transform
	else if ( geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM && self->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
	{
		unsigned int numSelected = qm_os_linked_list_get_size( self->selectedObjects );
		brushes                  = QM_OS_MEMORY_NEW_( ApeBrush *, numSelected );

		ApeBrushFace *face;
		QM_OS_LINKED_LIST_ITERATE( face, self->selectedObjects, i )
		{
			brushes[ numBrushes++ ] = face->parent;
		}
	}

	self->geometryMode = geometryMode;

	ape_editor_selection_rebuild_( self );

	// now restore it
	if ( brushes != nullptr )
	{
		if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_FACE )
		{
			for ( unsigned int i = 0; i < numBrushes; ++i )
			{
				ApeBrush *brush = brushes[ i ];
				for ( unsigned int j = 0; j < brush->numFaces; ++j )
				{
					ape_editor_add_object_to_selection( self, &brush->faces[ j ] );
				}
			}
		}
		else if ( self->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
		{
			for ( unsigned int i = 0; i < numBrushes; ++i )
			{
				ape_editor_add_object_to_selection( self, brushes[ i ] );
			}
		}

		qm_os_memory_free( brushes );
	}
}

QmMathVector2f ape_editor_get_default_material_scale()
{
	return editorDefaultMaterialScale;
}

/////////////////////////////////////////////////////////////////////////////////////

void ape_grid_toggle_command_( unsigned int, char ** );
void ape_light_command_( unsigned int, char ** );

void ape_editor_register_console_( void )
{
	PlRegisterConsoleCommand( "editor_toggle_grid", "Toggle the editing grid.", 0, ape_grid_toggle_command_ );
	PlRegisterConsoleCommand( "editor_save", "Save the current instance.", 1, save_command );
	PlRegisterConsoleCommand( "editor_load", "Load for the current instance.", 1, load_command );
	PlRegisterConsoleCommand( "editor_light", "Generate lightmap.", 0, ape_light_command_ );

	ape_console_var_register( "editor.showIcons", "Show icons in the editor mode.", "true", PL_VAR_BOOL, &showIcons, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );
	ape_console_var_register( "editor.iconFade", "Fade range for icons displayed in the editor.", "0.25", PL_VAR_F32, &iconFade, nullptr, APE_CONSOLE_VAR_FLAG_ARCHIVE );

	ape_console_var_register( "editor.materialScaleW", "Default material scale width.", "0.25", PL_VAR_F32, &editorDefaultMaterialScale.x, nullptr, 0 );
	ape_console_var_register( "editor.materialScaleH", "Default material scale height.", "0.25", PL_VAR_F32, &editorDefaultMaterialScale.y, nullptr, 0 );

	ape_console_var_register( "editor.showLightmapOverlay", "Show the lightmap overlaid on the screen, along with UV.", "false", PL_VAR_BOOL, &editorShowLightmapOverlay, nullptr, 0 );
}

static void pre_render_nodes( ApeEditorInstance *self, ApeCamera *camera, const ApeWorldNode *worldNode )
{
	assert( worldNode != NULL );

	// don't draw the sprite for the camera...
	const ApeWorldNode *cameraNode = ( ApeWorldNode * ) camera;
	if ( worldNode == cameraNode )
	{
		return;
	}

#if 0
	if ( showIcons )
	{
		if ( nodeIcons[ worldNode->type ] != NULL )
		{
			QmGfxTexture *texture = ape_material_get_texture_( nodeIcons[ worldNode->type ], 0, "diffuseMap" );
			if ( texture == nullptr )
			{
				return;
			}

			const float        size   = ( float ) texture->w;
			static const float scale  = 0.1f;
			const float        origin = -( size / 2.0f );

			QmMathColour4f colour;
			if ( worldNode->type == APE_WORLD_NODE_TYPE_LIGHT )
			{
				ApeLight *light = ( ApeLight * ) worldNode;
				colour          = QM_MATH_COLOUR4F_RGB( light->colour.r, light->colour.g, light->colour.b );
			}
			else
			{
				colour = QM_MATH_COLOUR4F_RGB( 1.0f, 1.0f, 1.0f );
			}

			ape_draw_sprite( nodeIcons[ worldNode->type ],
			                 &PL_QUAD( 0.0f, 0.0f, size, size ),
			                 &colour,
			                 &worldNode->position,
			                 &PL_VECTOR3( origin, origin, origin ),
			                 &PL_VECTOR3( 0.0f, camera->internal->angles.y, 0.0f ),
			                 scale );
		}
	}
#endif

	ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

	PlMatrixMode( PL_MODELVIEW_MATRIX );
	PlPushMatrix();
	PlLoadIdentityMatrix();

	ApeWorldNode *child;
	COM_ITERATE_LINKED_LIST( child, worldNode->children, i )
	{
		pre_render_nodes( self, camera, child );
	}

	PlPopMatrix();
}

static void draw_brush_gui( const ApeViewport *viewport, ApeGuiFont *font )
{
#if 0//todo
	ApeCamera *camera = viewport->camera;
	assert( camera != NULL );

	for ( unsigned int i = 0, num = 1; i < editorInstance->numPolygonPoints; ++i )
	{
		PLMatrix4 m              = PlMultiplyMatrix4( camera->internal->internal.proj, &camera->internal->internal.view );
		int       viewportSize[] = { viewport->x, viewport->y, viewport->width, viewport->height };
		QmMathVector2f screenPos      = PlConvertWorldToScreen( &editorInstance->polygonPoints[ i ], &m, viewportSize, true );
		if ( screenPos.x == 0.0f && screenPos.y == 0.0f )
		{
			return;
		}

		screenPos.x = floorf( screenPos.x );
		screenPos.y = floorf( screenPos.y );

		char msg[ 64 ];
		snprintf( msg, sizeof( msg ), "%u", num++ );
		//gui_font_draw_string( font, screenPos.x, screenPos.y, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, msg, strlen( msg ), true );

		QmMathVector3f end = ( i + 1 >= editorInstance->numPolygonPoints ) ? editorInstance->polygonPoints[ 0 ] : editorInstance->polygonPoints[ i + 1 ];

		QmMathVector2f otherScreenPos = PlConvertWorldToScreen( &end, &m, viewportSize, true );
		if ( otherScreenPos.x == 0.0f && otherScreenPos.y == 0.0f )
		{
			return;
		}

		// determine the point between the two on the screen
		QmMathVector2f midpointScreenPos = { ( screenPos.x + otherScreenPos.x ) / 2.0f, ( screenPos.y + otherScreenPos.y ) / 2.0f };
		snprintf( msg, sizeof( msg ), "%f (%s)", qm_math_vector3f_length( qm_math_vector3f_sub( end, editorInstance->polygonPoints[ i ] ) ), PlPrintVector3( &editorInstance->polygonPoints[ i ], PL_VAR_F32 ) );
		gui_font_draw_string( font, midpointScreenPos.x, midpointScreenPos.y, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, msg, strlen( msg ), true );
	}
#endif
}

void ape_editor_mode_transform_pre_render_( const ApeEditorInstance *instance );

void ape_editor_pre_render_scene_( ApeCamera *camera )
{
	if ( !ape_editor_is_active() )
	{
		return;
	}

	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == NULL )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	// slow, unoptimised, jelly

	if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_TRANSFORM )
	{
		ape_editor_mode_transform_pre_render_( instance );
	}

	ape_editor_selection_render_( instance );

	const ApeRoom *room = ape_camera_get_room( camera );
	if ( room != nullptr )
	{
		bool isWireframe = PlgIsGraphicsStateEnabled( PLG_GFX_STATE_WIREFRAME );
		if ( isWireframe )
		{
			PlgDisableGraphicsState( PLG_GFX_STATE_WIREFRAME );
		}

		pre_render_nodes( instance, camera, &room->base );

		if ( isWireframe )
		{
			PlgEnableGraphicsState( PLG_GFX_STATE_WIREFRAME );
		}
	}

	COM_PROFILE_FUNCTION_END();
}

void ape_editor_mode_polygon_post_render_( ApeEditorInstance *self );

void ape_editor_post_render_scene_()
{
	ApeEditorInstance *instance = ape_editor_get_active_instance();
	if ( instance == nullptr )
	{
		return;
	}

	COM_PROFILE_FUNCTION_START();

	ape_grid_draw_( &instance->grid );
	ape_grid_post_draw_( &instance->grid );

	if ( instance->geometryMode == APE_EDITOR_GEOMETRY_MODE_PLOT )
	{
		ape_editor_mode_polygon_post_render_( instance );
	}

	COM_PROFILE_FUNCTION_END();
}

static void draw_node_text_overlay( ApeEditorInstance *self, ApeWorldNode *root, const ApeViewport *viewport, ApeGuiFont *font, const PLMatrix4 *viewProj )
{
	if ( !showIcons )
	{
		return;
	}

	ApeWorldNode *node;
	COM_ITERATE_LINKED_LIST( node, root->children, i )
	{
		draw_node_text_overlay( self, node, viewport, font, viewProj );

		QmMathVector3f origin;
		if ( node->type == APE_WORLD_NODE_TYPE_BRUSH )
		{
			continue;
		}

		origin = ape_world_node_get_position( node );

		float          depth;
		QmMathVector2f screenPos = PlConvertWorldToScreen( &origin, viewProj, ( int[] ) { 0, 0, viewport->width, viewport->height }, &depth, true );
		if ( depth <= 0.0f )
		{
			continue;
		}

		ApeMaterial *material = nullptr;
		if ( node->type == APE_WORLD_NODE_TYPE_ENTITY )
		{
			// as usual, entities are a special case and can set their own icon
			material = ( ( ApeEntity * ) node )->editorSprite != nullptr ? ( ( ApeEntity * ) node )->editorSprite : nodeIcons[ APE_WORLD_NODE_TYPE_ENTITY ];
		}
		else
		{
			material = nodeIcons[ node->type ];
		}

		assert( material != nullptr );

		const float pixelScale = material != nullptr ? ape_material_get_width( material ) : 64.0f;
		float       scale      = QM_MATH_CLAMP( 0.0f, ( pixelScale * 2.0f ) - ( depth * ( ( pixelScale ) / 100.0f ) ) * iconFade, 64.0f );
		if ( scale <= 0.0f )
		{
			continue;
		}

		if ( node->type == APE_WORLD_NODE_TYPE_ENTITY )
		{
			const ApeEntity                *entity          = ( ApeEntity * ) node;
			const ApeEntityClassDefinition *classDefinition = entity->classDefinition;
			assert( classDefinition != nullptr );
			size_t size = strlen( classDefinition->name );

			float sw;
			gui_font_get_string_pixel_size( font, 1.0f, classDefinition->name, size, &sw, nullptr );
			gui_font_draw_string( font, screenPos.x - sw / 2.0f, screenPos.y, nullptr, nullptr, 1.0f, &PL_COLOUR_GOLD, classDefinition->name, size, true );
		}
		else
		{
			const ApeWorldNodeClass *classType = node->classType;
			assert( classType != nullptr );
			size_t size = strlen( classType->identifier );

			float sw;
			gui_font_get_string_pixel_size( font, 1.0f, classType->identifier, size, &sw, nullptr );
			gui_font_draw_string( font, screenPos.x - ( sw / 2.0f ), screenPos.y, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, classType->identifier, size, true );
		}

		if ( material != nullptr )
		{
			QmMathColour4ub colour;
			if ( node->type == APE_WORLD_NODE_TYPE_LIGHT )
			{
				ApeLight *light = ( ApeLight * ) node;
				colour          = QM_MATH_COLOUR4F_TO_4UB( light->colour );
				colour.a        = 255;
			}
			else
			{
				colour = QM_MATH_COLOUR4UB( 255, 255, 255, 255 );
			}

			ape_draw_textured_quad( material, screenPos.x - ( scale / 2.0f ), screenPos.y - scale, scale, scale, &colour, 0 );
		}
	}
}

void ape_editor_light_display_lightmap_overlay_( const ApeEditorInstance *instance );

void ape_editor_draw_gui_( const ApeViewport *viewport )
{
	if ( !ape_editor_is_active() || editorInstance == nullptr )
	{
		return;
	}

	ApeCamera *camera = viewport->camera;
	if ( camera == nullptr )
	{
		return;
	}

	ApeGuiFont *font = gui_get_default_font( GUI_FONT_DEFAULT_SMALL );

	if ( editorInstance->camera != nullptr )
	{
		char label[ 64 ];
		snprintf( label, sizeof( label ), "%s/%s\n", ape_get_camera_view_mode_label( camera->mode ), ape_get_camera_draw_mode_label( camera->drawMode ) );

		float dw, dh;
		gui_font_get_string_pixel_size( font, 1.0f, label, strlen( label ), &dw, &dh );
		gui_font_draw_string( font, ( float ) viewport->width - ( dw + dh ), ( float ) viewport->height - ( ( dh * 2.0f ) - 2.0f ), nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, label, strlen( label ), true );

		PLMatrix4 viewProj = PlMultiplyMatrix4( &camera->proj, &camera->view );

		// check the camera has a valid room, otherwise display a warning
		ApeRoom *room = ape_camera_get_room( editorInstance->camera );
		if ( room != nullptr )
		{
			draw_node_text_overlay( editorInstance, &room->base, viewport, font, &viewProj );
		}
		else
		{
			static const char *warning = "No active room for camera!\n";
			gui_font_draw_string( font, 0.0f, 0.0f, &dw, &dh, 1.0f, &PL_COLOUR_CRIMSON, warning, strlen( warning ), false );
		}

		if ( camera->mode == APE_CAMERA_MODE_PERSPECTIVE )
		{
			QmMathVector2f pos;
			if ( ape_grid_get_cursor_position( &editorInstance->grid, &pos ) != nullptr )
			{
				ape_set_active_shader_by_default_( APE_SHADER_DEFAULT_VERTEX );

				static constexpr float scale = 16.0f;

				QmMathVector3f worldPos = ape_grid_transform_point( &editorInstance->grid, &pos );

				float          w;
				QmMathVector2f screenPos = PlConvertWorldToScreen( &worldPos, &viewProj, ( int[] ) { 0, 0, viewport->width, viewport->height }, &w, true );
				if ( w > 0.f )
				{
					PlgDrawLineRectangle( screenPos.x - ( scale / 2.0f ), screenPos.y - ( scale / 2.0f ), scale, scale, PL_COLOUR_WHITE );
				}
			}
		}
	}

	//TODO: this should be drawn within the grid space, so it's more visible
	if ( editorInstance->geometryMode == APE_EDITOR_GEOMETRY_MODE_PLOT )
	{
		char label[ 64 ];
		snprintf( label, sizeof( label ), "x%.0f y%.0f w%d h%d\n",
		          editorInstance->polySize.x, editorInstance->polySize.y,
		          ( int ) editorInstance->polySize.w, ( int ) editorInstance->polySize.h );

		float dw, dh;
		gui_font_get_string_pixel_size( font, 1.0f, label, strlen( label ), &dw, &dh );
		gui_font_draw_string( font, viewport->width / 2.0f - dw / 2.0f, viewport->height - dh * 2.0f - 2.0f, nullptr, nullptr, 1.0f, &PL_COLOUR_WHITE, label, strlen( label ), true );
	}

	if ( editorShowLightmapOverlay )
	{
		ape_editor_light_display_lightmap_overlay_( editorInstance );
	}

	draw_brush_gui( viewport, font );

	gui_font_display( font );
}

void ape_editor_on_mouse_move( ApeEditorInstance *self, const ApeViewport *viewport, int x, int y )
{
	ape_grid_update_cursor_( &self->grid, x, y, self->camera, viewport );

	QmMathColour4ub pixel;
	if ( ape_editor_selection_get_pixel_under_cursor_( &pixel ) != nullptr )
	{
		self->hoverSelection = PlLookupHashTableUserData( self->selectionTable, &pixel, sizeof( QmMathColour4ub ) );
	}
	else
	{
		self->hoverSelection = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Material Preview
/////////////////////////////////////////////////////////////////////////////////////

static PLHashTable *editorMaterialPreviews;

static constexpr const char PREVIEW_FALLBACK_PATH[] = "materials/editor/no_preview.png";
static PLImage             *previewFallback;

static PLImage *get_material_preview_image( const char *path )
{
	AcmBranch *root = com_acm_load_file( path, "material" );
	if ( root == nullptr )
	{
		ape_console_warning_( "Failed to load material (%s) for preview!\n", path );
		return nullptr;
	}

	PLImage    *preview;
	const char *previewPath = acm_get_string( root, "previewTexture", nullptr );
	if ( previewPath != NULL )
	{
		preview = PlLoadImage( previewPath );
	}
	else
	{
		// the painful way...
		AcmBranch *diffuseNode = acm_linear_lookup( root, "diffuseMap" );
		if ( diffuseNode == nullptr )
		{
			ape_console_warning_( "Failed to find preview texture to use under material (%s)!\n", path );
			return nullptr;
		}

		PLPath buf;
		if ( acm_branch_get_string( diffuseNode, buf, sizeof( buf ) ) != ND_ERROR_SUCCESS )
		{
			ape_console_warning_( "Diffuse texture under material (%s) was not a valid string!\n", path );
			return nullptr;
		}

		preview = PlLoadImage( buf );
	}

	return preview;
}

static void cleanup_preview( void *user )
{
	PLImage *image = user;
	PlDestroyImage( image );
}

void ape_editor_flush_material_previews()
{
	PlDestroyHashTableEx( editorMaterialPreviews, cleanup_preview );
	editorMaterialPreviews = nullptr;

	PlDestroyImage( previewFallback );
	previewFallback = nullptr;
}

PLImage *ape_editor_get_material_preview( const char *path, uint16_t width, uint16_t height )
{
	// lazy init of fallback and previews list, bleh
	if ( previewFallback == nullptr )
	{
		previewFallback = PlLoadImage( PREVIEW_FALLBACK_PATH );
		if ( previewFallback == nullptr )
		{
			ape_console_error_( true, "Failed to load preview fallback (%s): %s\n", PREVIEW_FALLBACK_PATH, PlGetError() );
		}
	}

	if ( editorMaterialPreviews == nullptr )
	{
		editorMaterialPreviews = PlCreateHashTable();
		if ( editorMaterialPreviews == nullptr )
		{
			ape_console_error_( true, "Failed to create material preview table: %s\n", PlGetError() );
		}
	}

	// setup the name we'll use for the cache table
	PLPath hashName;
	if ( PlSetupPath( hashName, false, "%s_%u_%u", path, width, height ) == nullptr )
	{
		ape_console_warning_( "Failed to setup hash name for material preview (%s)!\n", path );
		return previewFallback;
	}
	size_t hashNameLength = strlen( hashName );

	PLImage *preview = PlLookupHashTableUserData( editorMaterialPreviews, hashName, hashNameLength );
	if ( preview != nullptr )
	{
		return preview;
	}

	// if the preview is null, then attempt to load it from the preview cache

	const char *appDataDir = com_get_app_data_directory();

	PLPath cachePath;
	PlSetupPath( cachePath, true, "%s/cache", appDataDir );
	if ( PlCreatePath( cachePath ) )
	{
		PlSetupPath( cachePath, true, "%s/cache/preview_%lu.qoi", appDataDir, PlGenerateHashFNV1( hashName, hashNameLength ) );
		if ( qm_fs_check_file_exists( cachePath ) )
		{
			if ( ( preview = PlLoadImage( cachePath ) ) != nullptr )
			{
				PlInsertHashTableNode( editorMaterialPreviews, hashName, hashNameLength, preview );
				return preview;
			}

			ape_console_warning_( "Failed to load cached image (%s): %s\n", cachePath, PlGetError() );
		}
		else
		{
			ape_console_print_( "Cache not found for material (%s). Caching...\n", path );
		}
	}
	else
	{
		ape_console_warning_( "Failed to create cache directory (%s): %s\n", cachePath, PlGetError() );
	}

	// and now the worst case, it's not yet been cached...
	if ( ( preview = get_material_preview_image( path ) ) == nullptr )
	{
		ape_console_warning_( "No preview image for material (%s): %s\n", path, PlGetError() );

		// go ahead and use the fallback instead
		return previewFallback;
	}

	if ( preview->width > width || preview->height > height )
	{
		// maintain aspect if we can
		if ( preview->width < width )
		{
			width = preview->width;
		}
		if ( preview->height < height )
		{
			height = preview->height;
		}

		PLImage *newPreview = PlResizeImage( preview, width, height );
		if ( newPreview != nullptr )
		{
			PlDestroyImage( preview );
			preview = newPreview;
		}
		else
		{
			ape_console_warning_( "Failed to resize preview for material (%s): %s\n", path, PlGetError() );
		}
	}

	if ( !PlWriteImage( preview, cachePath, 0 ) )
	{
		ape_console_warning_( "Failed to write image to cache (%s): %s\n", cachePath, PlGetError() );
	}

	PlInsertHashTableNode( editorMaterialPreviews, hashName, hashNameLength, preview );

	return preview;
}

/////////////////////////////////////////////////////////////////////////////////////

#if !defined( NDEBUG )

bool ape_editor_validate_properties_( const ApeProperty *properties, const unsigned int numProperties )
{
	bool valid = true;
	for ( unsigned int i = 0; i < numProperties; ++i )
	{
		const char *typeName = nullptr;
		switch ( properties[ i ].type )
		{
			default:
				ape_console_warning_( "Encountered invalid property type during validation (%u)!\n" );
				valid = false;
				break;
			case APE_PROPERTY_TYPE_FLOAT:
				typeName = "ApeFloatProperty";
				break;
			case APE_PROPERTY_TYPE_VEC2:
				typeName = "ApeVec2Property";
				break;
			case APE_PROPERTY_TYPE_VEC3:
				typeName = "ApeVec3Property";
				break;
			case APE_PROPERTY_TYPE_VEC4:
				typeName = "ApeVec4Property";
				break;
			case APE_PROPERTY_TYPE_ENUM:
				typeName = "ApeEnumProperty";
				break;
			case APE_PROPERTY_TYPE_COLOUR:
				typeName = "ApeColour4fProperty";
				break;
			case APE_PROPERTY_TYPE_INTEGER:
			case APE_PROPERTY_TYPE_BITFLAG:
				typeName = "ApeIntegerProperty";
				break;
			case APE_PROPERTY_TYPE_STRING:
			case APE_PROPERTY_TYPE_PATH:
				typeName = "ApeStringProperty";
				break;
			case APE_PROPERTY_TYPE_BOOLEAN:
				typeName = "ApeBooleanProperty";
				break;
		}

		if ( strcmp( typeName, properties[ i ].typeName ) != 0 )
		{
			ape_console_warning_( "Encountered invalid property, \"%s\" (%s != %s)!\n", properties[ i ].internalName, typeName, properties[ i ].typeName );
			valid = false;
		}
	}

	return valid;
}

#endif

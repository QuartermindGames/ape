// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "forge_viewport.h"

namespace forge
{
	class WorldEditor;
	class WorldViewport : public Viewport
	{
		FXDECLARE( WorldViewport )

	public:
		WorldViewport( FXComposite *composite, FXGLVisual *visual, WorldEditor *editor, ApeCameraViewMode viewMode );
		~WorldViewport() override = default;

		enum
		{
			ID_GRID_ALIGN = Viewport::ID_LAST,

			ID_FACE_INSPECTOR,
			ID_FACE_TOGGLE,
			ID_FACE_TOGGLE_OTHERS,

			ID_FACE_FLAG_MIRROR,

			ID_FACE_FLIP,
			ID_FACE_SHADE_SMOOTH,
			ID_FACE_SHADE_FLAT,
			ID_FACE_LINK_NEW_ROOM,
			ID_FACE_LINK_PORTAL,
			ID_FACE_UNLINK_PORTAL,

			ID_MOVE_NODE_TO_ROOM,

			ID_NODE_ATTACH,

			ID_MERGE,
			ID_EXPORT,
			ID_IMPORT,
			ID_OPEN_PROPERTIES,

			ID_CREATE_NODE,
			ID_CREATE_NODE_END = ID_CREATE_NODE + APE_WORLD_MAX_NODE_TYPES,

			ID_LAST,
		};

		long on_left_click( FXObject *, FXSelector, void * ) override;
		long on_right_click( FXObject *, FXSelector, void * ) override;
		long on_middle_click( FXObject *, FXSelector, void * ) override;

		long on_key( FXObject *, FXSelector, void * ) override;
		long on_motion( FXObject *, FXSelector, void * ) override;

		long on_grid_align( FXObject *, FXSelector, void * );
		long on_face_inspector( FXObject *, FXSelector, void * );
		long on_face_toggle( FXObject *, FXSelector, void * );
		long on_face_shade_smooth( FXObject *, FXSelector, void * );
		long on_face_shade_flat( FXObject *, FXSelector, void * );
		long on_face_flip( FXObject *, FXSelector, void * );
		long on_link_new_room( FXObject *, FXSelector, void * );
		long on_face_unlink_portal( FXObject *, FXSelector, void * );
		long on_face_link_portal( FXObject *, FXSelector, void * );

		long on_node_attach( FXObject *, FXSelector, void * );

		long on_move_node_to_room( FXObject *, FXSelector, void * );

		long on_toggle_face_flag( FXObject *, FXSelector, void * );

		long on_merge( FXObject *, FXSelector, void * );
		long on_export( FXObject *, FXSelector, void * );
		long on_import( FXObject *, FXSelector, void * );
		long on_open_properties( FXObject *, FXSelector, void * );

		long on_create_node( FXObject *, FXSelector, void * );

	protected:
		WorldViewport() = default;
	};
}// namespace forge

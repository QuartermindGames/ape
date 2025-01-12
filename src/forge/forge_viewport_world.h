// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

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
			ID_FACE_FLIP,
			ID_FACE_SMOOTH,

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
		long on_face_smooth( FXObject *, FXSelector, void * );
		long on_face_flip( FXObject *, FXSelector, void * );

		long on_create_node( FXObject *, FXSelector, void * );

	protected:
		WorldViewport() = default;
	};
}// namespace forge

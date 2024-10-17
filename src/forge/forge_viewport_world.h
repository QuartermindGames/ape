// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

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
		inline ~WorldViewport() override = default;

		enum
		{
			ID_GRID_ALIGN = Viewport::ID_LAST,

			ID_FACE_TOGGLE,
			ID_FACE_TOGGLE_OTHERS,

			ID_LAST,
		};

		long on_left_click( FXObject *, FXSelector, void * ) override;
		long on_right_click( FXObject *, FXSelector, void * ) override;
		long on_key( FXObject *, FXSelector, void * ) override;
		long on_motion( FXObject *, FXSelector, void * ) override;

		long on_grid_align( FXObject *, FXSelector, void * );
		long on_face_toggle( FXObject *, FXSelector, void * );

	protected:
		inline WorldViewport() = default;
	};
}// namespace forge

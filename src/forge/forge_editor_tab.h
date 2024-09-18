// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge/forge.h"

namespace forge
{
	class EditorTab : public FXTabItem
	{
		FXDECLARE( EditorTab )

	protected:
		inline EditorTab() = default;

	public:
		EditorTab( FXTabBook *owner, const FXString &heading, FXIcon *icon, ApeEditorMode mode );
		~EditorTab() override;

	protected:
		ApeEditorInstance instance{};

	public:
		ApeEditorInstance *get_internal()
		{
			return &instance;
		}

		void set_camera( ApeCamera *camera )
		{
			instance.camera = camera;
		}
	};
}// namespace forge

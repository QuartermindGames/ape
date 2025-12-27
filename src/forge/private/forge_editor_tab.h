// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "forge.h"

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
		ApeEditorInstance instance_{};

	public:
		ApeEditorInstance *get_internal()
		{
			return &instance_;
		}

		void set_camera( ApeCamera *camera )
		{
			instance_.camera = camera;
		}

		virtual void autosave() {}
	};
}// namespace forge

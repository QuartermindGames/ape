// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "forge.h"
#include "forge_editor_tab.h"

struct ApeModel;

namespace forge
{
	class Viewport;

	class editor_model : public EditorTab
	{
		FXDECLARE( editor_model )

	protected:
		inline editor_model() = default;

	public:
		editor_model( FXTabBook *owner, const FXString &modelName, ApeModel *model );
		~editor_model() override;

	private:
		ApeModel *model{ nullptr };

		ApeWorld  *world{ nullptr };
		ApeEntity *modelEntity{ nullptr };

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		FXToggleButton *_editModeButtons[ APE_EDITOR_MAX_GEOMETRY_MODES ]{};

		Viewport *_viewport{};

		void autosave() override {}
	};
}// namespace forge

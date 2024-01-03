// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../editor.h"

namespace ss::forge
{
	class ViewportFrame;

	class WorldEditor : public FXTabItem
	{
		FXDECLARE( WorldEditor )

	protected:
		inline WorldEditor() = default;

	public:
		WorldEditor( FXTabBook *owner, const FXString &worldName, ApeWorld *world );
		~WorldEditor() override;

	private:
		ApeWorld *_world{ nullptr };

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		FXToggleButton *_editModeButtons[ EDITOR_MAX_GEOMETRYMODES ]{};

		ViewportFrame *_viewportFrames[ 4 ]{};
	};
}// namespace ss::forge

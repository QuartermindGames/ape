// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../editor.h"

struct ApeModel;

namespace ss::forge
{
	class viewport_frame;

	class ModelEditor : public FXTabItem
	{
		FXDECLARE( ModelEditor )

	protected:
		inline ModelEditor() = default;

	public:
		ModelEditor( FXTabBook *owner, const FXString &modelName, ApeModel *model );
		~ModelEditor() override;

	private:
		ApeModel *model{ nullptr };

		ApeWorld *world{ nullptr };
		ApeEntity *modelEntity{ nullptr };

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		FXToggleButton *_editModeButtons[ APE_EDITOR_MAX_GEOMETRY_MODES ]{};

		viewport_frame *_viewport{};
	};
}// namespace ss::forge

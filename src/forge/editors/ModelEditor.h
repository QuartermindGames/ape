// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../editor.h"

namespace ss::forge
{
	struct SSApeModel;
	class ViewportFrame;

	class ModelEditor : public FXTabItem
	{
		FXDECLARE( ModelEditor )

	protected:
		inline ModelEditor() = default;

	public:
		ModelEditor( FXTabBook *owner, const FXString &modelName, SSApeModel *model );
		~ModelEditor() override;

	private:
		SSApeModel *_model{ nullptr };

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		FXToggleButton *_editModeButtons[ EDITOR_MAX_GEOMETRYMODES ]{};

		ViewportFrame *_viewport{};
	};
}// namespace ss::forge

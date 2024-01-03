// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../editor.h"

namespace ss::forge
{
	class ViewportFrame;

	class MaterialEditor : public FXTabItem
	{
		FXDECLARE( MaterialEditor )

	protected:
		inline MaterialEditor() = default;

	public:
		MaterialEditor( FXTabBook *owner, const FXString &worldName, ApeMaterial *material );
		~MaterialEditor() override;

	private:
		ApeWorld *_material{ nullptr };

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		ViewportFrame *_viewport;
	};
}// namespace ss::forge

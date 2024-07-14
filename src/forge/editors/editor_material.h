// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge/forge.h"

namespace forge
{
	class Viewport;

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

		Viewport *_viewport;
	};
}// namespace forge

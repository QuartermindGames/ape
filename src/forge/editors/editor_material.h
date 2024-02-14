// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../editor.h"

namespace ss::forge
{
	class viewport_frame;

	class editor_material : public FXTabItem
	{
		FXDECLARE( editor_material )

	protected:
		inline editor_material() = default;

	public:
		editor_material( FXTabBook *owner, const FXString &worldName, ApeMaterial *material );
		~editor_material() override;

	private:
		ApeWorld *_material{ nullptr };

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		viewport_frame *_viewport;
	};
}// namespace ss::forge

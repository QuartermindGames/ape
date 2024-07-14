// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

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
		EditorTab( FXTabBook *owner, const FXString &heading, FXIcon *icon = nullptr );
		~EditorTab() override;

	protected:
		ApeEditorState instance{};

	public:
		ApeEditorState *get_internal()
		{
			return &instance;
		}
	};
}// namespace forge

// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge.h"

namespace forge
{
	class AboutDialog : public FXDialogBox
	{
	public:
		explicit AboutDialog( FXWindow *parent );
		~AboutDialog() override = default;

	private:
		static FXIcon *editorIcon;
		static FXIcon *developerIcon;
	};
}// namespace forge
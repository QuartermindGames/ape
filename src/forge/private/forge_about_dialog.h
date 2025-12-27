// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

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
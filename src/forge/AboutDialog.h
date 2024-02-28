// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "editor.h"

namespace ss::forge
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
}// namespace ss::forge
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

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
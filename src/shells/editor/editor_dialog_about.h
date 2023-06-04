// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

namespace os::editor
{
	class AboutDialog : public FXDialogBox
	{
	public:
		AboutDialog( FXWindow *parent );
		~AboutDialog() = default;
	};
}
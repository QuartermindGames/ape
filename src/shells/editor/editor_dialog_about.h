// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

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
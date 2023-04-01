// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

namespace os::editor
{
	class ProjectDialog : public FXDialogBox
	{
	public:
		ProjectDialog( FXWindow *parent );
		~ProjectDialog() = default;

	protected:
	private:
		static void RegisterProjectCallback( const char *, void * );
		std::map< std::string, editor::Project > projects;
	};
}// namespace os::editor

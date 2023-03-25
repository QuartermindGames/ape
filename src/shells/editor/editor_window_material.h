// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

class EditorMainWindow;
class EditorMaterialWindow : FXWindow
{
	FXDECLARE( EditorMaterialWindow )

	inline EditorMaterialWindow() = default;

public:
	EditorMaterialWindow( EditorMainWindow *parent );

private:

};

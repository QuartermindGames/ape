// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

class EditorMainWindow;
class EditorFaceInspector : public FXDialogBox
{
	FXDECLARE( EditorFaceInspector )

	inline EditorFaceInspector() {};

public:
	EditorFaceInspector( EditorMainWindow *parent );
};

// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

namespace os::editor
{
	class ViewportFrame;

	class MaterialWindow : public FXTopWindow
	{
		FXDECLARE( MaterialWindow )

	public:
		explicit MaterialWindow( FXApp *app, ApeMaterial *material );
		~MaterialWindow();

	protected:
		inline MaterialWindow() = default;

	private:
		ViewportFrame *viewportFrame;
		ApeMaterial *material;
	};
}// namespace os::editor

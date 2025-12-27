// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "forge_viewport.h"

namespace forge
{
	class ModelEditor;
	class ModelViewport : public Viewport
	{
		FXDECLARE( ModelViewport )

	public:
		ModelViewport( ModelEditor *editor, FXComposite *composite, FXGLVisual *visual );
		~ModelViewport() override;

		void draw() override;

	protected:
		inline ModelViewport() = default;
	};
}// namespace forge

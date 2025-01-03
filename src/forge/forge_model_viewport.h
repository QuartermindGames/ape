// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge/forge_viewport.h"

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

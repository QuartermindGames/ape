// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "forge_model_viewport.h"
#include "forge_model_editor.h"

FXDEFMAP( forge::ModelViewport )
modelViewportMap[] = {

};

FXIMPLEMENT( forge::ModelViewport, forge::Viewport, modelViewportMap, ARRAYNUMBER( modelViewportMap ) )

forge::ModelViewport::ModelViewport( forge::ModelEditor *editor, FXComposite *composite, FXGLVisual *visual )
{
}

forge::ModelViewport::~ModelViewport()
{
}

void forge::ModelViewport::draw()
{
	Viewport::draw();
}

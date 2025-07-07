// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Base tab implementation for editors.
// Author:  Mark E. Sowden

#include "forge.h"

#include "forge_editor_tab.h"

FXDEFMAP( forge::EditorTab )
editorTabMap[] = {};
FXIMPLEMENT( forge::EditorTab, FXTabItem, editorTabMap, ARRAYNUMBER( editorTabMap ) )

forge::EditorTab::EditorTab( FXTabBook *owner, const FXString &heading, FXIcon *icon, ApeEditorMode mode )
    : FXTabItem( owner, heading )
{
	if ( icon != nullptr )
	{
		setIcon( icon );
	}

	if ( ape_editor_instance_setup( &instance_, mode ) == nullptr )
	{
		throw std::runtime_error( "Failed to initialize ApeEditorState" );
	}
}

forge::EditorTab::~EditorTab()
{
	ape_editor_instance_cleanup( &instance_ );
}

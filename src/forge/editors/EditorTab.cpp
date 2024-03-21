// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Base tab implementation for editors.
// Author:  Mark E. Sowden

#include "../editor.h"

#include "EditorTab.h"

FXDEFMAP( ss::forge::EditorTab )
editorTabMap[] = {};
FXIMPLEMENT( ss::forge::EditorTab, FXTabItem, editorTabMap, ARRAYNUMBER( editorTabMap ) )

ss::forge::EditorTab::EditorTab( FXTabBook *owner, const FXString &heading, FXIcon *icon )
    : FXTabItem( owner, heading )
{
	if ( icon != nullptr )
	{
		setIcon( icon );
	}

	if ( ape_editor_instance_initialize( &instance ) == nullptr )
	{
		throw std::runtime_error( "Failed to initialize ApeEditorState" );
	}
}

ss::forge::EditorTab::~EditorTab()
{
	ape_editor_instance_shutdown( &instance );
}

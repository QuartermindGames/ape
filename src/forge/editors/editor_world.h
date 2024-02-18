// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../editor.h"

namespace ss::forge
{
	class viewport_frame;

	class editor_world : public FXTabItem
	{
		FXDECLARE( editor_world )

	protected:
		inline editor_world() = default;

	public:
		editor_world( FXTabBook *owner, const FXString &worldName, ApeWorld *world );
		~editor_world() override;

		void create_new_entity( ApeWorldNode *parent = nullptr );

		void update_tree();

	private:
		ApeWorld *_world{};

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		FXToggleButton *_editModeButtons[ APE_EDITOR_MAX_GEOMETRY_MODES ]{};

#if 0
		viewport_frame *_viewportFrames[ 4 ]{};
#else
		viewport_frame *viewportFrame{};
#endif

		FXTreeList *nodeTree{};

	private:
		class entity_creation_dialog : FXDialogBox
		{
			FXDECLARE( entity_creation_dialog )

		public:
			explicit entity_creation_dialog( FXWindow *parent );
			~entity_creation_dialog() = default;

		protected:
			inline entity_creation_dialog() = default;

		private:
			FXListBox *classSelection{};
		};
	};
}// namespace ss::forge

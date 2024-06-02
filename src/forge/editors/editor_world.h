// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "../editor.h"

#include "EditorTab.h"

namespace ss::forge
{
	class viewport_frame;

	class editor_world : public EditorTab
	{
		FXDECLARE( editor_world )

	protected:
		inline editor_world() = default;

	public:
		enum
		{
			ID_SELECT_MODE = FXTabItem::ID_LAST,
			ID_FACE_MODE,
			ID_EDGE_MODE,
			ID_VERTEX_MODE,
			ID_TRANSFORM_MODE,

			ID_GRID_UP,
			ID_GRID_DOWN,
			ID_GRID_ROTATE,
		};

		editor_world( FXTabBook *owner, const FXString &worldName, ApeWorld *world );
		~editor_world() override;

		void create_new_object( const char *name, ApeWorldNodeType type );

		void update_tree();

		long on_change_geometry_mode( FXObject *, FXSelector, void * );
		long on_shift_grid( FXObject *, FXSelector, void * );

	private:
		ApeWorld *_world{};

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		FXToggleButton *geometryModeButtons[ APE_EDITOR_MAX_GEOMETRY_MODES ]{};

		FXTreeList *nodeTree{};
		FXComboBox *brushClassBox{};

		viewport_frame *viewports[ APE_EDITOR_MAX_VIEWPORTS ];

	private:
		class room_creation_dialog : FXDialogBox
		{
			FXDECLARE( room_creation_dialog )

		public:
		protected:
		private:
		};

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

	public:
		inline ApeWorld *get_world() { return _world; }
	};
}// namespace ss::forge

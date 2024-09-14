// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge/forge.h"
#include "forge/forge_editor_tab.h"

namespace forge
{
	class Viewport;

	class WorldEditor : public EditorTab
	{
		FXDECLARE( WorldEditor )

	protected:
		inline WorldEditor() = default;

	public:
		enum
		{
			ID_POLY_MODE = FXTabItem::ID_LAST,
			ID_FACE_MODE,
			ID_EDGE_MODE,
			ID_VERTEX_MODE,
			ID_TRANSFORM_MODE,

			ID_ROOM_NEW,
			ID_ROOM_EDIT,
			ID_ROOM_SELECT,

			ID_GRID_UP,
			ID_GRID_DOWN,
			ID_GRID_ALIGN,
		};

		WorldEditor( FXTabBook *owner, const FXString &worldName, ApeWorld *world );
		~WorldEditor() override;

		void create_new_object( const char *name, ApeWorldNodeType type );

		void update_tree();

		long on_change_geometry_mode( FXObject *, FXSelector, void * );
		long on_shift_grid( FXObject *, FXSelector, void * );
		long on_room_select( FXObject *, FXSelector, void * );
		long on_new_room( FXObject *, FXSelector, void * );

	private:
		ApeWorld *_world{};

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		FXToggleButton *geometryModeButtons[ APE_EDITOR_MAX_GEOMETRY_MODES ]{};

		FXTreeList *nodeTree{};
		FXComboBox *roomSelectBox{};

		Viewport *viewports[ APE_EDITOR_MAX_VIEWPORTS ];

	public:
		class RoomCreationDialog : public FXDialogBox
		{
			FXDECLARE( RoomCreationDialog )

		protected:
			inline RoomCreationDialog() = default;

		public:
			explicit RoomCreationDialog( FXWindow *parent );
			~RoomCreationDialog() override = default;

		protected:
		private:
		};

		class RoomPropertiesDialog : public FXDialogBox
		{
			FXDECLARE( RoomPropertiesDialog )

		protected:
			inline RoomPropertiesDialog() = default;

		public:
			explicit RoomPropertiesDialog( FXWindow *parent );
			~RoomPropertiesDialog() override = default;
		};

		class EntityCreationDialog : FXDialogBox
		{
			FXDECLARE( EntityCreationDialog )

		public:
			explicit EntityCreationDialog( FXWindow *parent );
			~EntityCreationDialog() = default;

		protected:
			inline EntityCreationDialog() = default;

		private:
			FXListBox *classSelection{};
		};

	public:
		inline ApeWorld *get_world() { return _world; }
	};
}// namespace forge

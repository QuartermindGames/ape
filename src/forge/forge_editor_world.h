// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge/forge.h"
#include "forge/forge_editor_tab.h"

namespace forge
{
	class WorldViewport;
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

			ID_ROOM_SAVE,
			ID_ROOM_NEW,
			ID_ROOM_EDIT,
			ID_ROOM_DELETE,
			ID_ROOM_SELECT,

			ID_MATERIAL_BROWSER,
			ID_OPEN_PROPERTIES,

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

		long on_room_save( FXObject *, FXSelector, void * );
		long on_room_select( FXObject *, FXSelector, void * );
		long on_new_room( FXObject *, FXSelector, void * );
		long on_edit_room( FXObject *, FXSelector, void * );

		void set_active_room( ApeRoom *room );

		long on_material_browser( FXObject *, FXSelector, void * );
		long on_properties( FXObject *, FXSelector, void * );

	private:
		ApeWorld *_world{};
		ApeRoom  *activeRoom{};

		FXDataTarget _gridHideTarget;
		FXDataTarget _gridSizeTarget;

		FXToggleButton *geometryModeButtons[ APE_EDITOR_MAX_GEOMETRY_MODES ]{};

		FXTreeList *nodeTree{};
		FXComboBox *roomSelectBox{};

		WorldViewport *viewports[ APE_EDITOR_MAX_VIEWPORTS ];

	public:
		class RoomDialog : public FXDialogBox
		{
			FXDECLARE( RoomDialog )

		protected:
			inline RoomDialog() = default;

		public:
			explicit RoomDialog( FXWindow *parent, ApeRoom *room );
			~RoomDialog() override = default;

			inline FXString get_room_name() { return nameField->getText(); }

			inline PLColourF32 get_room_ambience()
			{
				FXColor color = ambienceField->getRGBA();
				return PL_COLOURF32( PlByteToFloat( FXREDVAL( color ) ),
				                     PlByteToFloat( FXGREENVAL( color ) ),
				                     PlByteToFloat( FXBLUEVAL( color ) ),
				                     PlByteToFloat( FXALPHAVAL( color ) ) );
			}

			inline ApeAudioReverbPreset get_room_audio_preset()
			{
				return ( ApeAudioReverbPreset ) audioPresetField->getCurrentItem();
			}

		protected:
		private:
			FXTextField *nameField;
			FXColorWell *ambienceField;
			FXListBox   *audioPresetField;
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

		class TexturePicker : FXTopWindow
		{
			FXDECLARE( TexturePicker )

		protected:
			inline TexturePicker() = default;

		public:
			explicit TexturePicker( FXWindow *parent );
			~TexturePicker() override = default;
		};

	private:
		TexturePicker *texturePicker;

	public:
		inline ApeWorld *get_world() { return _world; }
	};
}// namespace forge

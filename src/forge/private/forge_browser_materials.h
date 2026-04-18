// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

namespace forge
{
	class MaterialBrowser : public FXDialogBox
	{
		FXDECLARE( MaterialBrowser )

	public:
		explicit MaterialBrowser( FXWindow *parent );
		~MaterialBrowser() override = default;

		const char *get_current() const;

		void create() override;

		enum
		{
			ID_MATERIAL_LIST = ID_LAST,
			ID_MATERIAL_GROUP_LIST,
			ID_MATERIAL_EDIT,
			ID_MATERIAL_APPLY,
			ID_MATERIAL_ICON_SIZE,
			ID_MATERIAL_FILTER,

			ID_MATERIAL_LAST,
		};

		long on_material_select( FXObject *, FXSelector, void * );
		long on_material_edit( FXObject *, FXSelector, void * );
		long on_material_group_select( FXObject *, FXSelector, void * );
		long on_material_icon_size( FXObject *, FXSelector, void * );
		long on_material_filter( FXObject *, FXSelector, void * );
		long on_material_apply( FXObject *, FXSelector, void * );

	protected:
		MaterialBrowser() = default;

	private:
		struct MaterialPreview
		{
			std::string path;
			std::string name;
			QmImage    *icon, *smallIcon;

			~MaterialPreview()
			{
				PlDestroyImage( icon );
				PlDestroyImage( smallIcon );
			}
		};
		std::map< std::string, std::vector< MaterialPreview * > > materialPreviewGroups;

		static void cache_preview_callback( const char *path, void *user );

		void update_material_list( const std::string &filter = "" );

		FXListBox  *materialGroupList;
		FXIconList *materialList;

		enum
		{
			VIEW_MODE_LIST,
			VIEW_MODE_ICONS,
			VIEW_MODE_BIG_ICONS,

			VIEW_MODE_COUNT,
		};
		FXToggleButton *viewModes[ VIEW_MODE_COUNT ];

		std::map< std::string, ApeMaterial * > materialMap;
	};
}// namespace forge

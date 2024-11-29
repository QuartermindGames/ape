
#pragma once

namespace forge
{
	class MaterialBrowser : public FXDialogBox
	{
		FXDECLARE( MaterialBrowser )

	public:
		explicit MaterialBrowser( FXWindow *parent );
		~MaterialBrowser() = default;

		const char *get_current() const;

		void create() override;

		enum
		{
			ID_MATERIAL_LIST = FXDialogBox::ID_LAST,
		};

		long on_material_select( FXObject *, FXSelector, void * );

	protected:
		MaterialBrowser() = default;

	private:
		struct MaterialPreview
		{
			std::string path;
			PLImage    *icon;

			inline ~MaterialPreview()
			{
				PlDestroyImage( icon );
			}
		};
		std::vector< MaterialPreview * > materialPreviews;

		static void cache_preview_callback( const char *path, void *user );

		FXIconList *materialList;

		std::map< std::string, ApeMaterial * > materialMap;
	};
}// namespace forge

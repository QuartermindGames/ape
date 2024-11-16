
#pragma once

namespace forge
{
	class MaterialBrowser : public FXDialogBox
	{
		FXDECLARE( MaterialBrowser )

	public:
		explicit MaterialBrowser( FXWindow *parent );
		inline ~MaterialBrowser() = default;

		ApeMaterial *get_current();

		void create() override;

	protected:
		inline MaterialBrowser() = default;

	private:
		FXIconList *materialList;

		std::map< std::string, ApeMaterial * > materialMap;
	};
}// namespace forge

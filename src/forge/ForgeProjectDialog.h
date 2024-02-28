// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "editor.h"

namespace ss::forge
{
	class ForgeProjectDialog : public FXDialogBox
	{
		FXDECLARE( ForgeProjectDialog )

	public:
		explicit ForgeProjectDialog( FXWindow *parent );
		~ForgeProjectDialog() = default;

		enum
		{
			ID_SELECT_PROJECT = FXDialogBox::ID_LAST,
		};
		long on_select_project( FXObject *, FXSelector, void * );
		long on_accept( FXObject *obj, FXSelector, void *ptr );

	protected:
		inline ForgeProjectDialog() = default;

	private:
		static constexpr const char *defaultName = "Enter a project name";
		static constexpr unsigned int baseWidth = 256;

		static void register_project_callback( const char *, void * );
		static std::map< std::string, forge::Project * > projects;

		FXListBox *listBox{};
		FXTextField *projectNameField{};
	};
}// namespace ss::forge

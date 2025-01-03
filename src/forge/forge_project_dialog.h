// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "forge.h"

namespace forge
{
	class ProjectDialog : public FXDialogBox
	{
		FXDECLARE( ProjectDialog )

	public:
		explicit ProjectDialog( FXWindow *parent );
		~ProjectDialog() = default;

		enum
		{
			ID_SELECT_PROJECT = FXDialogBox::ID_LAST,
		};
		long on_select_project( FXObject *, FXSelector, void * );
		long on_accept( FXObject *obj, FXSelector, void *ptr );

	protected:
		inline ProjectDialog() = default;

	private:
		static constexpr const char  *defaultName = "Enter a project name";
		static constexpr unsigned int baseWidth   = 256;

		static void                               register_project_callback( const char *, void                               *);
		static std::map< std::string, Project * > projects;

		FXListBox   *listBox{};
		FXTextField *projectNameField{};
	};
}// namespace forge

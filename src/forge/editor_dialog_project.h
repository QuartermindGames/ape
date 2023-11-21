// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

namespace os::editor
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
		long OnSelectProject( FXObject *, FXSelector, void * );
		long OnAccept( FXObject *, FXSelector, void * );

	protected:
		inline ProjectDialog() = default;

	private:
		static constexpr const char *defaultName = "Enter a project name";
		static constexpr unsigned int baseWidth  = 256;

		static void RegisterProjectCallback( const char *, void * );
		static std::map< std::string, editor::Project * > projects;

		FXListBox *listBox{};
		FXTextField *projectNameField{};
	};
}// namespace os::editor

// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include "editor.h"

namespace ss::forge
{
	class ForgeConsoleFrame : public FXVerticalFrame
	{
		FXDECLARE( ForgeConsoleFrame )

	public:
		explicit ForgeConsoleFrame( FXComposite *composite );
		~ForgeConsoleFrame() override;

		enum
		{
			ID_SUBMIT = FXVerticalFrame::ID_LAST,
			ID_SUBMIT_FIELD,
			ID_CLEAR,
		};

		void push_message( int level, const char *msg, const PLColour &colour );
		long submit_command( FXObject *, FXSelector, void * );
		long submit_key( FXObject *obj, FXSelector, void *ptr );

		long clear_command( FXObject *, FXSelector, void * );

	protected:
	private:
		inline ForgeConsoleFrame() = default;

		std::vector< std::string > _previousCommands;

		FXText *logField{};
		FXButton *submitButton{};
		FXComboBox *submitField{};
	};
}// namespace ss::forge

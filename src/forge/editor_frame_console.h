// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

namespace ss::forge
{
	class ConsoleFrame : public FXVerticalFrame
	{
		FXDECLARE( ConsoleFrame )

	public:
		explicit ConsoleFrame( FXComposite *composite );
		~ConsoleFrame() override;

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
		inline ConsoleFrame() = default;

		FXText *logField{};
		FXButton *submitButton{};
		FXTextField *submitField{};
	};
}// namespace ss::forge

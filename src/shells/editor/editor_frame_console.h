// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "editor.h"

namespace os::editor
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
		};

		void PushMessage( int level, const char *msg, const PLColour &colour );
        long SubmitCommand( FXObject *, FXSelector, void * );
		long SubmitKey( FXObject *, FXSelector, void * );

	protected:
	private:
		inline ConsoleFrame() = default;

		FXText *logField{};
		FXButton *submitButton{};
		FXTextField *submitField{};
	};
}// namespace os::editor

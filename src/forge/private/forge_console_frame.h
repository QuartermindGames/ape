// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include "forge.h"

namespace forge
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

		void push_message( int level, const char *msg, const QmMathColour4ub &colour );
		long submit_command( FXObject *, FXSelector, void * );
		long submit_key( FXObject *obj, FXSelector, void *ptr );

		long clear_command( FXObject *, FXSelector, void * );

	protected:
	private:
		inline ConsoleFrame() = default;

		std::vector< std::string > _previousCommands;

		FXText     *logField{};
		FXButton   *submitButton{};
		FXComboBox *submitField{};
	};
}// namespace forge

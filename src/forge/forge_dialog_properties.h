// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

namespace forge
{
	class PropertiesDialog : public FXDialogBox
	{
		FXDECLARE( PropertiesDialog )

	public:
		explicit PropertiesDialog( FXWindow *parent, ApeWorldNode *node );
		~PropertiesDialog() override = default;

		void set_node( ApeWorldNode *node );

	protected:
		PropertiesDialog() = default;

	private:
		void add_property( unsigned int row, const ApeWorldNodeProperty &property );

		ApeWorldNode *node{};
		FXTable      *table{};
	};
}// namespace forge

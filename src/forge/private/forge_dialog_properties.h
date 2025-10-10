// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

#include <list>

namespace forge
{
	class PropertiesDialog : public FXDialogBox
	{
		FXDECLARE( PropertiesDialog )

	public:
		explicit PropertiesDialog( FXWindow *parent, ApeWorldNode *node );
		~PropertiesDialog() override = default;

		void set_node( ApeWorldNode *node );

		enum
		{
			ID_TABLE = ID_LAST,
		};

		long on_table( FXObject *, FXSelector, void * );

	protected:
		PropertiesDialog() = default;

	private:
		enum class TablePropertyScope : uint8_t
		{
			GLOBAL,
			ENTITY,
		};

		struct TableProperty
		{
			const ApeEditorProperty *internal;
			void                    *ptr;
			TablePropertyScope       scope;
		};

		std::list< TableProperty > properties_;

		void add_property( unsigned int *row, const ApeEditorProperty *internalProperty, TablePropertyScope scope );

		ApeWorldNode *node{};
		FXTable      *table{};
	};
}// namespace forge

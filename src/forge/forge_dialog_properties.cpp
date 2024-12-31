// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "forge.h"
#include "forge_dialog_properties.h"

FXDEFMAP( forge::PropertiesDialog )
propertiesMap[] = {};

FXIMPLEMENT( forge::PropertiesDialog, FXDialogBox, propertiesMap, ARRAYNUMBER( propertiesMap ) )

forge::PropertiesDialog::PropertiesDialog( FXWindow *parent, ApeWorldNode *node )
    : FXDialogBox( parent, "Properties", DECOR_TITLE | DECOR_CLOSE | DECOR_BORDER | DECOR_RESIZE | DECOR_MENU )
{
	setWidth( 512 );
	setHeight( 512 );

	setPadLeft( 0 );
	setPadRight( 0 );
	setPadBottom( 0 );
	setPadTop( 0 );

	FXVerticalFrame *frame = new FXVerticalFrame( this, LAYOUT_FILL_X | LAYOUT_FILL_Y );
	frame->setPadLeft( 0 );
	frame->setPadRight( 0 );
	frame->setPadBottom( 0 );
	frame->setPadTop( 0 );

	table = new FXTable( frame, nullptr, 0, TABLE_COL_SIZABLE | TABLE_NO_ROWSELECT | LAYOUT_FILL_X | LAYOUT_FILL_Y );

	table->setDefRowHeight( 24 );

	FXHorizontalFrame *bottomFrame = new FXHorizontalFrame( frame, LAYOUT_FILL_X | FRAME_NONE );
	//bottomFrame->setPadLeft( 0 );
	//bottomFrame->setPadRight( 0 );
	//bottomFrame->setPadBottom( 0 );
	//bottomFrame->setPadTop( 0 );

	new FXButton( bottomFrame, "Cancel", nullptr, nullptr, 0, LAYOUT_RIGHT | FRAME_RAISED | FRAME_THICK );
	new FXButton( bottomFrame, "Apply", nullptr, nullptr, 0, LAYOUT_RIGHT | FRAME_RAISED | FRAME_THICK );

	set_node( node );
}

void forge::PropertiesDialog::set_node( ApeWorldNode *node )
{
	// check if it's already current,
	// so we don't populate all this over and over
	if ( this->node == node )
	{
		return;
	}

	this->node = node;

	table->clearItems();
	if ( this->node == nullptr )
	{
		return;
	}

	uint                        numBaseProperties;
	const ApeWorldNodeProperty *baseProperties = ape_world_node_get_properties( &numBaseProperties );
	assert( numBaseProperties > 0 );

	uint                        numProperties;
	const ApeWorldNodeProperty *properties = ape_world_node_get_class_properties( &numProperties, node->type );
	assert( numProperties > 0 );

	table->setTableSize( numBaseProperties + numProperties, 3 );

	table->setColumnWidth( 0, 100 );
	table->setColumnWidth( 1, 100 );
	table->setColumnWidth( 2, 400 );

	table->setColumnText( 0, "Property" );
	table->setColumnText( 1, "Value" );
	table->setColumnText( 2, "Description" );

	table->setColumnJustify( 1, FXHeaderItem::LEFT );

	uint row = 0;
	for ( uint i = 0; i < numBaseProperties; ++i, ++row )
	{
		add_property( row, baseProperties[ i ] );
	}

	for ( uint i = 0; i < numProperties; ++i, ++row )
	{
		add_property( row, properties[ i ] );
	}
}

void forge::PropertiesDialog::add_property( uint row, const ApeWorldNodeProperty &property )
{
	table->setItemText( row, 0, property.name );
	table->setItemText( row, 2, property.description );

	table->disableItem( row, 0 );
	table->disableItem( row, 2 );

	table->setItemJustify( row, 0, FXTableItem::RIGHT );
	table->setItemJustify( row, 1, FXTableItem::LEFT );
	table->setItemJustify( row, 2, FXTableItem::LEFT );

	//FXHorizontalFrame *inputFrame = new FXHorizontalFrame( table, LAYOUT_FILL_X | FRAME_NONE );

	void *value = ape_world_node_get_property_value( node, &property );
	table->setItemData( row, 1, value );

	const char *iconPath = nullptr;
	switch ( property.type )
	{
		default:
			assert( 0 );
			break;
		case APE_WORLD_NODE_PROPERTY_TYPE_FLOAT:
			iconPath = "resources/integer.gif";
			break;
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC2:
		{
			iconPath = "resources/transform.gif";

			const PLVector2 *vector = static_cast< const PLVector2 * >( value );

			char buf[ 128 ];
			snprintf( buf, sizeof( buf ), "%f %f", vector->x, vector->y );
			table->setItemText( row, 1, buf );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC3:
		{
			iconPath = "resources/transform.gif";

			const PLVector3 *vector = static_cast< const PLVector3 * >( value );

			char buf[ 128 ];
			snprintf( buf, sizeof( buf ), "%f %f %f", vector->x, vector->y, vector->z );
			table->setItemText( row, 1, buf );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC4:
		{
			iconPath = "resources/transform.gif";

			const PLVector4 *vector = static_cast< const PLVector4 * >( value );

			char buf[ 128 ];
			snprintf( buf, sizeof( buf ), "%f %f %f %f", vector->x, vector->y, vector->z, vector->w );
			table->setItemText( row, 1, buf );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_ENUM:
		{
			iconPath = "resources/list_details.png";

			FXString options;
			for ( unsigned int i = 0; i < property.enumType.numEnums; ++i )
			{
				options += property.enumType.enums[ i ].name;
				options += "\n";
			}

			table->setItem( row, 1, new FXComboTableItem( options ) );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_COLOUR: break;
		case APE_WORLD_NODE_PROPERTY_TYPE_INTEGER:
		{
			iconPath = "resources/integer.gif";

			const int *integer = static_cast< const int * >( value );

			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_STRING:
		{
			table->setItemText( row, 1, static_cast< const char * >( value ) );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_PATH: break;
		case APE_WORLD_NODE_PROPERTY_TYPE_BOOLEAN:
		{
			iconPath = "resources/boolean.png";
			table->setItem( row, 1, new FXComboTableItem( "False\nTrue\n" ) );
			break;
		}
	}

	if ( iconPath != nullptr )
	{
		table->setItemIcon( row, 0, load_fx_icon( getApp(), iconPath ) );
		table->setItemIconPosition( row, 0, FXTableItem::AFTER );
	}

	//table->setItem( row, 1, nullptr );
}

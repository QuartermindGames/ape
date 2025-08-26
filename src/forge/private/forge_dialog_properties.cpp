// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "forge.h"
#include "forge_dialog_properties.h"

FXDEFMAP( forge::PropertiesDialog )
propertiesMap[] = {
        FXMAPFUNC( SEL_CHANGED, forge::PropertiesDialog::ID_TABLE, forge::PropertiesDialog::on_table ),
        FXMAPFUNC( SEL_REPLACED, forge::PropertiesDialog::ID_TABLE, forge::PropertiesDialog::on_table ),
        FXMAPFUNC( SEL_INSERTED, forge::PropertiesDialog::ID_TABLE, forge::PropertiesDialog::on_table ),
};

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

	table = new FXTable( frame, this, ID_TABLE, TABLE_COL_SIZABLE | TABLE_NO_COLSELECT | TABLE_NO_ROWSELECT | LAYOUT_FILL_X | LAYOUT_FILL_Y );

	table->setDefRowHeight( 24 );

	FXHorizontalFrame *bottomFrame = new FXHorizontalFrame( frame, LAYOUT_FILL_X | FRAME_NONE );
	//bottomFrame->setPadLeft( 0 );
	//bottomFrame->setPadRight( 0 );
	//bottomFrame->setPadBottom( 0 );
	//bottomFrame->setPadTop( 0 );

	new FXButton( bottomFrame, "Close", nullptr, this, ID_CANCEL, LAYOUT_RIGHT | FRAME_RAISED | FRAME_THICK );
	//new FXButton( bottomFrame, "Apply", nullptr, this, ID_ACCEPT, LAYOUT_RIGHT | FRAME_RAISED | FRAME_THICK );

	set_node( node );
}

static unsigned int get_sub_rows_for_properties( const ApeWorldNodeProperty *properties, unsigned int numProperties )
{
	unsigned int numRows = 0;
	for ( unsigned int i = 0; i < numProperties; ++i )
	{
		switch ( properties[ i ].type )
		{
			default:
				break;
			case APE_WORLD_NODE_PROPERTY_TYPE_VEC2:
				numRows++;
				break;
			case APE_WORLD_NODE_PROPERTY_TYPE_VEC3:
				numRows += 2;
				break;
			case APE_WORLD_NODE_PROPERTY_TYPE_COLOUR:
			case APE_WORLD_NODE_PROPERTY_TYPE_VEC4:
				numRows += 3;
				break;
		}
	}

	return numRows;
}

void forge::PropertiesDialog::set_node( ApeWorldNode *node )
{
	// check if it's already current,
	// so we don't populate all this over and over
	if ( this->node == node )
	{
		return;
	}

	table->clearItems();

	this->node = node;
	if ( this->node == nullptr )
	{
		return;
	}

	unsigned int                numBaseProperties;
	const ApeWorldNodeProperty *baseProperties = ape_world_node_get_properties( &numBaseProperties );
	assert( numBaseProperties > 0 );

	unsigned int                numProperties;
	const ApeWorldNodeProperty *properties = ape_world_node_get_class_properties( &numProperties, node->type );

	unsigned int numRows = numBaseProperties + numProperties;
	numRows += get_sub_rows_for_properties( baseProperties, numBaseProperties );
	numRows += get_sub_rows_for_properties( properties, numProperties );

	table->setTableSize( numRows, 3 );

	table->setColumnWidth( 0, 100 );
	table->setColumnWidth( 1, 100 );
	table->setColumnWidth( 2, 400 );

	table->setColumnText( 0, "Property" );
	table->setColumnText( 1, "Value" );
	table->setColumnText( 2, "Description" );

	table->setColumnJustify( 1, FXHeaderItem::LEFT );

	unsigned int row = 0;
	for ( unsigned int i = 0; i < numBaseProperties; ++i, ++row )
	{
		add_property( &row, &baseProperties[ i ] );
	}

	for ( unsigned int i = 0; i < numProperties; ++i, ++row )
	{
		add_property( &row, &properties[ i ] );
	}
}

long forge::PropertiesDialog::on_table( FXObject *, FXSelector, void *ptr )
{
	FXTablePos  *pos  = ( FXTablePos * ) ptr;
	FXTableItem *item = table->getItem( pos->row, pos->col );
	if ( item == nullptr || item->getText().empty() )
	{
		return false;
	}

	const ApeWorldNodeProperty *property = ( const ApeWorldNodeProperty * ) item->getData();
	if ( property == nullptr )
	{
		return false;
	}

	ptr = ape_world_node_get_property_pointer( node, property );
	assert( ptr != nullptr );

	// we have to do this dumb shit, otherwise trying to use the item string directly doesn't work
	FXString string = item->getText();

	const char *text = string.text();
	switch ( property->type )
	{
		default:
		case APE_WORLD_NODE_PROPERTY_TYPE_INVALID:
			forge_warning_( "Unhandled property type (%u)!\n", property->type );
			break;
		case APE_WORLD_NODE_PROPERTY_TYPE_FLOAT:
			*( float * ) ptr = strtof( text, nullptr );
			break;
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC2:
		{
			for ( unsigned int i = 0; i < 2; ++i )
			{
				item                     = table->getItem( pos->row + i, pos->col );
				string                   = item->getText();
				*( ( float * ) ptr + i ) = strtof( string.text(), nullptr );
			}
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC3:
		{
			for ( unsigned int i = 0; i < 3; ++i )
			{
				item                     = table->getItem( pos->row + i, pos->col );
				string                   = item->getText();
				*( ( float * ) ptr + i ) = strtof( string.text(), nullptr );
			}
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_COLOUR:
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC4:
		{
			for ( unsigned int i = 0; i < 4; ++i )
			{
				item                     = table->getItem( pos->row + i, pos->col );
				string                   = item->getText();
				*( ( float * ) ptr + i ) = strtof( string.text(), nullptr );
			}
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_ENUM:
		{
			for ( unsigned int i = 0; i < property->enumType.numEnums; ++i )
			{
				if ( strcmp( text, property->enumType.enums[ i ].name ) != 0 )
				{
					continue;
				}

				*( uint32_t * ) ptr = property->enumType.enums[ i ].value;
			}
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_INTEGER:
			*( ( int32_t * ) ptr ) = strtol( text, nullptr, 10 );
			break;
		case APE_WORLD_NODE_PROPERTY_TYPE_STRING: break;
		case APE_WORLD_NODE_PROPERTY_TYPE_PATH: break;
		case APE_WORLD_NODE_PROPERTY_TYPE_BOOLEAN:
			if ( strcmp( text, "True" ) == 0 )
			{
				*( bool * ) ptr = true;
			}
			else
			{
				*( bool * ) ptr = false;
			}
			break;
	}

	// need to set these again, so transform is updated (because of this dumb idiot)
	ape_world_node_set_position( node, &node->position );
	ape_world_node_set_angles( node, &node->angles );

	return true;
}

void forge::PropertiesDialog::add_property( unsigned int *row, const ApeWorldNodeProperty *property )
{
	//FXHorizontalFrame *inputFrame = new FXHorizontalFrame( table, LAYOUT_FILL_X | FRAME_NONE );

	void *value = ape_world_node_get_property_pointer( node, property );
	table->setItemData( *row, 1, ( void * ) property );

	unsigned int numSubRows = 0;
	switch ( property->type )
	{
		default:
			assert( 0 );
			break;
		case APE_WORLD_NODE_PROPERTY_TYPE_FLOAT:
		{
			char buf[ 128 ];
			snprintf( buf, sizeof( buf ), "%f", *static_cast< const float * >( value ) );
			table->setItemText( *row, 1, buf );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC2:
		{
			const QmMathVector2f *vector = static_cast< const QmMathVector2f * >( value );
			table->setItemText( *row, 1, std::to_string( vector->x ).c_str() );
			table->setItemText( *row + 1, 1, std::to_string( vector->y ).c_str() );
			numSubRows++;
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC3:
		{
			const QmMathVector3f *vector = static_cast< const QmMathVector3f * >( value );
			table->setItemText( *row, 1, std::to_string( vector->x ).c_str() );
			table->setItemText( *row + 1, 1, std::to_string( vector->y ).c_str() );
			table->setItemText( *row + 2, 1, std::to_string( vector->z ).c_str() );
			numSubRows += 2;
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_VEC4:
		{
			const QmMathVector4f *vector = static_cast< const QmMathVector4f * >( value );
			table->setItemText( *row, 1, std::to_string( vector->x ).c_str() );
			table->setItemText( *row + 1, 1, std::to_string( vector->y ).c_str() );
			table->setItemText( *row + 2, 1, std::to_string( vector->z ).c_str() );
			table->setItemText( *row + 3, 1, std::to_string( vector->w ).c_str() );
			numSubRows += 3;
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_ENUM:
		{
			FXString options;
			for ( unsigned int i = 0; i < property->enumType.numEnums; ++i )
			{
				options += property->enumType.enums[ i ].name;
				options += "\n";
			}

			table->setItem( *row, 1, new FXComboTableItem( options, nullptr, ( void * ) property ) );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_COLOUR:
		{
			const QmMathColour4f *colour = ( QmMathColour4f * ) value;
			table->setItemText( *row, 1, std::to_string( colour->r ).c_str() );
			table->setItemText( *row + 1, 1, std::to_string( colour->g ).c_str() );
			table->setItemText( *row + 2, 1, std::to_string( colour->b ).c_str() );
			table->setItemText( *row + 3, 1, std::to_string( colour->a ).c_str() );
			numSubRows += 3;
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_INTEGER:
		{
			char buf[ 128 ];
			snprintf( buf, sizeof( buf ), "%d", *static_cast< const int * >( value ) );
			table->setItemText( *row, 1, buf );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_STRING:
		{
			table->setItemText( *row, 1, static_cast< const char * >( value ) );
			break;
		}
		case APE_WORLD_NODE_PROPERTY_TYPE_PATH: break;
		case APE_WORLD_NODE_PROPERTY_TYPE_BOOLEAN:
		{
			table->setItem( *row, 1, new FXComboTableItem( "False\nTrue\n", nullptr, ( void * ) property ) );
			break;
		}
	}

	static const char *icons[ APE_WORLD_NODE_MAX_PROPERTY_TYPES ] = {
	        nullptr,                     // invalid
	        "resources/integer.gif",     // float
	        "resources/transform.gif",   // vec2
	        "resources/transform.gif",   // vec3
	        "resources/transform.gif",   // vec4
	        "resources/list_details.png",// enum
	        "resources/colours.gif",     // colour
	        "resources/integer.gif",     // integer
	        "resources/string.gif",      // string
	        "resources/path.gif",        // path
	        "resources/boolean.png",     // boolean
	};

	if ( icons[ property->type ] != nullptr )
	{
		table->setItemIcon( *row, 0, load_fx_icon( getApp(), icons[ property->type ] ) );
		table->setItemIconPosition( *row, 0, FXTableItem::AFTER );
	}


	table->setItemText( *row, 0, property->name );
	table->setItemText( *row, 2, property->description );

	for ( unsigned int i = 0; i < numSubRows + 1; ++i )
	{
		table->disableItem( *row + i, 0 );
		table->disableItem( *row + i, 2 );

		table->setItemJustify( *row + i, 0, FXTableItem::RIGHT );
		table->setItemJustify( *row + i, 1, FXTableItem::LEFT );
		table->setItemJustify( *row + i, 2, FXTableItem::LEFT );
	}

	*row += numSubRows;
}

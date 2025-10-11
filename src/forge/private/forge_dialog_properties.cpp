// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

#include "forge.h"
#include "forge_dialog_properties.h"

#include "yin/core_entity.h"

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
	new FXButton( bottomFrame, "Close", nullptr, this, ID_CANCEL, LAYOUT_RIGHT | FRAME_RAISED | FRAME_THICK );
	//new FXButton( bottomFrame, "Apply", nullptr, this, ID_ACCEPT, LAYOUT_RIGHT | FRAME_RAISED | FRAME_THICK );

	set_node( node );
}

static unsigned int get_sub_rows_for_properties( const ApeEditorProperty *properties, const unsigned int numProperties )
{
	unsigned int numSubRows = 0;
	for ( unsigned int i = 0; i < numProperties; ++i )
	{
		switch ( properties[ i ].type )
		{
			default:
				break;
			case APE_EDITOR_PROPERTY_TYPE_VEC2:
				numSubRows++;
				break;
			case APE_EDITOR_PROPERTY_TYPE_VEC3:
				numSubRows += 2;
				break;
			case APE_EDITOR_PROPERTY_TYPE_COLOUR:
			case APE_EDITOR_PROPERTY_TYPE_VEC4:
				numSubRows += 3;
				break;
		}
	}

	return numSubRows;
}

void forge::PropertiesDialog::set_node( ApeWorldNode *node )
{
	// check if it's already current,
	// so we don't populate all this over and over
	if ( this->node == node )
	{
		return;
	}

	properties_.clear();

	table->clearItems();

	this->node = node;
	if ( this->node == nullptr )
	{
		return;
	}

	unsigned int             numBaseProperties;
	const ApeEditorProperty *baseProperties = ape_world_node_get_properties( &numBaseProperties );
	assert( numBaseProperties > 0 );

	unsigned int             numClassProperties;
	const ApeEditorProperty *classProperties = ape_world_node_get_class_properties( &numClassProperties, node->type );

	unsigned int             numEntityProperties = 0;
	const ApeEditorProperty *entityProperties    = nullptr;
	if ( node->type == APE_WORLD_NODE_TYPE_ENTITY )
	{
		ApeEntity *entity   = ( ApeEntity * ) node;
		numEntityProperties = entity->classDefinition->numProperties;
		entityProperties    = entity->classDefinition->properties;
	}

	unsigned int numRows = 0;
	numRows += get_sub_rows_for_properties( baseProperties, numBaseProperties ) + numBaseProperties;
	numRows += get_sub_rows_for_properties( classProperties, numClassProperties ) + numClassProperties;
	numRows += get_sub_rows_for_properties( entityProperties, numEntityProperties ) + numEntityProperties;

	table->setTableSize( numRows, 3 );

	table->setColumnWidth( 0, 100 );
	table->setColumnWidth( 1, 100 );
	table->setColumnWidth( 2, 400 );

	table->setColumnText( 0, "Property" );
	table->setColumnText( 1, "Value" );
	table->setColumnText( 2, "Description" );

	table->setColumnJustify( 1, FXHeaderItem::LEFT );

	properties_.resize( numRows );

	unsigned int row = 0;
	for ( unsigned int i = 0; i < numBaseProperties; ++i, ++row )
	{
		add_property( &row, &baseProperties[ i ], TablePropertyScope::GLOBAL );
	}
	for ( unsigned int i = 0; i < numClassProperties; ++i, ++row )
	{
		add_property( &row, &classProperties[ i ], TablePropertyScope::GLOBAL );
	}
	for ( unsigned int i = 0; i < numEntityProperties; ++i, ++row )
	{
		add_property( &row, &entityProperties[ i ], TablePropertyScope::ENTITY );
	}
}

long forge::PropertiesDialog::on_table( FXObject *, FXSelector, void *ptr )
{
	FXTablePos *pos = static_cast< FXTablePos * >( ptr );
	assert( pos != nullptr );

	FXTableItem *item = table->getItem( pos->row, pos->col );
	assert( item != nullptr );

	const TableProperty *tableProperty = ( const TableProperty * ) item->getData();
	if ( tableProperty == nullptr )
	{
		return false;
	}

	const ApeEditorProperty *editorProperty = tableProperty->internal;
	assert( editorProperty != nullptr );

	// we have to do this dumb shit, otherwise trying to use the item string directly doesn't work
	FXString string = item->getText();

	const char *text = string.text();
	switch ( editorProperty->type )
	{
		default:
		case APE_EDITOR_PROPERTY_TYPE_INVALID:
			forge_warning_( "Unhandled property type (%u)!\n", editorProperty->type );
			break;
		case APE_EDITOR_PROPERTY_TYPE_FLOAT:
			*( float * ) tableProperty->ptr = strtof( text, nullptr );
			break;
		case APE_EDITOR_PROPERTY_TYPE_VEC2:
		{
			for ( unsigned int i = 0; i < 2; ++i )
			{
				item                                    = table->getItem( pos->row + i, pos->col );
				string                                  = item->getText();
				*( ( float * ) tableProperty->ptr + i ) = strtof( string.text(), nullptr );
			}
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_VEC3:
		{
			for ( unsigned int i = 0; i < 3; ++i )
			{
				item                                    = table->getItem( pos->row + i, pos->col );
				string                                  = item->getText();
				*( ( float * ) tableProperty->ptr + i ) = strtof( string.text(), nullptr );
			}
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_COLOUR:
		case APE_EDITOR_PROPERTY_TYPE_VEC4:
		{
			for ( unsigned int i = 0; i < 4; ++i )
			{
				item                                    = table->getItem( pos->row + i, pos->col );
				string                                  = item->getText();
				*( ( float * ) tableProperty->ptr + i ) = strtof( string.text(), nullptr );
			}
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_ENUM:
		{
			for ( unsigned int i = 0; i < editorProperty->enumType.numEnums; ++i )
			{
				if ( strcmp( text, editorProperty->enumType.enums[ i ].name ) != 0 )
				{
					continue;
				}

				*( uint32_t * ) tableProperty->ptr = editorProperty->enumType.enums[ i ].value;
			}
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_INTEGER:
			*( ( int32_t * ) tableProperty->ptr ) = strtol( text, nullptr, 10 );
			break;
		case APE_EDITOR_PROPERTY_TYPE_STRING:
		case APE_EDITOR_PROPERTY_TYPE_PATH:
			//TODO: path should work differently, eventually... probably when we migrate to gtk
			snprintf( ( char * ) tableProperty->ptr, editorProperty->stringType.maxSize, "%s", text );
			break;
		case APE_EDITOR_PROPERTY_TYPE_BOOLEAN:
			if ( strcmp( text, "True" ) == 0 )
			{
				*( bool * ) tableProperty->ptr = true;
			}
			else
			{
				*( bool * ) tableProperty->ptr = false;
			}
			break;
	}

	// need to set these again, so transform is updated (because of this dumb idiot)
	ape_world_node_set_position( node, &node->position );
	ape_world_node_set_angles( node, &node->angles );

	return true;
}

void forge::PropertiesDialog::add_property( unsigned int *row, const ApeEditorProperty *internalProperty, TablePropertyScope scope )
{
	TableProperty property = {};
	property.internal      = internalProperty;
	property.scope         = scope;

	if ( scope == TablePropertyScope::ENTITY )
	{
		// for an entity, the offset is relative to the class data address
		property.ptr = ( char * ) ( ( ApeEntity * ) node )->classData + internalProperty->offset;
	}
	else
	{
		// but for any other type of node, the offset is relative to the node address
		property.ptr = ape_world_node_get_property_pointer( node, internalProperty );
	}

	assert( property.ptr != nullptr );

	unsigned int numSubRows = 0;
	switch ( internalProperty->type )
	{
		default:
			assert( 0 );
			break;
		case APE_EDITOR_PROPERTY_TYPE_FLOAT:
		{
			char buf[ 128 ];
			snprintf( buf, sizeof( buf ), "%f", *static_cast< const float * >( property.ptr ) );
			table->setItemText( *row, 1, buf );
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_VEC2:
		{
			const QmMathVector2f *vector = static_cast< const QmMathVector2f * >( property.ptr );
			table->setItemText( *row, 1, std::to_string( vector->x ).c_str() );
			table->setItemText( *row + 1, 1, std::to_string( vector->y ).c_str() );
			numSubRows++;
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_VEC3:
		{
			const QmMathVector3f *vector = static_cast< const QmMathVector3f * >( property.ptr );
			table->setItemText( *row, 1, std::to_string( vector->x ).c_str() );
			table->setItemText( *row + 1, 1, std::to_string( vector->y ).c_str() );
			table->setItemText( *row + 2, 1, std::to_string( vector->z ).c_str() );
			numSubRows += 2;
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_VEC4:
		{
			const QmMathVector4f *vector = static_cast< const QmMathVector4f * >( property.ptr );
			table->setItemText( *row, 1, std::to_string( vector->x ).c_str() );
			table->setItemText( *row + 1, 1, std::to_string( vector->y ).c_str() );
			table->setItemText( *row + 2, 1, std::to_string( vector->z ).c_str() );
			table->setItemText( *row + 3, 1, std::to_string( vector->w ).c_str() );
			numSubRows += 3;
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_ENUM:
		{
			FXString options;
			for ( unsigned int i = 0; i < internalProperty->enumType.numEnums; ++i )
			{
				options += internalProperty->enumType.enums[ i ].name;
				options += "\n";
			}

			const ApeEnumProperty *enumProperty = ( ApeEnumProperty * ) property.ptr;
			table->setItem( *row, 1, new FXComboTableItem( options, nullptr, ( void * ) internalProperty ) );
			table->setItemText( *row, 1, internalProperty->enumType.enums[ *enumProperty ].name );
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_COLOUR:
		{
			const QmMathColour4f *colour = ( QmMathColour4f * ) property.ptr;
			table->setItemText( *row, 1, std::to_string( colour->r ).c_str() );
			table->setItemText( *row + 1, 1, std::to_string( colour->g ).c_str() );
			table->setItemText( *row + 2, 1, std::to_string( colour->b ).c_str() );
			table->setItemText( *row + 3, 1, std::to_string( colour->a ).c_str() );
			numSubRows += 3;
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_INTEGER:
		{
			char buf[ 128 ];
			snprintf( buf, sizeof( buf ), "%d", *static_cast< const int * >( property.ptr ) );
			table->setItemText( *row, 1, buf );
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_STRING:
		{
			table->setItemText( *row, 1, static_cast< const char * >( property.ptr ) );
			break;
		}
		case APE_EDITOR_PROPERTY_TYPE_PATH: break;
		case APE_EDITOR_PROPERTY_TYPE_BOOLEAN:
		{
			table->setItem( *row, 1, new FXComboTableItem( "False\nTrue\n", nullptr, ( void * ) internalProperty ) );
			break;
		}
	}

	static const char *icons[ APE_EDITOR_MAX_PROPERTY_TYPES ] = {
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

	if ( icons[ internalProperty->type ] != nullptr )
	{
		table->setItemIcon( *row, 0, load_fx_icon( getApp(), icons[ internalProperty->type ] ) );
		table->setItemIconPosition( *row, 0, FXTableItem::AFTER );
	}

	properties_.emplace_back( property );
	table->setItemData( *row, 1, &properties_.back() );

	table->setItemText( *row, 0, internalProperty->name );
	table->setItemText( *row, 2, internalProperty->description );

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

/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>
#include <PL/pl_parse.h>

#include "common/common.h"
#include "NodePrivate.h"

static NLPropertyType PropertyTypeForString( const char *type ) {
	if ( pl_strcasecmp( type, "integer" ) == 0 )
		return NODE_PROPERTY_INTEGER;
	else if ( pl_strcasecmp( type, "float" ) == 0 )
		return NODE_PROPERTY_FLOAT;
	else if ( pl_strcasecmp( type, "string" ) == 0 )
		return NODE_PROPERTY_STRING;
	else if ( pl_strcasecmp( type, "bool" ) == 0 )
		return NODE_PROPERTY_BOOLEAN;
	else if ( pl_strcasecmp( type, "object" ) == 0 )
		return NODE_PROPERTY_OBJECT;
	else if ( pl_strcasecmp( type, "array" ) == 0 )
		return NODE_PROPERTY_ARRAY;

	return NODE_PROPERTY_INVALID;
}

static NLNode *ParseNode( NLNode *parent, const char *buf, size_t length ) {
	char type[ 64 ];
	if ( plParseToken( &buf, type, sizeof( type ) ) == NULL ) {
		return NULL;
	}

	char name[ 64 ];
	if ( plParseToken( &buf, name, sizeof( name ) ) == NULL ) {
		return NULL;
	}

	/* figure out what data type it is and read in it's result */
	bool status = false;
	NLPropertyType propertyType = PropertyTypeForString( type );
	switch ( propertyType ) {
		case NODE_PROPERTY_INTEGER: {
			int i = plParseInteger( &buf, &status );
			return NL_PushBackInt( parent, name, i );
		}
		case NODE_PROPERTY_FLOAT: {
			float i = plParseFloat( &buf, &status );
			return NL_PushBackFloat( parent, name, i );
		}
		case NODE_PROPERTY_STRING: {
			char i[ 256 ];
			plParseToken( &buf, i, sizeof( i ) );
			return NL_PushBackString( parent, name, i );
		}
		case NODE_PROPERTY_BOOLEAN: {
			char i[ 8 ];
			plParseToken( &buf, i, sizeof( i ) );
			return NL_PushBackBool( parent, name, ( pl_strcasecmp( i, "true" ) == 0 || i[ 0 ] == '1' ) );
		}
		case NODE_PROPERTY_OBJECT: {

		}

		default:
			Warning( "Unknown property type, \"%s\"!\n", type );
			break;
	}

	if ( !status ) {
		Warning( "Failed to parse property for \"%s\"!\n", name );
	}
}

NLNode *NL_ParseBuffer( const char *buf, size_t length ) {
	/*
	const char *end = buf + length;
	while( buf < end && *buf != '\0' ) {

	}
*/
	return ParseNode( NULL, buf, length );
}

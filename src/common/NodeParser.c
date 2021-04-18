/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>
#include <PL/pl_parse.h>

#include "common/Common.h"
#include "NodePrivate.h"

static void SkipToNextToken( const char **buf, unsigned int *line ) {
	while ( plIsEndOfLine( buf ) ) {
		plSkipLine( buf );
		*line++;
	}

    plSkipWhitespace( buf );
}

static const char *ParseToken( const char **buf, char *token, size_t size, unsigned int *line ) {
	SkipToNextToken( buf, line );
	return plParseToken( buf, token, size );
}

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

static NLNode *ParseObjectNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine );
static NLNode *ParseArrayNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine ) {
	char childType[ NL_MAX_TYPE_LENGTH ];
	if ( ParseToken( buf, childType, sizeof( childType ), &currentLine ) == NULL ) {
		Warning( "Failed to parse child type for array!\n" );
		return NULL;
	}

	char name[ NL_MAX_NAME_LENGTH ];
	if ( ParseToken( buf, name, sizeof( name ), &currentLine ) == NULL ) {
		Warning( "Failed to parse name!\n" );
		return NULL;
	}

	plSkipWhitespace( buf );
	if ( *( *buf ) != '{' ) {
		Warning( "No opening brace for array, \"%s\"!\n", name );
		return NULL;
	}
	( *buf )++;

	NLNode *arrayNode = xNL_PushBackNode( parent, name, NODE_PROPERTY_ARRAY );
	if ( arrayNode == NULL ) {
		return NULL;
	}

	arrayNode->childType = PropertyTypeForString( childType );
	switch ( arrayNode->childType ) {
		default:
			Warning( "Invalid child type for array, \"%s\"!\n", name );
			break;
		case NODE_PROPERTY_INTEGER: {
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				bool status;
				int i = plParseInteger( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse integer for array, \"%s\"!\n", name );
					break;
				}
				NL_PushBackInt( arrayNode, NULL, i );
				plSkipWhitespace( buf );
			}
			break;
		}
		case NODE_PROPERTY_FLOAT: {
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				bool status;
				float i = plParseFloat( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse integer for array, \"%s\"!\n", name );
					break;
				}
				NL_PushBackFloat( arrayNode, NULL, i );
				plSkipWhitespace( buf );
			}
			break;
		}
		case NODE_PROPERTY_OBJECT: {
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				if ( ParseObjectNode( arrayNode, buf, length, 0 ) == NULL ) {
					Warning( "Failed to parse object node for array, \"%s\"!\n", name );
					break;
				}
				plSkipWhitespace( buf );
			}
			break;
		}
		case NODE_PROPERTY_BOOLEAN: {
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				bool status;
				int i = plParseInteger( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse integer for array, \"%s\"!\n", name );
					break;
				}
				NL_PushBackInt( arrayNode, NULL, i );
				plSkipWhitespace( buf );
			}
			break;
		}
		case NODE_PROPERTY_STRING: {
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				char i[ NL_MAX_STRING_LENGTH ];
				if ( plParseEnclosedString( buf, i, sizeof( i ) ) == NULL ) {
					Warning( "Failed to parse enclosed string for array, \"%s\"!\n", name );
					break;
				}
				NL_PushBackString( arrayNode, NULL, i );
				plSkipWhitespace( buf );
			}
			break;
		}
		case NODE_PROPERTY_LINK: {
			assert( 0 );
			break;
		}
	}

	if ( *( *buf ) != '}' ) {
		Warning( "No closing brace for array, \"%s\"!\n", name );
		return arrayNode;
	}
	( *buf )++;

	return arrayNode;
}

static NLNode *ParseNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine );
static NLNode *ParseObjectNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine ) {
	char name[ NL_MAX_NAME_LENGTH ];
	if ( ParseToken( buf, name, sizeof( name ), &currentLine ) == NULL ) {
		Warning( "Failed to parse name!\n" );
		return NULL;
	}

	/* make sure the object is followed by an opening brace */
	plSkipWhitespace( buf );
	if ( *( *buf ) != '{' ) {
		Warning( "No opening brace for object, \"%s\"!\n", name );
		return NULL;
	}
	( *buf )++;

	NLNode *objectNode = NL_PushBackObj( parent, name );
	if ( objectNode == NULL ) {
		return NULL;
	}

	SkipToNextToken( buf, &currentLine );

	/* read in all the children nodes */
	while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
		if ( ParseNode( objectNode, buf, length, 0 ) == NULL ) {
			Warning( "Failed to parse child node for object, \"%s\" [%d]!\n", name, currentLine );
			break;
		}
		SkipToNextToken( buf, &currentLine );
	}

	if ( *( *buf ) != '}' ) {
		Warning( "No closing brace for object, \"%s\"!\n", name );
		return objectNode;
	}
	( *buf )++;

	return objectNode;
}

static NLNode *ParseNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine ) {
	/* now try reading in the type */
	char type[ NL_MAX_TYPE_LENGTH ];
	if ( ParseToken( buf, type, sizeof( type ), &currentLine ) == NULL ) {
		return NULL;
	}

	NLPropertyType propertyType = PropertyTypeForString( type );
	/* an array is a special case, parsing-wise */
	if ( propertyType == NODE_PROPERTY_ARRAY ) {
		return ParseArrayNode( parent, buf, length, currentLine );
	} else if ( propertyType == NODE_PROPERTY_OBJECT ) {
		return ParseObjectNode( parent, buf, length, currentLine );
	} else {
		char name[ NL_MAX_NAME_LENGTH ];
		if ( ParseToken( buf, name, sizeof( name ), &currentLine ) == NULL ) {
			Warning( "Failed to parse name [%d]!\n", currentLine );
			return NULL;
		}

		/* figure out what data type it is and read in it's result */
		switch ( propertyType ) {
			case NODE_PROPERTY_INTEGER: {
				bool status;
				int i = plParseInteger( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse integer, \"%s\" [%d]!\n", name, currentLine );
					return NULL;
				}
				return NL_PushBackInt( parent, name, i );
			}
			case NODE_PROPERTY_FLOAT: {
				bool status;
				float i = plParseFloat( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse float, \"%s\" [%d]!\n", name, currentLine );
					return NULL;
				}
				return NL_PushBackFloat( parent, name, i );
			}
			case NODE_PROPERTY_STRING: {
				char i[ NL_MAX_STRING_LENGTH ];
				if ( plParseEnclosedString( buf, i, sizeof( i ) ) == NULL ) {
					Warning( "Failed to parse string, \"%s\" [%d]!\n", name, currentLine );
					return NULL;
				}
				return NL_PushBackString( parent, name, i );
			}
			case NODE_PROPERTY_BOOLEAN: {
				char i[ NL_MAX_BOOL_LENGTH ];
				if ( ParseToken( buf, i, sizeof( i ), &currentLine ) == NULL ) {
					Warning( "Failed to parse boolean, \"%s\" [%d]!\n", name, currentLine );
					return NULL;
				}
				return NL_PushBackBool( parent, name, ( pl_strcasecmp( i, "true" ) == 0 || i[ 0 ] == '1' ) );
			}
			default:
				Warning( "Unknown property type, \"%s\" [%d]!\n", type, currentLine );
				break;
		}
	}

	return NULL;
}

NLNode *NL_ParseBuffer( const char *buf, size_t length ) {
	return ParseNode( NULL, &buf, length, 1 );
}

/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include <plcore/pl_linkedlist.h>
#include <plcore/pl_parse.h>

#include "CFWNodePrivate.h"

#define DEBUG_PARSER 0
#if !defined( NDEBUG ) && DEBUG_PARSER
#define DebugParser( FORMAT, ... ) Message( "PARSE: " FORMAT, ##__VA_ARGS__ )
#else
#define DebugParser( FORMAT, ... )
#endif

static void SkipToNextToken( const char **buf, unsigned int *line ) {
	while ( *( *buf ) == ' ' || *( *buf ) == '\t' || *( *buf ) == '\n' || *( *buf ) == '\r' ) {
		if ( *( *buf ) == '\n' ) {
			*line++;
		}

		( *buf )++;
	}

	DebugParser( "POS: %s\n", buf[ 0 ] );
}

static const char *ParseToken( const char **buf, char *token, size_t size, unsigned int *line ) {
	SkipToNextToken( buf, line );
	return PlParseToken( buf, token, size );
}

static NLPropertyType PropertyTypeForString( const char *type ) {
	if ( pl_strcasecmp( type, "string" ) == 0 )
		return NL_PROP_STR;
	if ( pl_strcasecmp( type, "bool" ) == 0 )
		return NL_PROP_BOOL;
	if ( pl_strcasecmp( type, "object" ) == 0 )
		return NL_PROP_OBJ;
	if ( pl_strcasecmp( type, "array" ) == 0 )
		return NL_PROP_ARRAY;

	if ( pl_strcasecmp( type, "uint8" ) == 0 )
		return NL_PROP_UI8;
	if ( pl_strcasecmp( type, "uint32" ) == 0 )
		return NL_PROP_UI32;
	if ( pl_strcasecmp( type, "uint64" ) == 0 )
		return NL_PROP_UI64;
	if ( pl_strcasecmp( type, "int8" ) == 0 || pl_strcasecmp( type, "bool" ) == 0 )
		return NL_PROP_I8;
	if ( pl_strcasecmp( type, "integer" ) == 0 || pl_strcasecmp( type, "int32" ) == 0 )
		return NL_PROP_I32;
	if ( pl_strcasecmp( type, "int64" ) == 0 )
		return NL_PROP_I64;
	if ( pl_strcasecmp( type, "float" ) == 0 )
		return NL_PROP_F32;
	if ( pl_strcasecmp( type, "float64" ) == 0 )
		return NL_PROP_F64;

	return NL_PROP_UNDEFINED;
}

static NLNode *ParseObjectNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine );
static NLNode *ParseArrayNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine ) {
	DebugParser( "Entering ParseArrayNode\n" );

	char childType[ NL_MAX_TYPE_LENGTH ];
	if ( ParseToken( buf, childType, sizeof( childType ), &currentLine ) == NULL ) {
		Warning( "Failed to parse child type for array!\n" );
		return NULL;
	}
	DebugParser( "childType( %s )\n", childType );

	char name[ NL_MAX_NAME_LENGTH ];
	if ( ParseToken( buf, name, sizeof( name ), &currentLine ) == NULL ) {
		Warning( "Failed to parse name!\n" );
		return NULL;
	}
	DebugParser( "name( %s )\n", name );

	SkipToNextToken( buf, &currentLine );
	if ( *( *buf ) != '{' ) {
		Warning( "No opening brace for array, \"%s\"!\n", name );
		return NULL;
	}
	( *buf )++;

	NLNode *arrayNode = xNL_PushBackNode( parent, name, NL_PROP_ARRAY );
	if ( arrayNode == NULL ) {
		return NULL;
	}

	SkipToNextToken( buf, &currentLine );

	arrayNode->childType = PropertyTypeForString( childType );
	switch ( arrayNode->childType ) {
		default:
			Warning( "Invalid child type for array, \"%s\"!\n", name );
			break;
		case NL_PROP_I32: {
			DebugParser( "Reading integer\n" );
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				bool status;
				int i = PlParseInteger( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse integer for array, \"%s\"!\n", name );
					break;
				}
				DebugParser( "PushBack Integer: %d\n", i );
				NL_PushBackI32( arrayNode, NULL, i );
				SkipToNextToken( buf, &currentLine );
			}
			break;
		}
		case NL_PROP_F32: {
			DebugParser( "Reading float\n" );
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				bool status;
				float i = PlParseFloat( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse integer for array, \"%s\"!\n", name );
					break;
				}
				DebugParser( "PushBack Float: %f\n", i );
				NL_PushBackF32( arrayNode, NULL, i );
				SkipToNextToken( buf, &currentLine );
			}
			break;
		}
		case NL_PROP_OBJ: {
			DebugParser( "Reading object\n" );
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				if ( ParseObjectNode( arrayNode, buf, length, 0 ) == NULL ) {
					Warning( "Failed to parse object node for array, \"%s\"!\n", name );
					break;
				}
				SkipToNextToken( buf, &currentLine );
			}
			break;
		}
		case NL_PROP_BOOL: {
			DebugParser( "Reading boolean\n" );
			while ( *( *buf ) != '\0' && *( *buf ) != '}' ) {
				bool status;
				int i = PlParseInteger( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse integer for array, \"%s\"!\n", name );
					break;
				}
				DebugParser( "PushBack Boolean: %d\n", i );
				NL_PushBackI32( arrayNode, NULL, i );
				SkipToNextToken( buf, &currentLine );
			}
			break;
		}
		case NL_PROP_STR: {
			DebugParser( "Reading string\n" );
			do {
				char i[ NL_MAX_STRING_LENGTH ];
				if ( PlParseEnclosedString( buf, i, sizeof( i ) ) == NULL ) {
					Warning( "Failed to parse enclosed string for array, \"%s\"!\n", name );
					break;
				}
				DebugParser( "PushBack String: %s\n", i );
				NL_PushBackStr( arrayNode, NULL, i );
				SkipToNextToken( buf, &currentLine );
			} while ( *( *buf ) != '\0' && *( *buf ) != '}' );
			break;
		}
		case NL_PROP_LINK: {
			DebugParser( "Reading link\n" );
			assert( 0 );
			break;
		}
	}

	if ( *( *buf ) != '}' ) {
		Warning( "No closing brace for array, \"%s\"!\n", name );
		return arrayNode;
	}
	( *buf )++;

	DebugParser( "Leaving ParseArrayNode\n" );
	return arrayNode;
}

static NLNode *ParseNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine );
static NLNode *ParseObjectNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine ) {
	DebugParser( "Entering ParseObjectNode\n" );

	char name[ NL_MAX_NAME_LENGTH ] = { '\0' };
	if ( parent == NULL || parent->type != NL_PROP_ARRAY ) {
		if ( ParseToken( buf, name, sizeof( name ), &currentLine ) == NULL ) {
			Warning( "Failed to parse name!\n" );
			return NULL;
		}
	}
	DebugParser( "name( %s )\n", name );

	/* make sure the object is followed by an opening brace */
	SkipToNextToken( buf, &currentLine );
	if ( *( *buf ) != '{' ) {
		Warning( "No opening brace for object, \"%s\"!\n", name );
		return NULL;
	}
	( *buf )++;

	NLNode *objectNode = NL_PushBackObj( parent, name );
	if ( objectNode == NULL ) {
		return NULL;
	}

	/* read in all the children nodes */
	SkipToNextToken( buf, &currentLine );
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

	DebugParser( "Leaving ParseObjectNode\n" );
	return objectNode;
}

static NLNode *ParseNode( NLNode *parent, const char **buf, size_t length, unsigned int currentLine ) {
	DebugParser( "Entering ParseNode\n" );

	/* now try reading in the type */
	char type[ NL_MAX_TYPE_LENGTH ];
	if ( ParseToken( buf, type, sizeof( type ), &currentLine ) == NULL ) {
		return NULL;
	}
	DebugParser( "type( %s )\n", type );

	NLPropertyType propertyType = PropertyTypeForString( type );
	/* an array is a special case, parsing-wise */
	if ( propertyType == NL_PROP_ARRAY ) {
		return ParseArrayNode( parent, buf, length, currentLine );
	} else if ( propertyType == NL_PROP_OBJ ) {
		return ParseObjectNode( parent, buf, length, currentLine );
	} else {
		char name[ NL_MAX_NAME_LENGTH ];
		if ( ParseToken( buf, name, sizeof( name ), &currentLine ) == NULL ) {
			Warning( "Failed to parse name [%d]!\n", currentLine );
			return NULL;
		}
		DebugParser( "name( %s )\n", name );

		/* figure out what data type it is and read in it's result */
		switch ( propertyType ) {
			case NL_PROP_I32: {
				bool status;
				int i = PlParseInteger( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse integer, \"%s\" [%d]!\n", name, currentLine );
					return NULL;
				}
				DebugParser( "PushBack Integer: %d\n", i );
				return NL_PushBackI32( parent, name, i );
			}
			case NL_PROP_F32: {
				bool status;
				float i = PlParseFloat( buf, &status );
				if ( !status ) {
					Warning( "Failed to parse float, \"%s\" [%d]!\n", name, currentLine );
					return NULL;
				}
				DebugParser( "PushBack Float: %f\n", i );
				return NL_PushBackF32( parent, name, i );
			}
			case NL_PROP_STR: {
				char i[ NL_MAX_STRING_LENGTH ];
				if ( PlParseEnclosedString( buf, i, sizeof( i ) ) == NULL ) {
					Warning( "Failed to parse string, \"%s\" [%d]!\n", name, currentLine );
					return NULL;
				}
				DebugParser( "PushBack String: %s\n", i );
				return NL_PushBackStr( parent, name, i );
			}
			case NL_PROP_BOOL: {
				char i[ NL_MAX_BOOL_LENGTH ];
				if ( ParseToken( buf, i, sizeof( i ), &currentLine ) == NULL ) {
					Warning( "Failed to parse boolean, \"%s\" [%d]!\n", name, currentLine );
					return NULL;
				}
				DebugParser( "PushBack Boolean: %s\n", i );
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

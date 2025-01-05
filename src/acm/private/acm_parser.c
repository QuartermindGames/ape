// SPDX-License-Identifier: MIT
// Ape Config Markup
// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_linkedlist.h>

#include "acm_private.h"

static const AcmLexerToken *get_next_token( const AcmLexerToken *token )
{
	AcmLexerToken    *nextToken = NULL;
	PLLinkedListNode *nextNode  = PlGetNextLinkedListNode( token->node );
	if ( nextNode != NULL )
	{
		nextToken = PlGetLinkedListNodeUserData( nextNode );
	}

	return nextToken;
}

typedef struct VariableProcessor
{
	const char     *symbol;
	AcmPropertyType propertyType;
	AcmTokenType   *acceptedTokenTypes;
	unsigned int    numTokenTypes;
} VariableProcessor;

static VariableProcessor variableProcessors[] = {
        {"string",  ACM_PROPERTY_TYPE_STRING,  ( AcmTokenType[] ){ ACM_TOKEN_TYPE_STRING, ACM_TOKEN_TYPE_IDENTIFIER }, 2},
        {"bool",    ACM_PROPERTY_TYPE_BOOL,    ( AcmTokenType[] ){ ACM_TOKEN_TYPE_STRING, ACM_TOKEN_TYPE_IDENTIFIER }, 2},
        {"uint8",   ND_PROPERTY_UI8,           ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1},
        {"uint16",  ND_PROPERTY_UI16,          ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1},
        {"uint32",  ND_PROPERTY_UI32,          ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1},
        {"uint",    ND_PROPERTY_UI32,          ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1}, // shorthand uint32
        {"uint64",  ND_PROPERTY_UI64,          ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1},
        {"int8",    ND_PROPERTY_INT8,          ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1},
        {"int16",   ND_PROPERTY_INT16,         ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1},
        {"int32",   ND_PROPERTY_INT32,         ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1},
        {"int",     ND_PROPERTY_INT32,         ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1}, // shorthand int32
        {"int64",   ND_PROPERTY_INT64,         ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER },                           1},
        {"float",   ACM_PROPERTY_TYPE_FLOAT32, ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER, ACM_TOKEN_TYPE_DECIMAL },   2},
        {"float64", ACM_PROPERTY_TYPE_FLOAT64, ( AcmTokenType[] ){ ACM_TOKEN_TYPE_INTEGER, ACM_TOKEN_TYPE_DECIMAL },   2},
};
#define NUM_VARIABLE_TYPES PL_ARRAY_ELEMENTS( variableProcessors )

static AcmBranch *parse_branch_variable( const char *name, const AcmLexerToken *typeToken, const AcmLexerToken *valueToken, AcmBranch *parent, const AcmLexerToken **currentToken )
{
	AcmBranch *branch = NULL;

	for ( uint i = 0; i < NUM_VARIABLE_TYPES; ++i )
	{
		if ( strcmp( typeToken->symbol, variableProcessors[ i ].symbol ) != 0 )
		{
			continue;
		}

		bool valid = false;
		for ( uint j = 0; j < variableProcessors[ i ].numTokenTypes; ++j )
		{
			valid = ( valueToken->type == variableProcessors[ i ].acceptedTokenTypes[ j ] );
			if ( valid )
			{
				break;
			}
		}

		if ( !valid )
		{
			Warning( "Unexpected value type for %s (%s): %u:%u (%s)\n",
			         typeToken->symbol,
			         valueToken->symbol, valueToken->lineNum, valueToken->linePos, valueToken->path );
			break;
		}

		branch = acm_push_variable_( parent, name, valueToken->symbol, variableProcessors[ i ].propertyType );
		break;
	}

	*currentToken = get_next_token( *currentToken );
	return branch;
}

static AcmBranch *parse_branch( const AcmLexerToken *token, AcmBranch *parent, const AcmLexerToken **currentToken );
static AcmBranch *parse_branch_object( const AcmLexerToken *token, AcmBranch *parent, const AcmLexerToken **currentToken )
{
	const AcmLexerToken *peekToken;
	if ( parent == NULL || parent->type != ACM_PROPERTY_TYPE_ARRAY )
	{
		if ( token->type != ACM_TOKEN_TYPE_IDENTIFIER )
		{
			Warning( "Unexpected token type for object: %u:%u (%s)\n", token->lineNum, token->linePos, token->path );
			return NULL;
		}

		peekToken = get_next_token( token );
	}
	else
	{
		peekToken = token;
	}

	if ( peekToken == NULL || peekToken->type != ACM_TOKEN_TYPE_OPEN_BRACKET )
	{
		Warning( "No opening bracket following object: %u:%u (%s)\n", token->lineNum, token->linePos, token->path );
		return NULL;
	}

	AcmBranch *branch = acm_push_object( parent, token->symbol );

	peekToken = get_next_token( peekToken );
	while ( peekToken != NULL && peekToken->type != ACM_TOKEN_TYPE_CLOSE_BRACKET )
	{
		*currentToken = peekToken;
		parse_branch( *currentToken, branch, currentToken );
		peekToken = *currentToken;
	}

	if ( peekToken == NULL )
	{
		Warning( "No closing bracket following object: %u:%u (%s)\n", token->lineNum, token->linePos, token->path );
	}

	*currentToken = get_next_token( *currentToken );
	return branch;
}

static AcmBranch *parse_branch_array( const AcmLexerToken *token, AcmBranch *parent, const AcmLexerToken **currentToken )
{
	if ( token->type != ACM_TOKEN_TYPE_TYPENAME )
	{
		Warning( "Expected typename to follow array (%s): %u:%u (%s)\n", token->symbol, token->lineNum, token->linePos, token->path );
		return NULL;
	}

	bool isObject = false;
	if ( strcmp( token->symbol, "object" ) == 0 )
	{
		isObject = true;
	}
	else if ( strcmp( token->symbol, "array" ) == 0 )
	{
		Warning( "Invalid typename following array (%s): %u:%u (%s)\n", token->symbol, token->lineNum, token->linePos, token->path );
		return NULL;
	}

	const AcmLexerToken *identifierToken = get_next_token( token );
	if ( identifierToken == NULL || identifierToken->type != ACM_TOKEN_TYPE_IDENTIFIER )
	{
		Warning( "Expected identifier to follow typename: %u:%u (%s)\n", token->lineNum, token->linePos, token->path );
		return NULL;
	}

	const AcmLexerToken *peekToken = get_next_token( identifierToken );
	if ( peekToken == NULL || peekToken->type != ACM_TOKEN_TYPE_OPEN_BRACKET )
	{
		Warning( "No opening bracket following object: %u:%u (%s)\n", token->lineNum, token->linePos, token->path );
		return NULL;
	}

	// determine child property type
	AcmPropertyType childType = ACM_PROPERTY_TYPE_INVALID;
	if ( strcmp( token->symbol, "object" ) == 0 )
	{
		childType = ACM_PROPERTY_TYPE_OBJECT;
	}
	else
	{
		for ( unsigned int i = 0; i < NUM_VARIABLE_TYPES; ++i )
		{
			if ( strcmp( token->symbol, variableProcessors[ i ].symbol ) != 0 )
			{
				continue;
			}

			childType = variableProcessors[ i ].propertyType;
			break;
		}
	}
	if ( childType == ACM_PROPERTY_TYPE_INVALID )
	{
		Warning( "Unsupported typename following array (%s): %u:%u (%s)\n", token->symbol, token->lineNum, token->linePos, token->path );
		return NULL;
	}

	AcmBranch *branch = acm_push_new_branch( parent, identifierToken->symbol, ACM_PROPERTY_TYPE_ARRAY, childType );

	peekToken = get_next_token( peekToken );
	while ( peekToken != NULL && peekToken->type != ACM_TOKEN_TYPE_CLOSE_BRACKET )
	{
		*currentToken = peekToken;
		if ( isObject )
		{
			parse_branch_object( *currentToken, branch, currentToken );
		}
		else
		{
			parse_branch_variable( NULL, token, *currentToken, branch, currentToken );
		}
		peekToken = *currentToken;
	}

	if ( peekToken == NULL )
	{
		Warning( "No closing bracket following object: %u:%u (%s)\n", token->lineNum, token->linePos, token->path );
	}

	*currentToken = get_next_token( peekToken );
	return branch;
}

static AcmBranch *parse_branch( const AcmLexerToken *token, AcmBranch *parent, const AcmLexerToken **currentToken )
{
	const AcmLexerToken *peekToken = get_next_token( token );

	if ( token->type != ACM_TOKEN_TYPE_TYPENAME )
	{
		*currentToken = peekToken;
		Warning( "Unexpected token type (%u): %u:%u (%s)\n", token->type, token->lineNum, token->linePos, token->path );
		return NULL;
	}

	if ( peekToken == NULL )
	{
		*currentToken = peekToken;
		Warning( "Next token missing for branch: %u:%u (%s)\n", peekToken->lineNum, peekToken->linePos, peekToken->path );
		return NULL;
	}

	AcmBranch *branch = NULL;
	if ( peekToken->type == ACM_TOKEN_TYPE_IDENTIFIER )
	{
		*currentToken = peekToken;
		if ( strcmp( token->symbol, "object" ) == 0 )
		{
			branch = parse_branch_object( peekToken, parent, currentToken );
		}
		else
		{
			// get the value too
			const AcmLexerToken *nameToken = *currentToken;
			*currentToken                  = get_next_token( *currentToken );
			if ( *currentToken == NULL )
			{
				Warning( "Unexpected end of input for variable: %u:%u (%s)\n", nameToken->lineNum, nameToken->linePos, nameToken->path );
				return NULL;
			}

			branch = parse_branch_variable( nameToken->symbol, token, *currentToken, parent, currentToken );
		}
	}
	else if ( peekToken->type == ACM_TOKEN_TYPE_TYPENAME && strcmp( peekToken->symbol, "array" ) != 0 )
	{
		*currentToken = peekToken;
		branch        = parse_branch_array( peekToken, parent, currentToken );
	}
	else
	{
		*currentToken = peekToken;
		Warning( "Unexpected token (%s): %u:%u (%s)\n", token->symbol, token->lineNum, token->linePos, token->path );
	}

	return branch;
}

AcmBranch *acm_parse_buffer( const char *buf, const char *file )
{
	AcmBranch *root  = NULL;
	AcmLexer  *lexer = acm_lexer_parse_buffer_( NULL, buf, file );
	if ( lexer == NULL )
	{
		return NULL;
	}

	PLLinkedListNode *firstNode = PlGetFirstNode( lexer->tokens );
	if ( firstNode != NULL )
	{
		const AcmLexerToken *token = PlGetLinkedListNodeUserData( firstNode );
		root                       = parse_branch( token, NULL, &token );
	}

	PL_DELETE( lexer );

	return root;
}
